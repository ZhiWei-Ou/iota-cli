/**
 * @brief MQTT broker implementation
 * @file broker_mqtt.h
 * @author Oswin
 * @date 2025-10-27
 * @details Provides the concrete MQTT implementation of the abstract broker interface.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef BROKER_MQTT__H_
#define BROKER_MQTT__H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "broker.h"
#include "xdef.h"

/**
 * @brief Opaque handle to the internal state of an MQTT broker client.
 */
typedef struct mqtt_broker_private* mqtt_broker;

/**
 * @brief Creates a new MQTT broker client instance.
 * @param addr The server address (hostname or IP).
 * @param port The server port.
 * @param context An optional user-defined context pointer to associate with this broker.
 * @return A handle to the new MQTT broker instance, or NULL on failure.
 */
mqtt_broker mqtt_broker_create(const char* addr, int port, void* context);

/**
 * @brief Gets the virtual function table for the MQTT broker implementation.
 * @details This function returns the set of operations (publish, subscribe, etc.)
 *          that implements the abstract broker API for the MQTT protocol.
 * @return A pointer to the static `broker_impl_ops_t` struct for the MQTT broker.
 */
broker_impl_ops_t* mqtt_broker_get_ops(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* BROKER_MQTT__H_ */
