/**
 * @brief 消息中间件组件
 * @file broker.c
 * @author Oswin
 * @date 2025-07-12
 * @details A message broker
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "broker.h"

#include <stdlib.h>
#include <string.h>

#include "broker/broker_mqtt.h"
#include "broker/broker_nats.h"
#include "xlog.h"

struct broker_private {
  void* broker_impl;
  broker_impl_ops_t* broker_ops;
  void* ctx;
};

static err_t null_publish(void* self, const char* topic, const char* message) {
  return X_RET_NOTSUP;
}
static err_t null_request(void* driver,
                          const char* topic,
                          const char* message,
                          broker_message* resp,
                          uint32_t timeout) {
  return X_RET_NOTSUP;
}
static err_t null_subscribe(void* self,
                            const char* topic,
                            broker_callback callback,
                            void* closure) {
  return X_RET_NOTSUP;
}
static err_t null_unsubscribe(void* self, const char* topic) {
  return X_RET_NOTSUP;
}
static err_t null_destroy(void* self) { return X_RET_NOTSUP; }
static const broker_impl_ops_t null_ops = {null_publish,
                                           null_request,
                                           null_subscribe,
                                           null_unsubscribe,
                                           null_destroy};

broker_message broker_message_new(const char* msg,
                                  const char* topic,
                                  const char* reply_nullable) {
  if (msg == NULL || topic == NULL) return NULL;

  broker_message message = xbox_malloc(sizeof(struct broker_message_private));
  if (message == NULL) return NULL;

  message->data = xbox_strdup(msg);
  if (message->data == NULL) {
    xbox_free(message);
    return NULL;
  }

  message->data_len = strlen(msg);
  message->topic = xbox_strdup(topic);
  message->reply = reply_nullable ? xbox_strdup(reply_nullable) : NULL;

  return message;
}

broker_message broker_message_dup(broker_message other) {
  if (other == NULL) return NULL;

  return broker_message_new(other->data, other->topic, other->reply);
}

err_t broker_message_del(broker_message message) {
  if (message == NULL) return X_RET_INVAL;

  if (message->data) xbox_free(message->data);

  if (message->topic) xbox_free(message->topic);

  if (message->reply) xbox_free(message->reply);

  xbox_free(message);

  return X_RET_OK;
}

const char* broker_message_data(broker_message message) {
  if (message == NULL) return NULL;
  return message->data;
}

size_t broker_message_size(broker_message message) {
  if (message == NULL) return 0;
  return message->data_len;
}

const char* broker_message_topic(broker_message message) {
  if (message == NULL) return NULL;
  return message->topic;
}

const char* broker_message_reply(broker_message message) {
  if (message == NULL) return NULL;
  return message->reply;
}

static void broker_check_impl(broker self) {
  if (self == NULL) return;

  if (self->broker_impl == NULL) {
    XLOG_W("the broker driver is nil");
  }

  if (self->broker_ops == NULL) {
    XLOG_W("the broker operators is nil");
    self->broker_ops = (broker_impl_ops_t*)&null_ops;
    return;
  }

  if (self->broker_ops->publish == NULL) {
    XLOG_W("the broker publish function is nil");
    self->broker_ops->publish = null_publish;
  }

  if (self->broker_ops->request == NULL) {
    XLOG_W("the broker request function is nil");
    self->broker_ops->request = null_request;
  }

  if (self->broker_ops->subscribe == NULL) {
    XLOG_W("the broker subscribe function is nil");
    self->broker_ops->subscribe = null_subscribe;
  }

  if (self->broker_ops->unsubscribe == NULL) {
    XLOG_W("the broker unsubscribe function is nil");
    self->broker_ops->unsubscribe = null_unsubscribe;
  }

  if (self->broker_ops->destroy == NULL) {
    XLOG_W("the broker destroy function is nil");
    self->broker_ops->destroy = null_destroy;
  }
}

broker broker_factory(broker_config conf) {
  if (conf == NULL) return NULL;

  broker self = xbox_malloc(sizeof(struct broker_private));
  if (self == NULL) return NULL;

  if (strcmp(conf->type, "nats") == 0) {
    self->broker_impl = nats_broker_create(conf->addr, conf->port, self);
    if (self->broker_impl == NULL) {
      xbox_free(self);
      return NULL;
    }
    self->broker_ops = nats_broker_get_ops();
  } else if (strcmp(conf->type, "mqtt") == 0) {
#if 0
        self->broker_impl = mqtt_broker_create(conf->addr, conf->port, self);
        if (self->broker_impl == NULL) {
            xbox_free(self);
            return NULL;
        }
        self->broker_ops = mqtt_broker_get_ops();
#endif
  }

  broker_check_impl(self);
  return self;
}

err_t broker_destroy(broker self) {
  if (self == NULL) return X_RET_INVAL;

  self->broker_ops->destroy(self->broker_impl);
  xbox_free(self);

  return X_RET_OK;
}

err_t broker_set_context(broker self, void* context) {
  if (self == NULL || context == NULL) return X_RET_INVAL;

  self->ctx = context;
  return X_RET_OK;
}

void* broker_get_context(broker self) {
  if (self == NULL) return NULL;

  return self->ctx;
}

err_t broker_publish(broker self, const char* topic, const char* payload) {
  if (self == NULL || topic == NULL || payload == NULL) return X_RET_INVAL;

  return self->broker_ops->publish(self->broker_impl, topic, payload);
}

err_t broker_request(broker self,
                     const char* topic,
                     const char* payload,
                     broker_message* resp,
                     uint32_t timeout) {
  if (self == NULL || topic == NULL || payload == NULL || resp == NULL)
    return X_RET_INVAL;

  return self->broker_ops->request(self->broker_impl,
                                   topic,
                                   payload,
                                   resp,
                                   timeout);
}

err_t broker_subscribe(broker self,
                       const char* topic,
                       broker_callback callback,
                       void* closure) {
  if (self == NULL || topic == NULL || callback == NULL) return X_RET_INVAL;

  return self->broker_ops->subscribe(self->broker_impl,
                                     topic,
                                     callback,
                                     closure);
}

err_t broker_unsubscribe(broker self, const char* topic) {
  if (self == NULL || topic == NULL) return X_RET_INVAL;

  return self->broker_ops->unsubscribe(self->broker_impl, topic);
}

err_t broker_config_unmarshal(const xjson root, broker_config_t* conf) {
  if (root == NULL || conf == NULL) return X_RET_INVAL;

  /* {"type": "nats", "addr": "127.0.0.1", "port": 4222"} */

  strncpy(conf->type,
          xjson_query_string(root, "/broker/type", ""),
          sizeof(conf->type));
  strncpy(conf->addr,
          xjson_query_string(root, "/broker/addr", ""),
          sizeof(conf->addr));
  conf->port = xjson_get_int(root, "/broker/port");

  return X_RET_OK;
}

err_t broker_config_marshal(const broker_config_t* config, xjson root) {
  if (config == NULL || root == NULL) return X_RET_INVAL;

  xjson_set_string(root, "/broker/type", config->type);
  xjson_set_string(root, "/broker/addr", config->addr);
  xjson_set_number(root, "/broker/port", config->port);

  return X_RET_OK;
}
