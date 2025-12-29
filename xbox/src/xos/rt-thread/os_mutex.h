/**
 * @brief RT-Thread 互斥锁
 * @file os_mutex.h
 * @author Oswin
 * @date 2025-06-30
 * @details 系统互斥锁
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef OS_MUTEX_H_
#define OS_MUTEX_H_

#include <rtthread.h>

#include "xdef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef rt_mutex_t os_mutex;

os_mutex os_mutex_create(const char* name);
err_t os_mutex_lock(os_mutex mutex);
err_t os_mutex_unlock(os_mutex mutex);
err_t os_mutex_destroy(os_mutex mutex);

#ifdef __cplusplus
}
#endif

#endif /* OS_MUTEX_H_ */
