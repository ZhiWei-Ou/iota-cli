/**
 * @brief message bus
 * @file mbus.c
 * @author Oswin
 * @date 2025-07-29
 * @details 消息总线
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "mbus.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "os_mailbox.h"
#include "os_mutex.h"
#include "os_thread.h"
#include "xhash.h"
#include "xlog.h"

#define INTERNAL_FREE(x) \
  do {                   \
    xbox_free((x));      \
    (x) = NULL;          \
  } while (0)

struct mbus_subscriber_private {
  char* subject;                 // 主题
  mbus_message_handler handler;  // 处理函数
  mbus_message
      exclusive_request;  // 抢占请求体的所有权，用于判断是不是一次性订阅
};
struct mbus_reply_private {
  char* subject;         // 回复的主题名称
  os_mailbox ref_inbox;  // 一个引用邮箱（不能释放）
  os_tick_t expires_at;  // 过期时间
};
struct mbus_server_private {
  char* name;                // 总线名字
  size_t max_client;         // 最大客户端
  mbus_client* clients;      // 客户端集合
  os_mailbox mb;             // 消息队列
  os_thread work_thread;     // 工作线程
  xbool_t thread_exit;       // 线程退出
  os_mutex lock;             // 互斥锁
  xhash_table* reply_table;  // 回复表
  xhash reply_hash;          // 回复哈希
  os_thread gc_thread;       // 消息回收线程
};
struct mbus_client_private {
  char* name;                  // 客户端名字
  mbus server;                 // 所属总线
  os_mailbox mb;               // 消息队列
  os_thread handle_thread;     // 处理线程
  os_thread gc_thread;         // 消息回收线程
  xbool_t thread_exit;         // 线程退出
  os_mutex lock;               // 互斥锁
  xhash_table* subscriptions;  // 订阅
  xhash hash;                  // 哈希
  void* context;               // 上下文
};

static size_t mbus_server_count = 0;
static mbus mbus_server_pool[MBUS_MAX_SERVER_SIZE] = {0};
static int mbus_pool_sort(const void* a, const void* b);
static err_t mbus_pool_register(mbus server);
static err_t mbus_pool_unregister(mbus server);
static err_t mbus_send_to_server(mbus server, mbus_message msg);
static err_t mbus_send_to_client(mbus_client client, mbus_message msg);
static err_t mbus_server_attach(mbus server, mbus_client client);
static err_t mbus_server_detach(mbus server, mbus_client client);
static int mbus_server_client_sort(const void* a, const void* b);
static void mbus_server_thread(void* argument);
static void mbus_server_background_thread(void* argument);
static void mbus_client_clear_expired_subscribers(mbus_client client);
static void mbus_client_thread(void* argument);
// static void mbus_client_gc_thread(void *argument);
typedef struct mbus_subscriber_private* mbus_subscriber;
static uint32_t mbus_subscribe_hash1(const void*);
static uint32_t mbus_subscribe_hash2(const void*);
static int mbus_subscribe_hash_cmp(const void* elem, const void* key);
static void mbus_subscribe_hash_destroy(void* data);
static mbus_subscriber mbus_subscriber_create(const char* subject,
                                              mbus_message_handler handler);
static mbus_subscriber mbus_subscriber_create_with_request(
    mbus_message request, mbus_message_handler handler);
static xbool_t mbus_subscriber_one_time(mbus_subscriber subscriber);
static xbool_t mbus_subscriber_is_expired(mbus_subscriber subscriber);
static int mbus_subscribe_hash_select_expired(const void* elem,
                                              const void* key);
static err_t mbus_subscriber_destroy(mbus_subscriber subscriber);
typedef struct mbus_reply_private* mbus_reply;
static uint32_t mbus_reply_hash1(const void*);
static uint32_t mbus_reply_hash2(const void*);
static int mbus_reply_hash_cmp(const void* elem, const void* key);
static int mbus_reply_hash_expires_at_cmp(const void* elem, const void* key);
static void mbus_reply_hash_destroy(void* data);
static mbus_reply mbus_reply_create(const char* subject,
                                    os_mailbox inbox,
                                    os_tick_t expires_at);
static err_t mbus_reply_destroy(mbus_reply reply);
extern os_mailbox mbus_message_inbox(const mbus_message msg);
static err_t mbus_subscribe_async(mbus_client client,
                                  mbus_message request,
                                  mbus_message_handler handler,
                                  uint32_t timeout_ms);

static int mbus_pool_sort(const void* a, const void* b) {
  mbus sa = *(mbus*)a;
  mbus sb = *(mbus*)b;

  if (sa == NULL && sb == NULL) return 0;
  if (sa == NULL) return 1;
  if (sb == NULL) return -1;

  return strcmp(sa->name, sb->name);
}

err_t mbus_pool_register(mbus server) {
  if (server == NULL) return X_RET_INVAL;

  if (mbus_server_count >= MBUS_MAX_SERVER_SIZE) {
    XLOG_W("mbus server pool is full");
    return X_RET_FULL;
  }

  mbus_server_pool[mbus_server_count++] = server;

  qsort(mbus_server_pool,
        mbus_server_count,
        sizeof(mbus_server_pool[0]),
        mbus_pool_sort);

  XLOG_D("mbus server: [\"%s\":%p] registered", server->name, server);

  return X_RET_OK;
}

err_t mbus_pool_unregister(mbus server) {
  if (server == NULL) return X_RET_INVAL;

  if (mbus_server_count <= 0) return X_RET_EMPTY;

  for (int i = 0; i < MBUS_MAX_SERVER_SIZE; ++i) {
    if (mbus_server_pool[i] == server) {
      mbus_server_pool[i] = NULL;
      mbus_server_count--;
      break;
    }
  }

  qsort(mbus_server_pool,
        mbus_server_count,
        sizeof(mbus_server_pool[0]),
        mbus_pool_sort);

  XLOG_D("mbus server: [\"%s\":%p] unregistered", server->name, server);

  return X_RET_OK;
}

mbus mbus_server_create(const char* name,
                        size_t max_client,
                        size_t max_message) {
  if (name == NULL || max_client <= 0) return NULL;

  if (mbus_server_count >= MBUS_MAX_SERVER_SIZE ||
      mbus_server_lookup(name) != NULL) {
    XLOG_W(
        "mbus server [\"%s\"] already exists or server pool is full(max server "
        "count %d).",
        name,
        MBUS_MAX_SERVER_SIZE);
    return NULL;
  };

  mbus self = xbox_malloc(sizeof(struct mbus_server_private));
  if (self == NULL) goto error_exit;
  memset(self, 0, sizeof(struct mbus_server_private));

  self->name = xbox_strdup(name);
  if (self->name == NULL) goto error_exit;

  self->max_client = max_client;
  self->clients = xbox_malloc(sizeof(mbus_client) * max_client);
  if (self->clients == NULL) goto error_exit;
  memset(self->clients, 0, sizeof(mbus_client) * max_client);

  self->reply_table = xbox_malloc(sizeof(xhash_table) * max_message);
  if (self->reply_table == NULL) goto error_exit;

  self->reply_hash = xhash_create(self->reply_table,
                                  max_message,
                                  mbus_reply_hash1,
                                  mbus_reply_hash2,
                                  mbus_reply_hash_cmp,
                                  mbus_reply_hash_destroy);
  if (self->reply_hash == NULL) goto error_exit;

  self->lock = os_mutex_create(self->name);
  if (self->lock == NULL) goto error_exit;

  self->mb = os_mailbox_create(max_message);
  if (self->mb == NULL) goto error_exit;

  self->thread_exit = xFALSE;
  self->work_thread = os_thread_create(self->name,
                                       mbus_server_thread,
                                       self,
                                       OS_THREAD_DEFAULT_STACK_SIZE,
                                       OS_THREAD_PRIO_NORMAL);
  if (self->work_thread == NULL) goto error_exit;

  self->gc_thread = os_thread_create(self->name,
                                     mbus_server_background_thread,
                                     self,
                                     OS_THREAD_DEFAULT_STACK_SIZE,
                                     OS_THREAD_PRIO_NORMAL);
  if (self->gc_thread == NULL) goto error_exit;

  mbus_pool_register(self);
  return self;

error_exit:
  if (self == NULL) return NULL;

  self->thread_exit = xTRUE;
  if (self->work_thread) os_thread_destroy(self->work_thread);
  if (self->gc_thread) os_thread_destroy(self->gc_thread);

  if (self->name) INTERNAL_FREE(self->name);

  if (self->clients) INTERNAL_FREE(self->clients);

  if (self->mb) os_mailbox_destroy(self->mb);

  if (self->lock) os_mutex_destroy(self->lock);

  if (self->reply_table) INTERNAL_FREE(self->reply_table);

  if (self->reply_hash) xhash_destroy(self->reply_hash);

  INTERNAL_FREE(self);
  return NULL;
}

mbus mbus_server_lookup(const char* name) {
  if (name == NULL) return NULL;

  for (int i = 0; i < MBUS_MAX_SERVER_SIZE; ++i) {
    if (mbus_server_pool[i] == NULL) return NULL;

    if (strcmp(mbus_server_pool[i]->name, name) == 0) {
      return mbus_server_pool[i];
    }
  }

  return NULL;
}

const char* mbus_server_uri(mbus server) {
  if (server == NULL) return "(null)";

  return server->name;
}

err_t mbus_server_destroy(mbus server) {
  if (server == NULL) return X_RET_INVAL;

  mbus_pool_unregister(server);

  server->thread_exit = xTRUE;
  if (server->work_thread) os_thread_destroy(server->work_thread);
  if (server->gc_thread) os_thread_destroy(server->gc_thread);

  if (server->name) INTERNAL_FREE(server->name);

  if (server->clients) INTERNAL_FREE(server->clients);

  if (server->mb) os_mailbox_destroy(server->mb);

  if (server->lock) os_mutex_destroy(server->lock);

  if (server->reply_hash) xhash_destroy(server->reply_hash);

  if (server->reply_table) INTERNAL_FREE(server->reply_table);

  INTERNAL_FREE(server);

  return X_RET_OK;
}

static err_t mbus_send_to_server(mbus server, mbus_message msg) {
  if (server == NULL || msg == NULL) return X_RET_INVAL;

  return os_mailbox_send_wait(server->mb, (os_mailbox_message)msg, 10);
}

static err_t mbus_send_to_client(mbus_client client, mbus_message msg) {
  if (client == NULL || msg == NULL) return X_RET_INVAL;

  return os_mailbox_send_wait(client->mb, (os_mailbox_message)msg, 10);
}

static err_t mbus_server_attach(mbus server, mbus_client client) {
  if (server == NULL || client == NULL) return X_RET_INVAL;

  xbool_t attach_status = xFALSE;

  os_mutex_lock(server->lock);
  for (size_t idx = 0; idx < server->max_client; ++idx) {
    if (server->clients[idx] != NULL) continue;

    attach_status = xTRUE;
    server->clients[idx] = client;
    qsort(server->clients,
          server->max_client,
          sizeof(mbus_client),
          mbus_server_client_sort);
    break;
  }
  os_mutex_unlock(server->lock);

  XLOG_D("mbus client [\"%s\":%p] attach to server [\"%s\":%p]",
         client->name,
         client,
         server->name,
         server);

  return attach_status ? X_RET_OK : X_RET_ERROR;
}

static err_t mbus_server_detach(mbus server, mbus_client client) {
  if (server == NULL || client == NULL) return X_RET_INVAL;

  os_mutex_lock(server->lock);
  for (size_t idx = 0; idx < server->max_client; ++idx) {
    if (server->clients[idx] != client) continue;

    server->clients[idx] = NULL;
    break;
  }
  os_mutex_unlock(server->lock);

  XLOG_D("mbus client: [\"%s\":%p] detach from server: [\"%s\":%p]",
         client->name,
         client,
         server->name,
         server);

  return X_RET_OK;
}

static int mbus_server_client_sort(const void* a, const void* b) {
  mbus_client sa = *(mbus_client*)a;
  mbus_client sb = *(mbus_client*)b;

  if (sa == NULL && sb == NULL) return 0;
  if (sa == NULL) return 1;
  if (sb == NULL) return -1;

  return sa <= sb ? 0 : 1;
}

/**
 * @brief the mbus server main work thread, this thread will handle all incoming messages.
 *        such as forward or assign message to client.
 *
 */
