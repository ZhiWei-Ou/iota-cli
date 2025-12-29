/**
 * @brief Message bus message API
 * @file mbus_message.c
 * @author Oswin
 * @date 2025-07-29
 * @details 消息总线消息对象
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#define XLOG_MOD "mbus_message"
#include "mbus_message.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "os_mailbox.h"

#define INTERNAL_FREE(x) \
  do {                   \
    xbox_free((x));      \
    (x) = NULL;          \
  } while (0)

typedef enum {
  MBUS_MESSAGE_PUBLISH,
  MBUS_MESSAGE_REQUEST,
} mbus_message_type;

struct mbus_message_private {
  char* subject;
  char* reply;
  void* payload;
  size_t size;
  mbus_message_type type;
  os_mailbox inbox;
  os_tick_t created_at;
  os_tick_t expires_at;
};
static char* mbus_message_generate_inbox(void);
static mbus_message mbus_message_create(mbus_message_type type,
                                        const char* subject,
                                        const void* payload,
                                        size_t payload_size,
                                        xbool_t create_inbox);

mbus_message mbus_message_create_publish(const char* subject,
                                         const void* payload,
                                         size_t payload_size) {
  if (subject == NULL || payload == NULL || payload_size <= 0) return NULL;

  return mbus_message_create(MBUS_MESSAGE_PUBLISH,
                             subject,
                             payload,
                             payload_size,
                             xFALSE);
}

mbus_message mbus_message_create_request(const char* subject,
                                         const void* payload,
                                         size_t payload_size) {
  if (subject == NULL || payload == NULL || payload_size <= 0) return NULL;

  return mbus_message_create(MBUS_MESSAGE_REQUEST,
                             subject,
                             payload,
                             payload_size,
                             xTRUE);
}

mbus_message mbus_message_duplicate(const mbus_message other,
                                    xbool_t preserve_created_at) {
  if (other == NULL) return NULL;

  mbus_message copy = mbus_message_create(MBUS_MESSAGE_PUBLISH,
                                          other->subject,
                                          other->payload,
                                          other->size,
                                          xFALSE);
  if (copy == NULL) return NULL;

  copy->type = other->type;
  copy->expires_at = other->expires_at;

  if (preserve_created_at) {
    copy->created_at = other->created_at;
  }

  if (other->reply) {
    copy->reply = xbox_strdup(other->reply);
    if (copy->reply == NULL) {
      mbus_message_destroy(copy);
      return NULL;
    }
  }

  return copy;
}

mbus_message mbus_message_wait(mbus_message request, uint32_t timeout) {
  if (request == NULL || request->type != MBUS_MESSAGE_REQUEST ||
      request->inbox == NULL)
    return NULL;

  mbus_message reply = NULL;

  // XLOG_D("- wait for reply: %s, inbox: %p, self: %p",
  //         request->reply, request->inbox, request);
  os_mailbox_recv(request->inbox, (os_mailbox_message*)&reply, timeout);

  return reply;
}

err_t mbus_message_destroy(mbus_message msg) {
  if (msg == NULL) return X_RET_INVAL;

  if (msg->reply) INTERNAL_FREE(msg->reply);

  if (msg->subject) INTERNAL_FREE(msg->subject);

  if (msg->payload) INTERNAL_FREE(msg->payload);

  if (msg->inbox) os_mailbox_destroy(msg->inbox);

  INTERNAL_FREE(msg);

  return X_RET_OK;
}

err_t mbus_message_set_expiration(mbus_message msg, uint32_t timeout) {
  if (msg == NULL) return X_RET_INVAL;
  msg->expires_at = msg->created_at + timeout;
  return X_RET_OK;
}

xbool_t mbus_message_is_expired(const mbus_message msg) {
  if (msg == NULL) return xFALSE;

  return msg->expires_at < os_time_now_millis();
}

void* mbus_message_data(const mbus_message msg) {
  if (msg == NULL) return NULL;
  return msg->payload;
}

size_t mbus_message_size(const mbus_message msg) {
  if (msg == NULL) return 0;
  return msg->size;
}

const char* mbus_message_subject(const mbus_message msg) {
  if (msg == NULL) return NULL;
  return msg->subject;
}

const char* mbus_message_reply(const mbus_message msg) {
  if (msg == NULL) return NULL;
  return msg->reply;
}

os_tick_t mbus_message_created_at(const mbus_message msg) {
  if (msg == NULL) return 0;
  return msg->created_at;
}

os_tick_t mbus_message_expires_at(const mbus_message msg) {
  if (msg == NULL) return 0;
  return msg->expires_at;
}

const char* mbus_message_kind(const mbus_message msg) {
  if (msg == NULL) return NULL;
  switch (msg->type) {
    case MBUS_MESSAGE_PUBLISH:
      return "publish";
    case MBUS_MESSAGE_REQUEST:
      return "request";
    default:
      return "unknown";
  }
}

mbus_message mbus_message_create(mbus_message_type type,
                                 const char* subject,
                                 const void* payload,
                                 size_t payload_size,
                                 xbool_t create_inbox) {
  if (subject == NULL || payload == NULL || payload_size <= 0) return NULL;

  void* data = NULL;
  char* topic = NULL;
  char* reply = NULL;
  os_mailbox inbox = NULL;
  mbus_message msg = NULL;

  msg = (mbus_message)xbox_malloc(sizeof(struct mbus_message_private));
  if (msg == NULL) return NULL;
  memset(msg, 0, sizeof(struct mbus_message_private));

  data = xbox_malloc(payload_size);
  if (data == NULL) {
    INTERNAL_FREE(msg);
    return NULL;
  }
  memcpy(data, payload, payload_size);

  topic = xbox_strdup(subject);
  if (topic == NULL) {
    INTERNAL_FREE(data);
    INTERNAL_FREE(msg);
    return NULL;
  }

  if (type == MBUS_MESSAGE_REQUEST) {
    reply = mbus_message_generate_inbox();
    if (reply == NULL) {
      INTERNAL_FREE(topic);
      INTERNAL_FREE(data);
      INTERNAL_FREE(msg);
      return NULL;
    }
  }

  if (create_inbox) {
    inbox = os_mailbox_create(1);
    if (inbox == NULL) {
      INTERNAL_FREE(topic);
      INTERNAL_FREE(reply);
      INTERNAL_FREE(data);
      INTERNAL_FREE(msg);
      return NULL;
    }
  }

  msg->subject = topic;
  msg->reply = reply;
  msg->payload = data;
  msg->size = payload_size;
  msg->type = type;
  msg->inbox = inbox;
  msg->created_at = os_time_now_millis();
  msg->expires_at = 0;

  return msg;
}

static char* mbus_message_generate_inbox(void) {
  static uint32_t id = 0;

  char* inbox = (char*)xbox_malloc(64);
  snprintf(inbox, 64, "/_INBOX/%" PRIu32, id++);

  return inbox;
}

os_mailbox mbus_message_inbox(const mbus_message msg) {
  if (msg == NULL) return NULL;
  return msg->inbox;
}
