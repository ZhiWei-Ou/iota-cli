/**
 * @brief 消息中间件组件
 * @file broker.h
 * @author Oswin
 * @date 2025-07-12
 * @details Defines the abstract interface for a message broker, allowing for
 *           pluggable messaging backends like MQTT, NATS, etc.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef XFRAMEWORK_BROKER__H_
#define XFRAMEWORK_BROKER__H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "xdef.h"
#include "xjson.h"

/**
 * @brief Configuration for connecting to a message broker.
 */
typedef struct {
  char type[16]; /**< The type of the broker (e.g., "mqtt", "nats"). */
  char addr[64]; /**< The address (hostname or IP) of the broker. */
  int port;      /**< The port number of the broker. */
} broker_config_t;

/**
 * @brief Deserializes a broker configuration from a JSON object.
 * @param root The JSON object containing the configuration.
 * @param[out] config The broker_config_t struct to populate.
 * @return X_RET_OK  on success.
 */
err_t broker_config_unmarshal(const xjson root, broker_config_t* config);

/**
 * @brief Serializes a broker configuration into a JSON object.
 * @param config The broker_config_t struct to serialize.
 * @param[out] root The JSON object to populate with configuration data.
 * @return X_RET_OK  on success.
 */
err_t broker_config_marshal(const broker_config_t* config, xjson root);

/** @brief Opaque handle to a broker instance. */
typedef struct broker_private* broker;

/** @brief Alias for a pointer to a broker configuration struct. */
typedef broker_config_t* broker_config;

/** @brief Opaque handle to a message received from a broker. */
typedef struct broker_message_private* broker_message;

/**
 * @internal
 * @brief Internal structure for a broker message.
 */
struct broker_message_private {
  char* data;      /**< The payload of the message. */
  size_t data_len; /**< The length of the payload. */
  char* topic;     /**< The topic the message was received on. */
  char* reply; /**< The reply topic, if applicable (for request-reply patterns).
                */
};

/**
 * @brief Callback function for handling incoming messages from a subscription.
 * @param self The broker instance that received the message.
 * @param message The incoming message.
 * @param closure User-provided data passed through from `broker_subscribe`.
 */
typedef void (*broker_callback)(broker self,
                                const broker_message message,
                                void* closure);

/**
 * @brief Factory function to create a broker instance based on configuration.
 * @param conf The configuration specifying the broker type and connection details.
 * @return A handle to a new broker instance, or NULL on failure (e.g., unknown type).
 */
broker broker_factory(broker_config conf);

/**
 * @brief Destroys a broker instance and frees all associated resources.
 * @param self The broker instance to destroy.
 * @return X_RET_OK  on success.
 */
err_t broker_destroy(broker self);

/**
 * @brief Associates a user-defined context pointer with a broker instance.
 * @param self The broker instance.
 * @param context The user-defined context pointer.
 * @return X_RET_OK  on success.
 */
err_t broker_set_context(broker self, void* context);

/**
 * @brief Retrieves the user-defined context from a broker instance.
 * @param self The broker instance.
 * @return The user-defined context pointer.
 */
void* broker_get_context(broker self);

/**
 * @brief Publishes a message to a topic.
 * @param self The broker instance.
 * @param topic The topic to publish to.
 * @param payload The message payload.
 * @return X_RET_OK  on success.
 */
err_t broker_publish(broker self, const char* topic, const char* payload);

/**
 * @brief Sends a request and waits for a reply.
 * @param self The broker instance.
 * @param topic The topic to send the request on.
 * @param payload The request payload.
 * @param[out] resp A pointer to store the response message. The caller must free it with `broker_message_del`.
 * @param timeout The maximum time to wait for a response, in milliseconds.
 * @return X_RET_OK  on success, X_RET_TIMEOUT if no reply is received in time.
 */
err_t broker_request(broker self,
                     const char* topic,
                     const char* payload,
                     broker_message* resp,
                     uint32_t timeout);

/**
 * @brief Subscribes to a topic to receive messages.
 * @param self The broker instance.
 * @param topic The topic to subscribe to.
 * @param callback The function to be called when a message is received.
 * @param closure A user-defined pointer to be passed to the callback.
 * @return X_RET_OK  on success.
 */
err_t broker_subscribe(broker self,
                       const char* topic,
                       broker_callback callback,
                       void* closure);

/**
 * @brief Unsubscribes from a topic.
 * @param self The broker instance.
 * @param topic The topic to unsubscribe from.
 * @return X_RET_OK  on success.
 */
err_t broker_unsubscribe(broker self, const char* topic);

/**
 * @brief Creates a new broker message object.
 * @param message The message payload.
 * @param topic The message topic.
 * @param reply_nullable The reply-to topic (can be NULL).
 * @return A new broker_message handle, or NULL on failure.
 */
broker_message broker_message_new(const char* message,
                                  const char* topic,
                                  const char* reply_nullable);
/**
 * @brief Duplicates a broker message object.
 * @param other The message to duplicate.
 * @return A new, deep-copied broker_message handle.
 */
broker_message broker_message_dup(broker_message other);

/**
 * @brief Deletes a broker message object and frees its memory.
 * @param message The message to delete.
 * @return X_RET_OK  on success.
 */
err_t broker_message_del(broker_message message);

/**
 * @brief Gets the data payload from a message.
 * @param message The message object.
 * @return A constant pointer to the data.
 */
const char* broker_message_data(broker_message message);

/**
 * @brief Gets the size of the data payload.
 * @param message The message object.
 * @return The size of the data in bytes.
 */
size_t broker_message_size(broker_message message);

/**
 * @brief Gets the topic from a message.
 * @param message The message object.
 * @return A constant pointer to the topic string.
 */
const char* broker_message_topic(broker_message message);

/**
 * @brief Gets the reply topic from a message.
 * @param message The message object.
 * @return A constant pointer to the reply topic string, or NULL if not present.
 */
const char* broker_message_reply(broker_message message);

/**
 * @brief A virtual function table (vtable) that defines the operations for a concrete broker implementation.
 * @details Each specific broker backend (e.g., MQTT) must provide an implementation of this interface.
 */
typedef struct {
  /**
   * @brief Pointer to the implementation of the `publish` operation.
   */
  err_t (*publish)(void* self, const char* topic, const char* message);

  /**
   * @brief Pointer to the implementation of the `request` operation.
   */
  err_t (*request)(void* driver,
                   const char* topic,
                   const char* message,
                   broker_message* resp,
                   uint32_t timeout);

  /**
   * @brief Pointer to the implementation of the `subscribe` operation.
   */
  err_t (*subscribe)(void* self,
                     const char* topic,
                     broker_callback callback,
                     void* closure);

  /**
   * @brief Pointer to the implementation of the `unsubscribe` operation.
   */
  err_t (*unsubscribe)(void* self, const char* topic);

  /**
   * @brief Pointer to the implementation of the `destroy` operation.
   */
  err_t (*destroy)(void* self);
} broker_impl_ops_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* XFRAMEWORK_BROKER__H_ */
