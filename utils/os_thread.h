/**
 * @brief Unix 线程管理
 * @file os_thread.h
 * @author Oswin
 * @date 2025-06-30
 * @details
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef OS_THREAD_H_
#define OS_THREAD_H_

#include "xdef.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle to an OS thread. */
typedef struct posix_thread_wrapper* os_thread;

/** @brief Enumeration of thread priorities. */
typedef enum {
  OS_THREAD_PRIO_IDLE, /**< Idle priority */

  OS_THREAD_PRIO_LOW,  /**< Low priority */
  OS_THREAD_PRIO_LOW1, /**< Low priority + 1 */
  OS_THREAD_PRIO_LOW2, /**< Low priority + 2 */
  OS_THREAD_PRIO_LOW3, /**< Low priority + 3 */

  OS_THREAD_PRIO_NORMAL,  /**< Normal priority */
  OS_THREAD_PRIO_NORMAL1, /**< Normal priority + 1 */
  OS_THREAD_PRIO_NORMAL2, /**< Normal priority + 2 */
  OS_THREAD_PRIO_NORMAL3, /**< Normal priority + 3 */

  OS_THREAD_PRIO_HIGH,  /**< High priority */
  OS_THREAD_PRIO_HIGH1, /**< High priority + 1 */
  OS_THREAD_PRIO_HIGH2, /**< High priority + 2 */
  OS_THREAD_PRIO_HIGH3  /**< High priority + 3 */
} os_thread_priority;

/** @brief Default stack size for a new thread. */
#define OS_THREAD_DEFAULT_STACK_SIZE (2048)

/** @brief Typedef for the thread entry function. */
typedef void (*os_thread_entry)(void*);

/**
 * @brief Creates a new thread.
 * @param name The name of the thread.
 * @param entry The entry function for the thread.
 * @param param The parameter to pass to the thread entry function.
 * @param stack_size The stack size for the thread.
 * @param priority The priority of the thread.
 * @return A handle to the created thread on success, or NULL on failure.
 */
os_thread os_thread_create(const char* name,
                           os_thread_entry entry,
                           void* param,
                           size_t stack_size,
                           os_thread_priority priority);
/**
 * @brief Destroys a thread and frees its resources.
 * @param self The thread to destroy.
 * @return XBOX_OK on success, or an error code on failure.
 */
err_t os_thread_destroy(os_thread self);

/**
 * @brief Gets the name of a thread.
 * @param self The thread handle. If NULL, gets the name of the current thread.
 * @return The name of the thread.
 */
const char* os_thread_name(os_thread self_nullable);

/**
 * @brief Gets the handle of the currently running thread.
 * @return The handle of the current thread.
 */
os_thread os_thread_self(void);

/**
 * @brief Yields the processor to other threads.
 */
void os_thread_yield(void);

/**
 * @brief Suspends the current thread for a specified number of seconds.
 * @param sec The number of seconds to sleep.
 */
void os_sleep(int sec);

/**
 * @brief Suspends the current thread for a specified number of milliseconds.
 * @param ms The number of milliseconds to sleep.
 */
void os_msleep(int ms);

#ifdef __cplusplus
}
#endif

#endif /* OS_THREAD_H_ */
