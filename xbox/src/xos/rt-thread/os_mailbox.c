/**
 * @brief RT-Thread 邮箱
 * @file os_mailbox.c
 * @author Oswin
 * @date 2025-07-15
 * @details
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "os_mailbox.h"

os_mailbox os_mailbox_create(size_t size) {
  return rt_mb_create("", size, RT_IPC_FLAG_FIFO);
}

err_t os_mailbox_destroy(os_mailbox self) {
  if (self == NULL) return X_RET_INVAL;

  rt_err_t err = rt_mb_delete(self);

  return err == RT_EOK ? X_RET_OK : X_RET_ERROR;
}

err_t os_mailbox_drain(os_mailbox self,
                       void (*cleanup_nullable)(os_mailbox_message)) {
  if (self == NULL) return X_RET_INVAL;

  if (cleanup_nullable) {
    while (xTRUE) {
      os_mailbox_message msg;
      rt_err_t err = rt_mb_recv(self, &msg, 5);
      if (err == RT_EOK) {
        if (cleanup_nullable) cleanup_nullable(msg);
      } else {
        break;
      }
    }
  }

  return os_mailbox_destroy(self);
}

err_t os_mailbox_send(os_mailbox self, os_mailbox_message msg) {
  if (self == NULL) return X_RET_INVAL;

  rt_err_t err = rt_mb_send(self, msg);

  return err == RT_EOK ? X_RET_OK : X_RET_ERROR;
}

err_t os_mailbox_send_wait(os_mailbox self,
                           os_mailbox_message msg,
                           uint32_t timeout) {
  if (self == NULL) return X_RET_INVAL;

  rt_err_t err = rt_mb_send_wait(self,
                                 msg,
                                 timeout * RT_TICK_PER_SECOND / 1000);

  if (err == RT_EOK)
    return X_RET_OK;
  else
    return X_RET_TIMEOUT;
}

err_t os_mailbox_recv(os_mailbox self,
                      os_mailbox_message* msg,
                      uint32_t timeout) {
  if (self == NULL) return X_RET_INVAL;

  rt_err_t err = rt_mb_recv(self, msg, timeout * RT_TICK_PER_SECOND / 1000);

  if (err == RT_EOK)
    return X_RET_OK;
  else
    return X_RET_TIMEOUT;
}