static void mbus_server_thread(void* argument) {
  mbus self = (mbus)argument;

  char subject[128] = {0};
  while (self->thread_exit == xFALSE) {
    err_t err = X_RET_OK;
    mbus_message msg = NULL;

    err = os_mailbox_recv(self->mb, (os_mailbox_message*)&msg, 333);
    if (err != X_RET_OK || msg == NULL) continue;

    xbool_t pub_sub = mbus_message_reply(msg) == NULL ? xTRUE : xFALSE;
    const char* constant_subject = mbus_message_subject(msg);
    if (constant_subject == NULL) {
      mbus_message_destroy(msg);
      continue;
    }

    snprintf(subject, xARRAY_SIZE(subject), "%s", constant_subject);

    os_mutex_lock(self->lock);

    struct mbus_reply_private search = {0};
    search.subject = subject;

    mbus_reply find_reply = xhash_lookup(self->reply_hash, &search);
    if (find_reply != NULL) {
      err = os_mailbox_send_wait(find_reply->ref_inbox,
                                 (os_mailbox_message)msg,
                                 10);
      if (err != X_RET_OK) {
        mbus_message_destroy(msg);
      }

      XLOG_T(
          "mbus server: [\"%s\":%p], forward message: %p, reply to [subject: "
          "\"%s\", inbox: %p]",
          self->name,
          self,
          msg,
          find_reply->subject,
          find_reply->ref_inbox);

      xhash_remove(self->reply_hash, &search);
      os_mutex_unlock(self->lock);
      continue;
    }

    for (size_t idx = 0; idx < self->max_client; ++idx) {
      mbus_client client = self->clients[idx];
      if (client == NULL) break;

      if (mbus_is_subscribed(client, subject)) {
        if (pub_sub) { /* Pub/Sub model */
          /* Pub/Sub model supports one-to-many communication */
          /* Copy message and send to client that avoid blocking
           * server */
          mbus_message copy = mbus_message_duplicate(msg, xFALSE);
          err = mbus_send_to_client(client, copy);
          if (err != X_RET_OK) mbus_message_destroy(copy);

          XLOG_T(
              "mbus server: [\"%s\":%p], forward message(pub/sub): "
              "[addr: "
              "%p, subject: \"%s\", payload: %p], send to client: "
              "[\"%s\":%p]",
              self->name,
              self,
              msg,
              subject,
              mbus_message_data(msg),
              client->name,
              client);

        } else { /* Request/Response model */
          /* Request/Response model supports one-to-one communication
           */
          /* Directly forward the request to the first availabl client
           */
          /* Set the request to nullptr after it is successfully sent,
           * allowing the client to take ownership of the message */
          mbus_reply r = mbus_reply_create(mbus_message_reply(msg),
                                           mbus_message_inbox(msg),
                                           mbus_message_expires_at(
                                               (mbus_message)msg));

          err = xhash_insert(self->reply_hash, r);
          if (r == NULL || err != X_RET_OK) {
            mbus_reply_destroy(r);
            break;
          }

          mbus_message copy = mbus_message_duplicate(msg, xFALSE);
          err = mbus_send_to_client(client, copy);
          if (err == X_RET_OK) {
            XLOG_T(
                "mbus server: [\"%s\":%p], forward message(req/res): "
                "[addr: %p, subject: \"%s\", payload: %p], send to client: "
                "[\"%s\":%p]",
                self->name,
                self,
                msg,
                subject,
                mbus_message_data(msg),
                client->name,
                client);
            msg = NULL;
            break;
          }
        }
      }
    }

    os_mutex_unlock(self->lock);

    if (pub_sub) mbus_message_destroy(msg);
  }

  XLOG_D("server [\"%s\":%p] thread exit", self->name, self);
}

