/**
 * @brief 线程池组件
 * @file thread_pool.h
 * @author Oswin
 * @date 2025-07-21
 * @details A thread pool implementation for executing concurrent tasks.
 *           The API is based on submitting "worker" objects, which encapsulate
 *           the task function and its context, to a pool of threads.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef THREAD_POOL_H_
#define THREAD_POOL_H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "os_time.h"
#include "xdef.h"

/** @brief Opaque handle to a worker object, representing a single task. */
typedef struct worker_private* worker;

/**
 * @brief Function pointer for the main task to be executed by a worker.
 * @param self The worker instance executing the task.
 * @param closure User-provided data associated with the task.
 */
typedef void (*worker_todo)(worker self, void* closure);

/**
 * @brief Function pointer for a hook executed before the main task.
 * @param self The worker instance.
 * @param closure User-provided data.
 */
typedef void (*worker_pre_todo_hook)(worker self, void* closure);

/**
 * @brief Function pointer for a hook executed after the main task.
 * @param self The worker instance.
 * @param closure User-provided data.
 */
typedef void (*worker_post_todo_hook)(worker self, void* closure);

/**
 * @brief Creates a new worker (task) object.
 * @param todo The main function to be executed.
 * @param closure_nullable User-defined data to be passed to the todo function. Can be NULL.
 * @return A handle to the new worker object, or NULL on failure.
 */
worker new_worker(worker_todo todo, void* closure_nullable);

/**
 * @brief Creates a new worker object with lifecycle hooks and a separate context.
 * @param todo The main function to be executed.
 * @param pre_todo_hook_nullable A function to run before `todo`. Can be NULL.
 * @param post_todo_hook_nullable A function to run after `todo`. Can be NULL.
 * @param closure_nullable User-defined data to be passed to the hook and todo functions. Can be NULL.
 * @param context_nullable A separate user-defined context pointer for the worker. Can be NULL.
 * @return A handle to the new worker object, or NULL on failure.
 */
worker new_worker_advance(worker_todo todo,
                          worker_pre_todo_hook pre_todo_hook_nullable,
                          worker_post_todo_hook post_todo_hook_nullable,
                          void* closure_nullable,
                          void* context_nullable);

/**
 * @brief Associates a user-defined context pointer with a worker.
 * @param self The worker instance.
 * @param context The user context pointer.
 * @return X_RET_OK  on success.
 */
err_t worker_set_context(worker self, void* context);

/**
 * @brief Retrieves the user-defined context from a worker.
 * @param self The worker instance.
 * @return The user context pointer.
 */
void* worker_get_context(const worker self);

/**
 * @brief Gets the system tick count when the worker was created.
 * @param self The worker instance.
 * @return The creation timestamp.
 */
os_tick_t worker_create_at(const worker self);

/**
 * @brief Gets the system tick count when the worker finished execution.
 * @param self The worker instance.
 * @return The completion timestamp.
 */
os_tick_t worker_done_at(const worker self);

/**
 * @brief Releases a worker object and frees its memory.
 * @param self The worker object to release.
 * @return X_RET_OK  on success.
 */
err_t worker_release(worker self);

/** @brief Opaque handle to a thread pool instance. */
typedef struct thread_pool_private* thread_pool;

/**
 * @brief Creates a new thread pool.
 * @param size The number of threads to maintain in the pool.
 * @param max_workers The capacity of the task queue.
 * @return A handle to the new thread pool, or NULL on failure.
 */
thread_pool thread_pool_create(int size, int max_workers);

/**
 * @brief Destroys a thread pool gracefully.
 * @details This function waits for all queued tasks to be completed by the
 *          worker threads before shutting down and freeing the pool.
 * @param pool The thread pool to destroy.
 * @return X_RET_OK  on success.
 */
err_t thread_pool_destroy(thread_pool pool);

/**
 * @brief Submits a worker (task) to the thread pool for execution.
 * @param pool The thread pool.
 * @param work The worker object to be executed. The pool takes ownership of the worker.
 * @param wait_ms The maximum time to wait (in milliseconds) if the task queue is full.
 *                Use 0 for no wait, or a negative value to wait indefinitely.
 * @return X_RET_OK  on success.
 * @return X_RET_TIMEOUT if the queue is full and the wait time expires.
 * @return X_RET_FULL if the queue is full and `wait_ms` is 0.
 */
err_t thread_pool_submit_work(thread_pool pool, worker work, uint32_t wait_ms);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* THREAD_POOL_H_ */
