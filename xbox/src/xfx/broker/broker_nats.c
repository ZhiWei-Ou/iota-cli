/**
 * @brief NATS broker implementation
 * @file broker_nats.c
 * @author Oswin
 * @date 2025-07-13
 * @details
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "broker_nats.h"

#include <stdlib.h>
#include <string.h>

#include "nats/nats.h"
#include "xlog.h"
#include "xset.h"

struct nats_broker_private {
  natsConnection* nc;
  natsOptions* opt;
  xset subscribers;
  broker ctx;
};

struct nats_subscribe_wrapper {
  nats_broker self;
  natsSubscription* subscribe;
  char* topic;
  broker_callback callback;
  void* closure;
};

static int nats_subscribe_compare(const void*, const void*);
static void nats_subscribe_destroy(void*);

static err_t nats_publish(void* self, const char* topic, const char* message);
static err_t nats_request(void* driver,
                          const char* topic,
                          const char* message,
                          broker_message* resp,
                          uint32_t timeout);
static err_t nats_subscribe(void* broker,
                            const char* topic,
                            broker_callback callback,
                            void* closure);
static err_t nats_unsubscribe(void* broker, const char* topic);
static err_t nats_destroy(void* broker);

nats_broker nats_broker_create(const char* addr, int port, broker context) {
  if (addr == NULL) return NULL;

  natsStatus status;
  char addr_str[128];
  nats_broker broker = xbox_malloc(sizeof(struct nats_broker_private));
  if (broker == NULL) return NULL;

  broker->nc = NULL;
  broker->opt = NULL;
  broker->subscribers = NULL;
  broker->ctx = context;
  broker->subscribers = xset_create(nats_subscribe_compare,
                                    nats_subscribe_destroy);
  if (!broker->subscribers) {
    xbox_free(broker);
    return NULL;
  }

  snprintf(addr_str, sizeof(addr_str), "nats://%s:%d", addr, port);
  natsOptions_Create(&broker->opt);
  natsOptions_SetURL(broker->opt, addr_str);
  natsOptions_SetMaxReconnect(broker->opt, -1);
  natsOptions_SetAllowReconnect(broker->opt, true);
  natsOptions_SetReconnectWait(broker->opt, 1000);

  status = natsConnection_Connect(&broker->nc, broker->opt);
  if (status != NATS_OK) {
    XLOG_W("nats connect failed, error: %s, error_code: %d",
           natsStatus_GetText(status),
           status);

    nats_destroy(broker);
    return NULL;
  }

  return broker;
}

broker_impl_ops_t* nats_broker_get_ops(void) {
  static broker_impl_ops_t ops = {
      .publish = nats_publish,
      .request = nats_request,
      .subscribe = nats_subscribe,
      .unsubscribe = nats_unsubscribe,
      .destroy = nats_destroy,
  };

  return &ops;
}

static err_t nats_publish(void* context,
                          const char* topic,
                          const char* message) {
  natsStatus status = NATS_OK;
  nats_broker self = (nats_broker)context;

  if (self == NULL || topic == NULL || message == NULL) return X_RET_INVAL;

  status = natsConnection_PublishString(self->nc, topic, message);
  if (status != NATS_OK) {
    XLOG_T("nats publish failed, error: %s, error_code: %d",
           natsStatus_GetText(status),
           status);
    return X_RET_ERROR;
  }

  return X_RET_OK;
}

err_t nats_request(void* driver,
                   const char* topic,
                   const char* message,
                   broker_message* resp,
                   uint32_t timeout) {
  if (driver == NULL || topic == NULL || message == NULL || resp == NULL)
    return X_RET_INVAL;

  natsStatus status;
  nats_broker self = (nats_broker)driver;
  natsMsg* resp_msg = NULL;

  status = natsConnection_RequestString(&resp_msg,
                                        self->nc,
                                        topic,
                                        message,
                                        timeout);
  if (status != NATS_OK) {
    if (status == NATS_TIMEOUT) return X_RET_TIMEOUT;

    XLOG_T("nats request failed, error: %s, error_code: %d",
           natsStatus_GetText(status),
           status);
    return X_RET_ERROR;
  }

  if (resp_msg == NULL) {
    XLOG_T("nats request get response is nil.");
    return X_RET_ERROR;
  }

  *resp = broker_message_new(natsMsg_GetData(resp_msg),
                             natsMsg_GetSubject(resp_msg),
                             natsMsg_GetReply(resp_msg));

  natsMsg_Destroy(resp_msg);

  return X_RET_OK;
}

static void nats_callback_adapter(natsConnection* nc,
                                  natsSubscription* sub,
                                  natsMsg* msg,
                                  void* closure) {
  struct nats_subscribe_wrapper* arg = (struct nats_subscribe_wrapper*)closure;
  if (arg == NULL || arg->self == NULL || arg->callback == NULL) return;

  struct broker_message_private message = {
      .data = (char*)natsMsg_GetData(msg),
      .data_len = natsMsg_GetDataLength(msg),
      .topic = (char*)natsMsg_GetSubject(msg),
      .reply = (char*)natsMsg_GetReply(msg),
  };

  XLOG_T("[broker] <<-- MSG_TOPIC: [%s], MSG_PAYLOAD: %.*s",
         natsSubscription_GetSubject(sub),
         natsMsg_GetDataLength(msg),
         natsMsg_GetData(msg));

  arg->callback(arg->self->ctx, &message, arg->closure);

  natsMsg_Destroy(msg);
}

static err_t nats_subscribe(void* driver,
                            const char* topic,
                            broker_callback callback,
                            void* closure) {
  if (driver == NULL || topic == NULL || callback == NULL) return X_RET_INVAL;

  natsStatus result;
  natsSubscription* subscribe = NULL;
  nats_broker self = (nats_broker)driver;

  struct nats_subscribe_wrapper key = {.topic = (char*)topic};
  struct nats_subscribe_wrapper* sub = xset_lookup(self->subscribers, &key);
  if (sub != NULL) {
    return X_RET_EXIST;
  }

  sub = (struct nats_subscribe_wrapper*)xbox_malloc(
      sizeof(struct nats_subscribe_wrapper));
  if (sub == NULL) {
    return X_RET_NOMEM;
  }

  sub->topic = xbox_strdup(topic);
  sub->callback = callback;
  sub->closure = closure;
  sub->self = self;
  sub->subscribe = NULL;
  xset_insert(self->subscribers, sub);
  result = natsConnection_Subscribe(&subscribe,
                                    self->nc,
                                    topic,
                                    nats_callback_adapter,
                                    sub);
  if (result != NATS_OK) {
    XLOG_W("nats subscribe failed, error: %s, error_code: %d",
           natsStatus_GetText(result),
           result);
    xset_remove(self->subscribers, &key);
    return X_RET_ERROR;
  }

  sub->subscribe = subscribe;
  return X_RET_OK;
}

static err_t nats_unsubscribe(void* driver, const char* topic) {
  if (driver == NULL || topic == NULL) return X_RET_INVAL;

  nats_broker self = (nats_broker)driver;
  struct nats_subscribe_wrapper key = {.topic = (char*)topic};
  return xset_remove(self->subscribers, &key);
}

static err_t nats_destroy(void* broker) {
  nats_broker self = (nats_broker)broker;
  if (self == NULL) return X_RET_INVAL;

  xset_destroy(self->subscribers);

  if (self->opt) {
    natsOptions_Destroy(self->opt);
  }

  natsConnection_Close(self->nc);
  natsConnection_Destroy(self->nc);
  xbox_free(self);

  return X_RET_OK;
}

static int nats_subscribe_compare(const void* a, const void* b) {
  struct nats_subscribe_wrapper* src = (struct nats_subscribe_wrapper*)a;
  struct nats_subscribe_wrapper* dst = (struct nats_subscribe_wrapper*)b;

  return strcmp(src->topic, dst->topic);
}

static void nats_subscribe_destroy(void* data) {
  struct nats_subscribe_wrapper* wrapper = (struct nats_subscribe_wrapper*)data;
  if (wrapper == NULL) return;

  if (wrapper->topic) xbox_free(wrapper->topic);

  if (wrapper->subscribe) {
    natsSubscription_Unsubscribe(wrapper->subscribe);
    natsSubscription_Destroy(wrapper->subscribe);
  }

  xbox_free(wrapper);
}
