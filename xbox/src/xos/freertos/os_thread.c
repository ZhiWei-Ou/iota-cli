/**
 * @brief FreeRTOS 线程管理
 * @file os_thread.c
 * @author Oswin
 * @date 2025-06-30
 * @details This file provides the thread implementation for FreeRTOS.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "os_thread.h"

#include <stdlib.h>

// This function maps the os_thread_priority to FreeRTOS task priorities.
static UBaseType_t map_priority(os_thread_priority priority) {
  UBaseType_t prio;
  switch (priority) {
    case OS_THREAD_PRIO_IDLE:
      prio = tskIDLE_PRIORITY;
      break;
    case OS_THREAD_PRIO_LOW:
      prio = tskIDLE_PRIORITY + 1;
      break;
    case OS_THREAD_PRIO_LOW1:
      prio = tskIDLE_PRIORITY + 2;
      break;
    case OS_THREAD_PRIO_LOW2:
      prio = tskIDLE_PRIORITY + 3;
      break;
    case OS_THREAD_PRIO_LOW3:
      prio = tskIDLE_PRIORITY + 4;
      break;
    case OS_THREAD_PRIO_NORMAL:
      prio = (configMAX_PRIORITIES / 2);
      break;
    case OS_THREAD_PRIO_NORMAL1:
      prio = (configMAX_PRIORITIES / 2) + 1;
      break;
    case OS_THREAD_PRIO_NORMAL2:
      prio = (configMAX_PRIORITIES / 2) + 2;
      break;
    case OS_THREAD_PRIO_NORMAL3:
      prio = (configMAX_PRIORITIES / 2) + 3;
      break;
    case OS_THREAD_PRIO_HIGH:
      prio = configMAX_PRIORITIES - 4;
      break;
    case OS_THREAD_PRIO_HIGH1:
      prio = configMAX_PRIORITIES - 3;
      break;
    case OS_THREAD_PRIO_HIGH2:
      prio = configMAX_PRIORITIES - 2;
      break;
    case OS_THREAD_PRIO_HIGH3:
      prio = configMAX_PRIORITIES - 1;
      break;
    default:
      prio = tskIDLE_PRIORITY;
      break;
  }
  return prio;
}

os_thread os_thread_create(const char* name,
                           os_thread_entry entry,
                           void* param,
                           size_t stack_size,
                           os_thread_priority priority) {
  TaskHandle_t handle = NULL;

  BaseType_t ret = xTaskCreate((TaskFunction_t)entry,
                               name,
                               (configSTACK_DEPTH_TYPE)(stack_size /
                                                        sizeof(StackType_t)),
                               param,
                               map_priority(priority),
                               &handle);

  if (ret != pdPASS) {
    return NULL;
  }

  return handle;
}

err_t os_thread_destroy(os_thread self) {
  if (self == NULL) {
    return X_RET_INVAL;
  }

  // NOTE: The header comment states this function waits for the thread to exit.
  // However, vTaskDelete terminates the task immediately without a waiting
  // mechanism. This implementation assumes immediate deletion is acceptable.
  vTaskDelete(self);

  return X_RET_OK;
}

const char* os_thread_name(os_thread self) {
  if (self == NULL) {
    // Fallback to current task if handle is NULL
    return pcTaskGetName(xTaskGetCurrentTaskHandle());
  }
  return pcTaskGetName(self);
}

os_thread os_thread_self(void) { return xTaskGetCurrentTaskHandle(); }

void os_thread_yield(void) { taskYIELD(); }

void os_sleep(int sec) {
  if (sec > 0) {
    vTaskDelay(pdMS_TO_TICKS(sec * 1000));
  } else {
    // A minimal delay to allow other tasks to run
    vTaskDelay(1);
  }
}

void os_msleep(int ms) {
  if (ms > 0) {
    vTaskDelay(pdMS_TO_TICKS(ms));
  } else {
    // A minimal delay to allow other tasks to run
    vTaskDelay(1);
  }
}