mbus_client mbus_connect(const char* name, const char* uri) {
  return mbus_connect_custom(name,
                             uri,
                             MBUS_CLIENT_DEFAULT_MAX_SUBSCRIPTIONS,
                             MBUS_CLIENT_DEFAULT_MAX_MESSAGE,
                             2048);
}

mbus_client mbus_connect_custom(const char* name,
                                const char* uri,
                                size_t max_subscription,
                                size_t max_message,
                                size_t thread_stack_size) {
  if (name == NULL || uri == NULL) return NULL;

  mbus server = mbus_server_lookup(uri);
  if (server == NULL) {
    XLOG_W("server \"%s\" is not found", uri);
    return NULL;
  }

  mbus_client self = xbox_malloc(sizeof(struct mbus_client_private));
  if (self == NULL) return NULL;
  memset(self, 0, sizeof(struct mbus_client_private));

  self->name = xbox_strdup(name);
  self->server = server;

  self->mb = os_mailbox_create(max_message);
  if (self->mb == NULL) goto error_exit;

  self->lock = os_mutex_create("mbus_client");
  if (self->lock == NULL) goto error_exit;

  self->subscriptions = xbox_malloc(sizeof(xhash_table) * max_subscription);
  if (self->subscriptions == NULL) goto error_exit;
  memset(self->subscriptions, 0, sizeof(xhash_table) * max_subscription);

  self->hash = xhash_create(self->subscriptions,
                            max_subscription,
                            mbus_subscribe_hash1,
                            mbus_subscribe_hash2,
                            mbus_subscribe_hash_cmp,
                            mbus_subscribe_hash_destroy);
  if (self->hash == NULL) goto error_exit;

  self->thread_exit = xFALSE;
  self->handle_thread = os_thread_create(self->name,
                                         mbus_client_thread,
                                         self,
                                         OS_THREAD_DEFAULT_STACK_SIZE,
                                         OS_THREAD_PRIO_NORMAL);
  if (self->handle_thread == NULL) goto error_exit;

  self->gc_thread = NULL;

  if (mbus_server_attach(server, self) != X_RET_OK) goto error_exit;
  return self;

error_exit:
  mbus_disconnect(self);
  return NULL;
}

