/**
 * @brief FreeRTOS 邮箱
 * @file os_mailbox.c
 * @author Oswin
 * @date 2025-07-15
 * @details This file provides the mailbox implementation for FreeRTOS using queues.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "os_mailbox.h"

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

os_mailbox os_mailbox_create(size_t size) {
  return xQueueCreate(size, sizeof(os_mailbox_message));
}

err_t os_mailbox_destroy(os_mailbox self) {
  if (self == NULL) {
    return X_RET_INVAL;
  }
  vQueueDelete(self);
  return X_RET_OK;
}

err_t os_mailbox_drain(os_mailbox self,
                       void (*cleanup_nullable)(os_mailbox_message)) {
  if (self == NULL) {
    return X_RET_INVAL;
  }

  os_mailbox_message msg;
  while (xQueueReceive(self, &msg, 0) == pdTRUE) {
    if (cleanup_nullable) {
      cleanup_nullable(msg);
    }
  }
  vQueueDelete(self);
  return X_RET_OK;
}

err_t os_mailbox_send(os_mailbox self, os_mailbox_message msg) {
  if (self == NULL) {
    return X_RET_INVAL;
  }
  if (xQueueSend(self, &msg, portMAX_DELAY) == pdPASS) {
    return X_RET_OK;
  }
  return X_RET_ERROR;
}

err_t os_mailbox_send_wait(os_mailbox self,
                           os_mailbox_message msg,
                           uint32_t timeout) {
  if (self == NULL) {
    return X_RET_INVAL;
  }
  if (xQueueSend(self, &msg, pdMS_TO_TICKS(timeout)) == pdPASS) {
    return X_RET_OK;
  }
  return X_RET_TIMEOUT;
}

err_t os_mailbox_recv(os_mailbox self,
                      os_mailbox_message* msg,
                      uint32_t timeout) {
  if (self == NULL || msg == NULL) {
    return X_RET_INVAL;
  }
  if (xQueueReceive(self, msg, pdMS_TO_TICKS(timeout)) == pdPASS) {
    return X_RET_OK;
  }
  return X_RET_TIMEOUT;
}
