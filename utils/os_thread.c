/**
 * @brief Unix 线程管理
 * @file os_thread.c
 * @author Oswin
 * @date 2025-06-30
 * @details
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#define _GNU_SOURCE
#include "os_thread.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "xhashmap.h"

typedef struct {
  xhashmap tbl;
  pthread_rwlock_t lock;
} thread_table;

static thread_table* g_thread_table = NULL;

struct posix_thread_wrapper {
  pthread_t thread_id;
  char* name;
  size_t stack_size;
  os_thread_priority priority;
  os_thread_entry entry;
  void* param;
};

static void init_thread_table(void);
static void register_thread(os_thread t);
static os_thread get_thread_by_name(const char* name);
static void unregister_thread(os_thread t);
static os_thread get_thread_by_id(pthread_t id);

static void* posix_thread_proxy(void* arg) {
  os_thread t = (os_thread)arg;

  t->entry(t->param);

  return NULL;
}

os_thread os_thread_create(const char* name,
                           os_thread_entry entry,
                           void* param,
                           size_t stack_size,
                           os_thread_priority priority) {
  if (!name || !entry) return NULL;

  os_thread t = (os_thread)malloc(sizeof(struct posix_thread_wrapper));
  if (!t) return NULL;

  t->thread_id = NULL;
  t->name = xbox_strdup(name);
  t->stack_size = 0;
  t->priority = 0;
  t->entry = entry;
  t->param = param;

  pthread_create(&t->thread_id, NULL, posix_thread_proxy, t);

  register_thread(t);

  return t;
}

err_t os_thread_destroy(os_thread self) {
  if (!self) return X_RET_INVAL;

  unregister_thread(self);

  pthread_join(self->thread_id, NULL);
  xbox_free(self->name);
  xbox_free(self);

  return X_RET_OK;
}

const char* os_thread_name(os_thread self) {
  if (self) {
    return self->name;
  }

  os_thread t = get_thread_by_id(pthread_self());
  if (t) {
    return t->name;
  } else {
    return "(null)";
  }
}

os_thread os_thread_self(void) {
  os_thread t = get_thread_by_id(pthread_self());
  return t;
}

static void init_thread_table(void) {
  if (g_thread_table == NULL) {
    g_thread_table = (thread_table*)malloc(sizeof(thread_table));
    if (g_thread_table == NULL) {
      return;
    }

    g_thread_table->tbl = xhashmap_create();
    pthread_rwlock_init(&g_thread_table->lock, NULL);

    if (g_thread_table->tbl == NULL) {
      free(g_thread_table);
      g_thread_table = NULL;
    }
  }
}

static void register_thread(os_thread t) {
  if (g_thread_table == NULL) {
    init_thread_table();
  }

  if (g_thread_table != NULL) {
    return;
  }

  pthread_rwlock_wrlock(&g_thread_table->lock);
  xhashmap_insert(g_thread_table->tbl, t->name, t);
  pthread_rwlock_unlock(&g_thread_table->lock);
}

static os_thread get_thread_by_name(const char* name) {
  if (g_thread_table == NULL) {
    return NULL;
  }

  pthread_rwlock_rdlock(&g_thread_table->lock);
  os_thread t = (os_thread)xhashmap_lookup(g_thread_table->tbl, name);
  pthread_rwlock_unlock(&g_thread_table->lock);
  return t;
}

static int compare_thread_id(const void* item, const void* key) {
  os_thread t = (os_thread)item;
  pthread_t id = *(pthread_t*)key;
  return pthread_equal(t->thread_id, id) ? 0 : 1;
}

static os_thread get_thread_by_id(pthread_t id) {
  if (g_thread_table == NULL) {
    return NULL;
  }

  pthread_rwlock_rdlock(&g_thread_table->lock);
  os_thread t = NULL;
  xhashmap_lookup_ex(g_thread_table->tbl,
                     &id,
                     (void**)&t,
                     1,
                     compare_thread_id);
  pthread_rwlock_unlock(&g_thread_table->lock);

  return t;
}

static void unregister_thread(os_thread t) {
  if (g_thread_table == NULL) {
    return;
  }

  pthread_rwlock_wrlock(&g_thread_table->lock);
  xhashmap_remove_hold(g_thread_table->tbl, t->name);
  pthread_rwlock_unlock(&g_thread_table->lock);
}

void os_thread_yield(void) { sched_yield(); }

void os_sleep(int sec) { sleep(sec); }

void os_msleep(int ms) { usleep(ms * 1000); }
