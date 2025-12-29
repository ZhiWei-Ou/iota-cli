/**
 * @brief Unix 邮箱
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

#include <stdint.h>  // < intptr_t

#include "xdef.h"

/** @brief Mailbox message type. */
typedef intptr_t os_mailbox_message;
/** @brief Opaque handle to an OS mailbox. */
typedef struct os_mailbox_private* os_mailbox;

/**
 * @brief 创建一个邮箱
 * @param size 邮箱最多的邮件数量
 * @return os_mailbox
 */
os_mailbox os_mailbox_create(size_t size);

/**
 * @brief 销毁邮箱
 * @param self The mailbox instance.
 * @return err_t XBOX_OK on success, or an error code on failure.
 */
err_t os_mailbox_destroy(os_mailbox self);

/**
 * @brief 清空并销毁
 * @param self The mailbox instance.
 * @param cleanup_nullable Optional cleanup function for each message.
 * @return err_t XBOX_OK on success, or an error code on failure.
 */
err_t os_mailbox_drain(os_mailbox self,
                       void (*cleanup_nullable)(os_mailbox_message));

/**
 * @brief 发送邮件（阻塞式发送，直到发送完成）
 * @param self The mailbox instance.
 * @param msg The message to send.
 * @return err_t XBOX_OK on success, or an error code on failure.
 */
err_t os_mailbox_send(os_mailbox self, os_mailbox_message msg);

/**
 * @brief 发送邮件（阻塞式发送，直到超时或者发送完成）
 * @param self The mailbox instance.
 * @param msg The message to send.
 * @param timeout 毫秒 Timeout in milliseconds.
 * @return err_t XBOX_OK on success, or an error code on failure.
 */
err_t os_mailbox_send_wait(os_mailbox self,
                           os_mailbox_message msg,
                           uint32_t timeout);

/**
 * @brief 接收邮件（阻塞式接收，直到超时或者接收完成）
 * @param self The mailbox instance.
 * @param msg Pointer to store the received message.
 * @param timeout Timeout in milliseconds.
 * @return err_t XBOX_OK on success, or an error code on failure.
 */
err_t os_mailbox_recv(os_mailbox self,
                      os_mailbox_message* msg,
                      uint32_t timeout);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* OS_MAILBOX_H_ */
