/**
 * @brief FreeRTOS 邮箱
 * @file os_mailbox.h
 * @author Oswin
 * @date 2025-07-15
 * @details
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef OS_MAILBOX_H_
#define OS_MAILBOX_H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "xdef.h"

typedef intptr_t os_mailbox_message;
typedef QueueHandle_t os_mailbox;

/**
 * @brief Create a mailbox
 *
 * @param size The maximum number of messages in the mailbox
 * @return os_mailbox
 */
os_mailbox os_mailbox_create(size_t size);

/**
 * @brief Destroy the mailbox
 *
 * @param self
 * @return err_t
 */
err_t os_mailbox_destroy(os_mailbox self);

/**
 * @brief Drain and destroy the mailbox
 *
 * @param self
 * @param cleanup
 * @return err_t
 */
err_t os_mailbox_drain(os_mailbox self,
                       void (*cleanup_nullable)(os_mailbox_message));

/**
 * @brief Send a message (blocks until sent)
 *
 * @param self
 * @param msg
 * @return err_t
 */
err_t os_mailbox_send(os_mailbox self, os_mailbox_message msg);

/**
 * @brief Send a message (blocks until timeout or sent)
 *
 * @param self
 * @param msg
 * @param timeout milliseconds
 * @return err_t
 */
err_t os_mailbox_send_wait(os_mailbox self,
                           os_mailbox_message msg,
                           uint32_t timeout);

/**
 * @brief Receive a message (blocks until timeout or received)
 *
 * @param self
 * @param msg
 * @param timeout
 * @return err_t
 */
err_t os_mailbox_recv(os_mailbox self,
                      os_mailbox_message* msg,
                      uint32_t timeout);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* OS_MAILBOX_H_ */
