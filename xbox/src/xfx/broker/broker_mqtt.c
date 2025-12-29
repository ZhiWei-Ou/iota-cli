/**
 * @brief MQTT broker implementation
 * @file broker_mqtt.c
 * @author Oswin
 * @date 2025-10-27
 * @details
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "broker_mqtt.h"

#include <mosquitto.h>

static xbool_t mosquitto_lib_init_flag = xFALSE;

struct mqtt_broker_private {
  struct mosquitto* mosq;
  void* context;
};

static err_t mqtt_publish(void* self,
                          const char* topic,
                          const void* payload,
                          size_t len);
static err_t mqtt_request(void* self,
                          void** resp,
                          size_t* resp_len,
                          const char* topic,
                          const void* payload,
                          size_t len,
                          uint32_t timeout);
static err_t mqtt_subscribe(void* self,
                            const char* topic,
                            broker_callback callback,
                            void* closure);
static err_t mqtt_unsubscribe(void* self, const char* topic);
static err_t mqtt_destroy(void* self);

mqtt_broker mqtt_broker_create(const char* addr, int port, void* context) {
  if (addr == NULL || context == NULL) return NULL;

  if (mosquitto_lib_init_flag == xFALSE) {
    mosquitto_lib_init();
    mosquitto_lib_init_flag = xTRUE;
  }

  mqtt_broker self = xbox_malloc(sizeof(struct mqtt_broker_private));
  if (self == NULL) return NULL;

  self->mosq = mosquitto_new(NULL, true, NULL);
  if (self->mosq == NULL) {
    xbox_free(self);
    return NULL;
  }

  return self;
}

broker_impl_ops_t* mqtt_broker_get_ops(void) {
  static broker_impl_ops_t ops = {
      // .publish = mqtt_publish,
      // .request = mqtt_request,
      // .subscribe = mqtt_subscribe,
      // .unsubscribe = mqtt_unsubscribe,
      // .destroy = mqtt_destroy,
  };

  return &ops;
}

static err_t mqtt_publish(void* self,
                          const char* topic,
                          const void* payload,
                          size_t len) {
  return X_RET_OK;
}

static err_t mqtt_request(void* self,
                          void** resp,
                          size_t* resp_len,
                          const char* topic,
                          const void* payload,
                          size_t len,
                          uint32_t timeout);
static err_t mqtt_subscribe(void* self,
                            const char* topic,
                            broker_callback callback,
                            void* closure);
static err_t mqtt_unsubscribe(void* self, const char* topic);
static err_t mqtt_destroy(void* self);