err_t mbus_disconnect(mbus_client client) {
  if (client == NULL) return X_RET_INVAL;

  mbus_server_detach(client->server, client);

  client->thread_exit = xTRUE;
  if (client->handle_thread) os_thread_destroy(client->handle_thread);

  if (client->gc_thread) os_thread_destroy(client->gc_thread);

  if (client->lock) os_mutex_destroy(client->lock);

  if (client->mb) os_mailbox_destroy(client->mb);

  if (client->hash) xhash_destroy(client->hash);

  INTERNAL_FREE(client->name);
  INTERNAL_FREE(client->subscriptions);
  INTERNAL_FREE(client);
  return X_RET_OK;
}

const char* mbus_name(mbus_client client) {
  if (client == NULL) return "(null)";

  return client->name;
}

err_t mbus_set_context(mbus_client client, void* context) {
  if (context == NULL) return X_RET_INVAL;

  client->context = context;
  return X_RET_OK;
}

void* mbus_context(mbus_client client) {
  if (client == NULL) return NULL;
  return client->context;
}

static xbool_t mbus_subscriber_is_expired(mbus_subscriber subscriber) {
  if (subscriber == NULL) return xFALSE;

  return mbus_message_is_expired(subscriber->exclusive_request);
}

static int mbus_subscribe_hash_select_expired(const void* elem,
                                              const void* key) {
  xUNUSED(key);

  mbus_subscriber sub = (mbus_subscriber)elem;

  return mbus_subscriber_is_expired(sub) ? 0 : 1;
}

