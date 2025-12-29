/**
 * @brief NATS broker implementation
 * @file broker_nats.h
 * @author Oswin
 * @date 2025-07-13
 * @details Provides the concrete NATS implementation of the abstract broker interface.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef __BROKER_NATS__H_
#define __BROKER_NATS__H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "broker.h"
#include "xdef.h"

/**
 * @brief Opaque handle to the internal state of a NATS broker client.
 */
typedef struct nats_broker_private* nats_broker;

/**
 * @brief Creates a new NATS broker client instance.
 * @param addr The server address (hostname or IP).
 * @param port The server port.
 * @param context An optional user-defined context pointer to associate with this broker.
 * @return A handle to the new NATS broker instance, or NULL on failure.
 */
nats_broker nats_broker_create(const char* addr, int port, broker context);

/**
 * @brief Gets the virtual function table for the NATS broker implementation.
 * @details This function returns the set of operations (publish, subscribe, etc.)
 *          that implements the abstract broker API for the NATS protocol.
 * @return A pointer to the static `broker_impl_ops_t` struct for the NATS broker.
 */
broker_impl_ops_t* nats_broker_get_ops(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __BROKER_NATS__H_ */
