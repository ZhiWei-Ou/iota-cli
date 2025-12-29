/**
 * @brief Message bus message API
 * @file mbus_message.h
 * @author Oswin
 * @date 2025-07-29
 * @details Defines the API for the message object used by the mbus system.
 *           This includes functions for creating, managing, and accessing
 *           properties of a message.
 * @see test/unix/mbus_message_test.cpp
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef XFRAMEWORK_MBUS_MESSAGE__H_
#define XFRAMEWORK_MBUS_MESSAGE__H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "os_time.h"
#include "xdef.h"

/**
 * @brief Opaque handle to a message bus message object.
 */
typedef struct mbus_message_private* mbus_message;

/**
 * @brief Creates a new "publish" type message.
 * @details This is for fire-and-forget messages that do not expect a reply.
 * @param subject The subject (topic) to publish the message on.
 * @param payload Pointer to the message payload data. The data will be copied.
 * @param payload_size The size of the payload data.
 * @return A handle to the new message object, or NULL on failure.
 */
mbus_message mbus_message_create_publish(const char* subject,
                                         const void* payload,
                                         size_t payload_size);

/**
 * @brief Creates a new "request" type message.
 * @details This message is intended for request-reply patterns and includes
 *          mechanisms for receiving a reply.
 * @param subject The subject to send the request on.
 * @param payload Pointer to the message payload data. The data will be copied.
 * @param payload_size The size of the payload data.
 * @return A handle to the new message object, or NULL on failure.
 */
mbus_message mbus_message_create_request(const char* subject,
                                         const void* payload,
                                         size_t payload_size);
/**
 * @brief Creates a deep copy of a message object.
 * @param other The message to duplicate.
 * @param preserve_created_at If true, the new message will have the same creation
 *                            timestamp as the original. Otherwise, it gets a
 * new one.
 * @return A handle to the duplicated message object, or NULL on failure.
 */
mbus_message mbus_message_duplicate(const mbus_message other,
                                    xbool_t preserve_created_at);

/**
 * @brief Waits for a reply to a given request message.
 * @details This function should only be called on a message created with
 *          `mbus_message_create_request`. It blocks until another client
 *          sends a reply to this message's reply-to subject.
 * @param request The request message that is awaiting a reply.
 * @param timeout The maximum time to wait, in milliseconds.
 * @return A handle to the reply message, or NULL on timeout or error.
 * @note The caller is responsible for freeing the returned reply message with `mbus_message_destroy()`.
 */
mbus_message mbus_message_wait(mbus_message request, uint32_t timeout);

/**
 * @brief Destroys a message object and frees all associated memory.
 * @param msg The message object to destroy.
 * @return X_RET_OK  on success.
 */
err_t mbus_message_destroy(mbus_message msg);

/**
 * @brief Sets an expiration time for a message.
 * @param msg The message object.
 * @param ms The time-to-live for the message, in milliseconds from now.
 * @return X_RET_OK  on success.
 */
err_t mbus_message_set_expiration(mbus_message msg, uint32_t ms);

/**
 * @brief Checks if a message has expired.
 * @param msg The message object.
 * @return xTRUE if the message has expired, xFALSE otherwise.
 */
xbool_t mbus_message_is_expired(const mbus_message msg);

/**
 * @brief Gets a pointer to the message's data payload.
 * @param msg The message object.
 * @return A pointer to the payload data.
 */
void* mbus_message_data(const mbus_message msg);

/**
 * @brief Gets the size of the message's data payload.
 * @param msg The message object.
 * @return The size of the payload in bytes.
 */
size_t mbus_message_size(const mbus_message msg);

/**
 * @brief Gets the message's subject (topic).
 * @param msg The message object.
 * @return A constant string for the subject.
 */
const char* mbus_message_subject(const mbus_message msg);

/**
 * @brief Gets the message's reply-to subject.
 * @details This is typically only set on request-type messages.
 * @param msg The message object.
 * @return A constant string for the reply-to subject, or NULL if not set.
 */
const char* mbus_message_reply(const mbus_message msg);

/**
 * @brief Gets the system tick count when the message was created.
 * @param msg The message object.
 * @return The creation timestamp.
 */
os_tick_t mbus_message_created_at(const mbus_message msg);

/**
 * @brief Gets the system tick count when the message will expire.
 * @param msg The message object.
 * @return The expiration timestamp, or 0 if no expiration is set.
 */
os_tick_t mbus_message_expires_at(const mbus_message msg);

/**
 * @brief Gets the kind of the message.
 * @param msg The message object.
 * @return A string literal, either "publish" or "request".
 */
const char* mbus_message_kind(const mbus_message msg);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* XFRAMEWORK_MBUS_MESSAGE__H_ */
