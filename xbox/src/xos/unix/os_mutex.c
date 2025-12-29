/**
 * @brief Unix 互斥锁
 * @file os_mutex.c
 * @author Oswin
 * @date 2025-06-30
 * @details
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "os_mutex.h"

#include <stdlib.h>

os_mutex os_mutex_create(const char* name) {
  xUNUSED(name);

  pthread_mutex_t* m = xbox_malloc(sizeof(pthread_mutex_t));

  pthread_mutex_init(m, NULL);

  return m;
}

err_t os_mutex_lock(os_mutex m) {
  if (!m) return X_RET_INVAL;

  pthread_mutex_lock(m);

  return X_RET_OK;
}

err_t os_mutex_unlock(os_mutex m) {
  if (!m) return X_RET_INVAL;

  pthread_mutex_unlock(m);

  return X_RET_OK;
}

err_t os_mutex_destroy(os_mutex m) {
  if (!m) return X_RET_INVAL;

  pthread_mutex_destroy(m);
  xbox_free(m);

  return X_RET_OK;
}
