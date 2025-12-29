/**
 * @brief RT-Thread 线程管理
 * @file os_thread.c
 * @author Oswin
 * @date 2025-06-30
 * @details
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "os_thread.h"

#include <rtthread.h>

#include "xlist.h"

struct rt_thread_list {
  struct rt_mutex lock;
  xlist head;
};

static struct rt_thread_list global_thread_list = {0};

static int os_thread_init(void) {
  rt_mutex_init(&global_thread_list.lock,
                "gtl", /* global thread list lock */
                RT_IPC_FLAG_PRIO);

  global_thread_list.head = xlist_create();
  return 0;
}
INIT_COMPONENT_EXPORT(os_thread_init);

struct rt_thread_context {
  rt_thread_t handle_;
  rt_mailbox_t mailbox_;
  char* name_;
  os_thread_entry entry_;
  void* parameter_;
};

static void rtthread_entry(void* param) {
  struct rt_thread_context* self = (struct rt_thread_context*)param;

  rt_mutex_take(&global_thread_list.lock, RT_WAITING_FOREVER);
  xlist_push_back(global_thread_list.head, self);
  rt_mutex_release(&global_thread_list.lock);

  self->entry_(self->parameter_);

  rt_mutex_take(&global_thread_list.lock, RT_WAITING_FOREVER);
  xlist_remove(global_thread_list.head, self);
  rt_mutex_release(&global_thread_list.lock);

  rt_mb_send(self->mailbox_, 0);
}

os_thread os_thread_create(const char* name,
                           os_thread_entry entry,
                           void* param,
                           size_t stack_size,
                           os_thread_priority priority) {
  struct rt_thread_context* self = (struct rt_thread_context*)rt_malloc(
      sizeof(struct rt_thread_context));
  if (self == NULL) return NULL;

  self->name_ = rt_strdup(name);
  self->entry_ = entry;
  self->parameter_ = param;

  self->handle_ = rt_thread_create(name,
                                   rtthread_entry,
                                   self,
                                   stack_size,
                                   priority,
                                   50);
  if (self->handle_ == NULL) {
    rt_free(self->name_);
    rt_free(self);
    return NULL;
  }

  self->mailbox_ = rt_mb_create(name, 1, RT_IPC_FLAG_PRIO);
  if (self->mailbox_ == NULL) {
    rt_thread_delete(self->handle_);
    rt_free(self->name_);
    rt_free(self);
    return NULL;
  }

  rt_thread_startup(self->handle_);

  return self;
}

err_t os_thread_destroy(os_thread self) {
  if (self == NULL) return X_RET_INVAL;

  rt_ubase_t value = 0;
  rt_mb_recv(self->mailbox_, &value, RT_WAITING_FOREVER);

  rt_mb_delete(self->mailbox_);
  rt_free(self->name_);
  rt_free(self);

  return X_RET_OK;
}

const char* os_thread_name(os_thread self) {
  if (self == NULL) return "";

  return self->name_;
}

os_thread os_thread_self(void) {
  os_thread self = NULL;
  rt_thread_t x = rt_thread_self();
  rt_mutex_take(&global_thread_list.lock, RT_WAITING_FOREVER);
  void* data;
  xlist_foreach(global_thread_list.head, data) {
    struct rt_thread_context* ctx = (struct rt_thread_context*)data;
    if (ctx->handle_ == x) {
      self = ctx;
      break;
    }
  }
  rt_mutex_release(&global_thread_list.lock);

  return self;
}

void os_thread_yield(void) { rt_thread_yield(); }

void os_sleep(int sec) { os_msleep(sec * 1000); }

void os_msleep(int ms) { rt_thread_mdelay(ms); }