static void mbus_client_clear_expired_subscribers(mbus_client client) {
  if (client == NULL) return;

  int8_t dummy = 0xFF;
  void* expired_subs[12] = {0};
  err_t err;

  os_mutex_lock(client->lock);

  err = xhash_list_lookup(client->hash,
                          &dummy,
                          expired_subs,
                          xARRAY_SIZE(expired_subs),
                          mbus_subscribe_hash_select_expired);

  for (int i = 0; i < err; i++) {
    mbus_subscriber sub = expired_subs[i];
    xhash_remove(client->hash, sub);
  }

  os_mutex_unlock(client->lock);
}

static void mbus_client_thread(void* argument) {
  mbus_client self = (mbus_client)argument;

  if (self == NULL) return;

  while (self->thread_exit == xFALSE) {
    err_t err = X_RET_OK;

#if MBUS_ENABLE_ASYNC
    mbus_client_clear_expired_subscribers(self);
#endif

    mbus_message msg = NULL;
    err = os_mailbox_recv(self->mb, (os_mailbox_message*)&msg, 50);
    if (err != X_RET_OK || msg == NULL) continue;

    struct mbus_subscriber_private dog = {0};
    dog.subject = (char*)mbus_message_subject(msg);

    os_mutex_lock(self->lock);
    mbus_subscriber sub = xhash_lookup(self->hash, &dog);
    os_mutex_unlock(self->lock);
    if (sub == NULL) {
      mbus_message_destroy(msg);
      continue;
    }

    sub->handler(self, msg);

#if MBUS_ENABLE_ASYNC
    if (mbus_subscriber_one_time(sub)) {
      os_mutex_lock(self->lock);
      xhash_remove(self->hash, sub);
      os_mutex_unlock(self->lock);
    }
#endif

    mbus_message_destroy(msg);
  }

  XLOG_D("mbus client [\"%s\":%p] thread exit", self->name, self);
}

