/**
 * @brief Unix 互斥锁
 * @file os_mutex.h
 * @author Oswin
 * @date 2025-06-30
 * @details
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef OS_MUTEX_H_
#define OS_MUTEX_H_

#include <pthread.h>

#include "xdef.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle to an OS mutex. */
typedef pthread_mutex_t* os_mutex;

/**
 * @brief Creates a new mutex.
 * @param name A descriptive name for the mutex (can be NULL).
 * @return A handle to the created mutex on success, or NULL on failure.
 */
os_mutex os_mutex_create(const char* name);

/**
 * @brief Locks the specified mutex.
 * @param mutex The mutex to lock.
 * @return XBOX_OK on success, or an error code on failure.
 */
err_t os_mutex_lock(os_mutex mutex);

/**
 * @brief Unlocks the specified mutex.
 * @param mutex The mutex to unlock.
 * @return XBOX_OK on success, or an error code on failure.
 */
err_t os_mutex_unlock(os_mutex mutex);

/**
 * @brief Destroys a mutex and frees its resources.
 * @param mutex The mutex to destroy.
 * @return XBOX_OK on success, or an error code on failure.
 */
err_t os_mutex_destroy(os_mutex mutex);

#ifdef __cplusplus
}
#endif

#endif /* OS_MUTEX_H_ */
