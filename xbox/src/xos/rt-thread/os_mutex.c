/**
 * @brief RT-Thread 互斥锁
 * @file os_mutex.c
 * @author Oswin
 * @date 2025-06-30
 * @details
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "os_mutex.h"

os_mutex os_mutex_create(const char* name) {
  return rt_mutex_create(name, RT_IPC_FLAG_PRIO);
}

err_t os_mutex_lock(os_mutex mutex) {
  rt_err_t err = rt_mutex_take(mutex, RT_WAITING_FOREVER);

  return err == RT_EOK ? X_RET_OK : X_RET_ERROR;
}

err_t os_mutex_unlock(os_mutex mutex) {
  rt_err_t err = rt_mutex_release(mutex);

  return err == RT_EOK ? X_RET_OK : X_RET_ERROR;
}

err_t os_mutex_destroy(os_mutex mutex) {
  rt_err_t err = rt_mutex_delete(mutex);

  return err == RT_EOK ? X_RET_OK : X_RET_ERROR;
}