static mbus_subscriber mbus_subscriber_create(const char* subject,
                                              mbus_message_handler handler) {
  if (subject == NULL || handler == NULL) return NULL;

  mbus_subscriber sub = xbox_malloc(sizeof(struct mbus_subscriber_private));
  if (sub == NULL) return NULL;

  sub->subject = xbox_strdup(subject);
  sub->handler = handler;
  sub->exclusive_request = NULL;
  return sub;
}

static mbus_subscriber mbus_subscriber_create_with_request(
    mbus_message request, mbus_message_handler handler) {
  if (request == NULL || handler == NULL) return NULL;

  if (mbus_message_reply(request) == NULL) return NULL;

  mbus_subscriber sub = xbox_malloc(sizeof(struct mbus_subscriber_private));
  if (sub == NULL) return NULL;

  sub->subject = xbox_strdup(mbus_message_reply(request));
  sub->handler = handler;
  sub->exclusive_request = request;

  return sub;
}

static xbool_t mbus_subscriber_one_time(mbus_subscriber subscriber) {
  if (subscriber == NULL) return xFALSE;

  return subscriber->exclusive_request != NULL;
}

static err_t mbus_subscriber_destroy(mbus_subscriber subscriber) {
  if (subscriber == NULL) return X_RET_INVAL;

  if (subscriber->subject) INTERNAL_FREE(subscriber->subject);

  if (subscriber->exclusive_request)
    mbus_message_destroy(subscriber->exclusive_request);

  INTERNAL_FREE(subscriber);
  return X_RET_OK;
}

static uint32_t mbus_subscribe_hash1(const void* e) {
  mbus_subscriber sub = (mbus_subscriber)e;

  assert(sub != NULL && sub->subject != NULL);

  return xhash_fnv1a(sub->subject, strlen(sub->subject));
}

static uint32_t mbus_subscribe_hash2(const void* e) {
  mbus_subscriber sub = (mbus_subscriber)e;

  assert(sub != NULL && sub->subject != NULL);

  return xhash_bkdr(sub->subject, strlen(sub->subject));
}

static int mbus_subscribe_hash_cmp(const void* elem, const void* key) {
  mbus_subscriber a = (mbus_subscriber)elem;
  mbus_subscriber b = (mbus_subscriber)key;

  assert(a != NULL && a->subject != NULL);
  assert(b != NULL && b->subject != NULL);

  return strcmp(a->subject, b->subject);
}

static void mbus_subscribe_hash_destroy(void* data) {
  mbus_subscriber_destroy((mbus_subscriber)data);
}

