/**
 * @brief 线程池组件
 * @file thread_pool.c
 * @author Oswin
 * @date 2025-07-21
 * @details A thread pool implementation for executing concurrent tasks.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "thread_pool.h"

#include <stdlib.h>

#include "os_mailbox.h"
#include "os_thread.h"

enum worker_thread_state {
  WORKER_THREAD_TERMINATED,
  WORKER_THREAD_IDLE,
  WORKER_THREAD_BUSY
};
struct worker_thread {
  enum worker_thread_state state;
  os_thread thread;
  thread_pool owner;
};
static struct worker_thread* worker_thread_create(const char* name,
                                                  thread_pool owner);
static err_t worker_thread_destroy(struct worker_thread* wt);

struct worker_private {
  worker_todo todo;
  worker_pre_todo_hook pre_hook;
  worker_post_todo_hook post_hook;
  os_tick_t acquire_at;
  os_tick_t done_at;
  void* context;
  void* closure;
};

struct thread_pool_private {
  int size;
  int max_workers;
  os_mailbox mailbox;
  struct worker_thread* pool[];  // Array of worker threads
};

static void worker_thread(void* argument);

static struct worker_thread* worker_thread_create(const char* name,
                                                  thread_pool owner) {
  if (name == NULL || owner == NULL) return NULL;

  struct worker_thread* self = xbox_malloc(sizeof(struct worker_thread));
  if (self == NULL) return NULL;  // Memory allocation failed

  self->state = WORKER_THREAD_IDLE;
  self->owner = owner;
  self->thread = NULL;

  // Create the thread
  self->thread = os_thread_create(name,
                                  worker_thread,
                                  self,
                                  OS_THREAD_DEFAULT_STACK_SIZE,
                                  OS_THREAD_PRIO_NORMAL);
  if (self->thread == NULL) {
    xbox_free(self);
    return NULL;  // Thread creation failed
  }

  return self;
}

static err_t worker_thread_destroy(struct worker_thread* wt) {
  if (wt == NULL) return X_RET_INVAL;

  // Wait for the thread to finish
  if (wt->thread != NULL) {
    os_thread t = wt->thread;
    wt->thread = NULL;
    os_thread_destroy(t);
  }

  xbox_free(wt);
  return X_RET_OK;
}

worker new_worker(worker_todo todo, void* closure_nullable) {
  return new_worker_advance(todo, NULL, NULL, closure_nullable, NULL);
}

worker new_worker_advance(worker_todo todo,
                          worker_pre_todo_hook pre_todo,
                          worker_post_todo_hook post_todo,
                          void* closure,
                          void* context) {
  if (todo == NULL) return NULL;  // Invalid argument

  worker w = xbox_malloc(sizeof(struct worker_private));
  if (w == NULL) return NULL;  // Memory allocation failed

  w->todo = todo;
  w->pre_hook = pre_todo;
  w->post_hook = post_todo;
  w->closure = closure;
  w->context = context;
  w->acquire_at = os_time_now_micros();
  w->done_at = 0;

  return w;
}

err_t worker_set_context(worker self, void* context) {
  if (self == NULL || context == NULL) return X_RET_INVAL;

  self->context = context;
  return X_RET_OK;
}

void* worker_get_context(const worker self) {
  if (self == NULL) return NULL;  // Invalid argument

  return self->context;
}

os_tick_t worker_create_at(const worker self) {
  if (self == NULL) return 0;  // Invalid argument

  return self->acquire_at;
}

os_tick_t worker_done_at(const worker self) {
  if (self == NULL) return 0;  // Invalid argument

  return self->done_at;
}

err_t worker_release(worker self) {
  if (self == NULL) return X_RET_INVAL;

  xbox_free(self);
  return X_RET_OK;
}

thread_pool thread_pool_create(int size, int max_workers) {
  if (size <= 0 || max_workers <= 0) return NULL;

  thread_pool pool = xbox_malloc(sizeof(struct thread_pool_private) +
                                 sizeof(struct worker_thread*) * size);
  if (pool == NULL) return NULL;

  pool->size = size;
  pool->max_workers = max_workers;

  pool->mailbox = os_mailbox_create((size_t)max_workers);
  if (pool->mailbox == NULL) {
    xbox_free(pool);
    return NULL;
  }

  for (int i = 0; i < size; i++) {
    pool->pool[i] = worker_thread_create("WorkerThread", pool);
    if (pool->pool[i] == NULL) {
      // Clean up previously created threads
      for (int j = 0; j < i; j++) {
        worker_thread_destroy(pool->pool[j]);
      }
      os_mailbox_destroy(pool->mailbox);
      xbox_free(pool);
      return NULL;  // Failed to create worker thread
    }
  }

  return pool;
}

err_t thread_pool_submit_work(thread_pool pool, worker work, uint32_t wait_ms) {
  if (pool == NULL || work == NULL) return X_RET_INVAL;

  // Try to send the work to the mailbox
  err_t err = os_mailbox_send_wait(pool->mailbox,
                                   (os_mailbox_message)work,
                                   wait_ms);
  if (err != X_RET_OK) {
    return err;  // Failed to send work
  }

  return X_RET_OK;
}

static void worker_thread(void* argument) {
  struct worker_thread* self = (struct worker_thread*)argument;
  thread_pool ctx = self->owner;

  if (self == NULL || ctx == NULL) return;

  /* Wait self->thread to be set, refer to worker_thread_create() */
  os_msleep(100);

  while (self->thread != NULL) {
    // Wait for work from the mailbox
    worker work = NULL;
    err_t err = os_mailbox_recv(ctx->mailbox, (os_mailbox_message*)&work, 100);

    if (err != X_RET_OK || work == NULL) continue;

    self->state = WORKER_THREAD_BUSY;

    if (work->pre_hook) work->pre_hook(work, work->closure);

    if (work->todo) work->todo(work, work->closure);

    work->done_at = os_time_now_micros();
    if (work->post_hook) work->post_hook(work, work->closure);

    worker_release(work);

    self->state = WORKER_THREAD_IDLE;
  }

  self->state = WORKER_THREAD_TERMINATED;
}

err_t thread_pool_destroy(thread_pool pool) {
  if (pool == NULL) return X_RET_INVAL;

  for (int i = 0; i < pool->size; i++) {
    worker_thread_destroy(pool->pool[i]);
  }

  os_mailbox_destroy(pool->mailbox);
  xbox_free(pool);
  return X_RET_OK;
}
