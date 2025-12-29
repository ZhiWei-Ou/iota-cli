/**
 * @brief message bus
 * @file mbus.h
 * @author Oswin
 * @date 2025-07-29
 * @details An in-process message bus for inter-thread/inter-module communication.
 *           It follows a client-server model where multiple clients can connect
 *           to a central bus "server" to exchange messages via
 * publish-subscribe and request-reply patterns.
 * @see test/unix/mbus_test.cpp
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef XFRAMEWORK_MBUS__H_
#define XFRAMEWORK_MBUS__H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mbus_message.h"
#include "xdef.h"

/** @brief The maximum number of mbus server instances that can be created. */
#define MBUS_MAX_SERVER_SIZE (12)
/** @brief The default maximum number of subscriptions per client. */
#define MBUS_CLIENT_DEFAULT_MAX_SUBSCRIPTIONS (32)
/** @brief The default capacity of the message queue for a client. */
#define MBUS_CLIENT_DEFAULT_MAX_MESSAGE (128)

/** @brief Macro to enable or disable the asynchronous request feature. */
#ifndef MBUS_ENABLE_ASYNC
#define MBUS_ENABLE_ASYNC 1
#endif

/** @brief Opaque handle to an mbus server (the central bus). */
typedef struct mbus_server_private* mbus;
/** @brief Opaque handle to a client connected to the mbus. */
typedef struct mbus_client_private* mbus_client;

/**
 * @brief Callback function for handling incoming messages for a subscription.
 * @param client The client instance that received the message.
 * @param message The received message.
 */
typedef void (*mbus_message_handler)(mbus_client client,
                                     const mbus_message message);

/**
 * @brief Creates a new mbus server instance.
 * @param uri A unique name (URI) to identify this bus server.
 * @param max_client The maximum number of clients that can connect to this server.
 * @param max_message The capacity of the server's internal message queue.
 * @return A handle to the new mbus server, or NULL on failure.
 */
mbus mbus_server_create(const char* uri, size_t max_client, size_t max_message);

/**
 * @brief Looks up an existing mbus server by its URI.
 * @param uri The URI of the server to find.
 * @return A handle to the found mbus server, or NULL if not found.
 */
mbus mbus_server_lookup(const char* uri);

/**
 * @brief Gets the URI of an mbus server.
 * @param server The mbus server instance.
 * @return A constant string representing the server's URI.
 */
const char* mbus_server_uri(mbus server);

/**
 * @brief Destroys an mbus server and disconnects all clients.
 * @param server The mbus server to destroy.
 * @return X_RET_OK  on success.
 */
err_t mbus_server_destroy(mbus server);

/**
 * @brief Connects a new client to an mbus server.
 * @details This uses default values for subscriptions and message queue size.
 * @param name The name of this client.
 * @param uri The URI of the mbus server to connect to.
 * @return A handle to the new mbus client, or NULL on failure.
 */
mbus_client mbus_connect(const char* name, const char* uri);

/**
 * @brief Connects a new client to an mbus server with custom parameters.
 * @param name The name of this client.
 * @param uri The URI of the mbus server to connect to.
 * @param max_subscription The maximum number of topics this client can subscribe to.
 * @param max_message The capacity of this client's incoming message queue.
 * @param thread_stack_size The stack size for the client's dedicated listener thread.
 * @return A handle to the new mbus client, or NULL on failure.
 */
mbus_client mbus_connect_custom(const char* name,
                                const char* uri,
                                size_t max_subscription,
                                size_t max_message,
                                size_t thread_stack_size);

/**
 * @brief Disconnects a client from the mbus server.
 * @param client The client to disconnect.
 * @return X_RET_OK  on success.
 */
err_t mbus_disconnect(mbus_client client);

/**
 * @brief Gets the name of a client.
 * @param client The client instance.
 * @return A constant string representing the client's name.
 */
const char* mbus_name(mbus_client client);

/**
 * @brief Associates a user-defined context pointer with a client instance.
 * @param client The client instance.
 * @param context The user context pointer.
 * @return X_RET_OK  on success.
 */
err_t mbus_set_context(mbus_client client, void* context);

/**
 * @brief Retrieves the user-defined context from a client instance.
 * @param client The client instance.
 * @return The user context pointer.
 */
void* mbus_context(mbus_client client);

/**
 * @brief Subscribes a client to a message subject (topic).
 * @param client The client instance.
 * @param subject The subject string to subscribe to.
 * @param handler The callback function to execute when a message is received on this subject.
 * @return X_RET_OK  on success.
 */
err_t mbus_subscribe(mbus_client client,
                     const char* subject,
                     mbus_message_handler handler);

/**
 * @brief Unsubscribes a client from a message subject.
 * @param client The client instance.
 * @param subject The subject to unsubscribe from.
 * @return X_RET_OK  on success.
 */
err_t mbus_unsubscribe(mbus_client client, const char* subject);

/**
 * @brief Checks if a client is subscribed to a specific subject.
 * @param client The client instance.
 * @param subject The subject to check.
 * @return xTRUE if subscribed, xFALSE otherwise.
 */
xbool_t mbus_is_subscribed(mbus_client client, const char* subject);

/**
 * @brief Publishes a message to a subject on the bus (fire-and-forget).
 * @param client The client instance publishing the message.
 * @param subject The subject to publish to.
 * @param payload A pointer to the message payload data.
 * @param payload_size The size of the payload data.
 * @return X_RET_OK  on success.
 */
err_t mbus_publish(mbus_client client,
                   const char* subject,
                   const void* payload,
                   size_t payload_size);

/**
 * @brief Sends a request and waits synchronously for a reply.
 * @details This function blocks until a reply is received or the timeout expires.
 * @param client The client instance sending the request.
 * @param subject The subject to send the request on.
 * @param payload A pointer to the request payload data.
 * @param payload_size The size of the payload.
 * @param timeout_ms The maximum time to wait for a reply, in milliseconds.
 * @return A handle to the reply message, or NULL on timeout or error.
 * @note The caller is responsible for freeing the returned message with `mbus_message_delete()`.
 */
mbus_message mbus_request(mbus_client client,
                          const char* subject,
                          const void* payload,
                          size_t payload_size,
                          uint32_t timeout_ms);

#if MBUS_ENABLE_ASYNC
/**
 * @brief Sends a request asynchronously and receives the reply via a callback.
 * @details This function is non-blocking. The request is queued and handled
 *          by the client's background thread. The reply is delivered to the
 *          provided handler.
 * @param client The client instance sending the request.
 * @param subject The subject to send the request on.
 * @param payload A pointer to the request payload data.
 * @param payload_size The size of the payload.
 * @param handler The callback function to execute when the reply is received.
 * @param timeout_ms The maximum time to wait for a reply, in milliseconds.
 * @return X_RET_OK  on success, or an error code if the request could not be queued.
 */
err_t mbus_request_async(mbus_client client,
                         const char* subject,
                         const void* payload,
                         size_t payload_size,
                         mbus_message_handler handler,
                         uint32_t timeout_ms);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* XFRAMEWORK_MBUS__H_ */