err_t mbus_subscribe(mbus_client client,
                     const char* subject,
                     mbus_message_handler handler) {
  if (client == NULL || subject == NULL || handler == NULL) return X_RET_INVAL;

  mbus_subscriber sub = mbus_subscriber_create(subject, handler);
  if (sub == NULL) return X_RET_NOMEM;

  os_mutex_lock(client->lock);
  err_t err = xhash_insert(client->hash, sub);
  os_mutex_unlock(client->lock);

  if (err != X_RET_OK) {
    XLOG_D(
        "mbus client [\"%s\":%p] subscribe \"%s\" failed, error: %s, error "
        "code: %d",
        client->name,
        client,
        subject,
        err_str(err),
        err);
    mbus_subscriber_destroy(sub);
    return err;
  }

  XLOG_D("mbus client [\"%s\":%p] subscribe [\"%s\"]",
         client->name,
         client,
         subject);

  return X_RET_OK;
}

err_t mbus_unsubscribe(mbus_client client, const char* subject) {
  if (client == NULL || subject == NULL) return X_RET_INVAL;

  struct mbus_subscriber_private sub = {0};
  sub.subject = (char*)subject;

  os_mutex_lock(client->lock);
  err_t err = xhash_remove(client->hash, &sub);
  os_mutex_unlock(client->lock);

  if (err == X_RET_OK) {
    XLOG_D("mbus client [\"%s\":%p] unsubscribe [\"%s\"]",
           client->name,
           client,
           subject);
  }

  return err;
}

xbool_t mbus_is_subscribed(mbus_client client, const char* subject) {
  if (client == NULL || subject == NULL) return xFALSE;

  struct mbus_subscriber_private search = {0};
  search.subject = (char*)subject;

  os_mutex_lock(client->lock);
  void* match = xhash_lookup(client->hash, &search);
  os_mutex_unlock(client->lock);

  return (match != NULL) ? xTRUE : xFALSE;
}

err_t mbus_publish(mbus_client client,
                   const char* subject,
                   const void* payload,
                   size_t payload_size) {
  if (client == NULL || subject == NULL || payload == NULL || payload_size <= 0)
    return X_RET_INVAL;

  mbus_message msg = mbus_message_create_publish(subject,
                                                 payload,
                                                 payload_size);
  if (msg == NULL) return X_RET_NOMEM;

  err_t err = mbus_send_to_server(client->server, msg);
  if (err != X_RET_OK) mbus_message_destroy(msg);

  return X_RET_OK;
}

mbus_message mbus_request(mbus_client client,
                          const char* subject,
                          const void* payload,
                          size_t payload_size,
                          uint32_t timeout_ms) {
  if (client == NULL || subject == NULL || payload == NULL || payload_size <= 0)
    return NULL;

  mbus_message request = mbus_message_create_request(subject,
                                                     payload,
                                                     payload_size);
  if (request == NULL) return NULL;

  err_t err = mbus_send_to_server(client->server, request);
  if (err != X_RET_OK) {
    mbus_message_destroy(request);
    return NULL;
  }

  mbus_message response = mbus_message_wait(request, timeout_ms);

  mbus_message_destroy(request);

  return response;
}

err_t mbus_subscribe_async(mbus_client client,
                           mbus_message request,
                           mbus_message_handler handler,
                           uint32_t timeout_ms) {
  if (client == NULL || request == NULL || handler == NULL || timeout_ms == 0)
    return X_RET_INVAL;

  mbus_message_set_expiration(request, timeout_ms);

  mbus_subscriber sub = mbus_subscriber_create_with_request(request, handler);
  if (sub == NULL) return X_RET_NOMEM;

  os_mutex_lock(client->lock);
  err_t err = xhash_insert(client->hash, sub);
  os_mutex_unlock(client->lock);

  if (err != X_RET_OK) {
    sub->exclusive_request = NULL;
    mbus_subscriber_destroy(sub);
  }

  return err;
}

