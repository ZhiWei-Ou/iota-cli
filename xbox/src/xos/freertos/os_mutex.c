/**
 * @brief FreeRTOS 互斥锁
 * @file os_mutex.c
 * @author Oswin
 * @date 2025-06-30
 * @details This file provides the mutex implementation for FreeRTOS.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "os_mutex.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

os_mutex os_mutex_create(const char* name) {
  (void)name;  // FreeRTOS mutexes don't have names by default.
  return xSemaphoreCreateMutex();
}

err_t os_mutex_lock(os_mutex mutex) {
  if (mutex == NULL) {
    return X_RET_INVAL;
  }
  if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
    return X_RET_OK;
  }
  return X_RET_ERROR;
}

err_t os_mutex_unlock(os_mutex mutex) {
  if (mutex == NULL) {
    return X_RET_INVAL;
  }
  if (xSemaphoreGive(mutex) == pdTRUE) {
    return X_RET_OK;
  }
  return X_RET_ERROR;
}

err_t os_mutex_destroy(os_mutex mutex) {
  if (mutex == NULL) {
    return X_RET_INVAL;
  }
  vSemaphoreDelete(mutex);
  return X_RET_OK;
}
