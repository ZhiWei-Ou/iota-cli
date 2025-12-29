/**
 * @brief RT-Thread 线程管理
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

typedef struct rt_thread_context* os_thread;

typedef enum {
  OS_THREAD_PRIO_IDLE,
  OS_THREAD_PRIO_LOW,
  OS_THREAD_PRIO_LOW1,
  OS_THREAD_PRIO_LOW2,
  OS_THREAD_PRIO_LOW3,
  OS_THREAD_PRIO_NORMAL,
  OS_THREAD_PRIO_NORMAL1,
  OS_THREAD_PRIO_NORMAL2,
  OS_THREAD_PRIO_NORMAL3,
  OS_THREAD_PRIO_HIGH,
  OS_THREAD_PRIO_HIGH1,
  OS_THREAD_PRIO_HIGH2,
  OS_THREAD_PRIO_HIGH3
} os_thread_priority;
#define OS_THREAD_DEFAULT_STACK_SIZE (2048)
typedef void (*os_thread_entry)(void*);

/**
 * @breif This function create a thread
 *
 * @param name The thread name
 * @param entry The thread work function
 * @param param The work function parameter
 * @param stack_size The thread stack_size
 * @param stack_size The thread priority
 *
 * @return os_thread
 */
os_thread os_thread_create(const char* name,
                           os_thread_entry entry,
                           void* param,
                           size_t stack_size,
                           os_thread_priority priority);

/**
 * @breif This function will destroy thread
 *  This function will waiting for the work function exit
 *
 * @param self The os_thread handle
 * @return err_t
 */
err_t os_thread_destroy(os_thread self);

/**
 * @breif This function will return the name of the thread handle.
 *
 * @param self The os_thread handle
 * @return const char *
 *      If error, used empty string "" to intead of the NULL
 */
const char* os_thread_name(os_thread self);

/**
 * @breif Get current thread handle
 *
 * @return os_thread
 */
os_thread os_thread_self(void);
void os_thread_yield(void);

void os_sleep(int sec);
void os_msleep(int ms);

#ifdef __cplusplus
}
#endif

#endif /* OS_THREAD_H_ */