#if MBUS_ENABLE_ASYNC
err_t mbus_request_async(mbus_client client,
                         const char* subject,
                         const void* payload,
                         size_t payload_size,
                         mbus_message_handler handler,
                         uint32_t timeout_ms) {
  if (client == NULL || subject == NULL || payload == NULL ||
      payload_size <= 0 || handler == NULL || timeout_ms == 0)
    return X_RET_INVAL;

  mbus_message request = mbus_message_create_request(subject,
                                                     payload,
                                                     payload_size);
  if (request == NULL) return X_RET_NOMEM;

  if (mbus_subscribe_async(client, request, handler, timeout_ms) != X_RET_OK) {
    mbus_message_destroy(request);
    return X_RET_ERROR;
  }

  return mbus_send_to_server(client->server, request);
}
#endif

static uint32_t mbus_reply_hash1(const void* a) {
  mbus_reply reply = (mbus_reply)a;
  assert(reply != NULL && reply->subject != NULL);

  return xhash_fnv1a(reply->subject, strlen(reply->subject));
}
static uint32_t mbus_reply_hash2(const void* a) {
  mbus_reply reply = (mbus_reply)a;
  assert(reply != NULL && reply->subject != NULL);
  return xhash_fnv1a(reply->subject, strlen(reply->subject));
}
static int mbus_reply_hash_cmp(const void* elem, const void* key) {
  mbus_reply a = (mbus_reply)elem;
  mbus_reply b = (mbus_reply)key;

  assert(a != NULL && a->subject != NULL);
  assert(b != NULL && b->subject != NULL);

  return strcmp(a->subject, b->subject);
}
static void mbus_reply_hash_destroy(void* data) {
  mbus_reply_destroy((mbus_reply)data);
}
static mbus_reply mbus_reply_create(const char* subject,
                                    os_mailbox inbox,
                                    os_tick_t expires_at) {
  if (subject == NULL || inbox == NULL) return NULL;

  mbus_reply reply = (mbus_reply)xbox_malloc(sizeof(struct mbus_reply_private));
  if (reply == NULL) return NULL;
  memset(reply, 0, sizeof(struct mbus_reply_private));

  reply->subject = xbox_strdup(subject);
  if (reply->subject == NULL) {
    INTERNAL_FREE(reply);
    return NULL;
  }

  reply->ref_inbox = inbox;
  reply->expires_at = expires_at;

  return reply;
}
static err_t mbus_reply_destroy(mbus_reply reply) {
  if (reply == NULL) return X_RET_INVAL;

  if (reply->subject) INTERNAL_FREE(reply->subject);

  /* do not free inbox(the inbox is referenced),
   * refer to mbus_reply_create() */

  INTERNAL_FREE(reply);

  return X_RET_OK;
}

static void mbus_server_reply_gc(mbus self) {
  // TODO: Remove and free timed-out reply messages

  /* the mbus_reply_private object is an intermediate object created by the
   * server (in the heap) the mbus request message will shares some read-only
   * fields with mbus_reply_private, and the object only held by the mbus
   * server, therefore, we start a GC to free this intermediate
   * object(mbus_reply_private) */
  os_mutex_lock(self->lock);
  void* match[64] = {0};
  err_t match_count = 0;
  struct mbus_reply_private search = {0};
  search.expires_at = os_time_now_millis();

  match_count = xhash_list_lookup(self->reply_hash,
                                  &search,
                                  match,
                                  xARRAY_SIZE(match),
                                  mbus_reply_hash_expires_at_cmp);

  for (int i = 0; i < match_count; i++) {
    mbus_reply reply = (mbus_reply)match[i];
    xhash_remove(self->reply_hash, reply);
  }

  os_mutex_unlock(self->lock);
}

/**
 * @brief Background service thread of the mbus server
 *
 * Responsibilities:
 *  1. perform GC actions for server.
 *  2. handle async tasks.
 *  3. provide room for future extensions
 *
 */
static void mbus_server_background_thread(void* argument) {
  mbus self = (mbus)argument;

  while (self->thread_exit == xFALSE) {
    os_msleep(100);

    // GC
    mbus_server_reply_gc(self);

    // handle async task
  }

  XLOG_D("mbus server [\"%s\":%p] gc thread exit", self->name, self);
}

static int mbus_reply_hash_expires_at_cmp(const void* elem, const void* key) {
  mbus_reply a = (mbus_reply)elem;
  mbus_reply b = (mbus_reply)key;

  return (a->expires_at > b->expires_at) ? 0 : 1;
}
