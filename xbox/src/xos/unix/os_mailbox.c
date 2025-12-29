/**
 * @brief Unix 邮箱
 * @file os_mailbox.c
 * @author Oswin
 * @date 2025-07-15
 * @details
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "os_mailbox.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "os_mutex.h"

#if defined(__MACH__)
#include <mach/mach.h>
typedef semaphore_t os_sema;
#elif defined(__unix__)
#include <semaphore.h>
typedef sem_t os_sema;
#elif defined(__MVS__)
#include <zos-semaphore.h>
typedef sem_t os_sema;
#else
#error "platform currently not supported"
#endif

static void os_sema_init(os_sema* sema, int initial_value);
static void os_sema_fini(os_sema* sema);
static xbool_t os_sema_wait(os_sema* sema);
static xbool_t os_sema_timed_wait(os_sema* sema, uint32_t usecs);
static xbool_t os_sema_try_wait(os_sema* sema);
static void os_sema_signal(os_sema* sema);
static void os_sema_signal_with_count(os_sema* sema, int count);

#if defined(__MACH__)
//---------------------------------------------------------
// Apple, IOS, OSX
//---------------------------------------------------------
void os_sema_init(os_sema* sema, int initial_value) {
  assert(sema != NULL);
  assert(initial_value >= 0);
  kern_return_t rc = semaphore_create(mach_task_self(),
                                      sema,
                                      SYNC_POLICY_FIFO,
                                      initial_value);
  assert(rc == KERN_SUCCESS);
  xUNUSED(rc);
}
void os_sema_fini(os_sema* sema) {
  assert(sema != NULL);
  semaphore_destroy(mach_task_self(), *sema);
}
xbool_t os_sema_wait(os_sema* sema) {
  assert(sema != NULL);
  return (semaphore_wait(*sema) == KERN_SUCCESS) ? xTRUE : xFALSE;
}
xbool_t os_sema_timed_wait(os_sema* sema, uint32_t usecs) {
  assert(sema != NULL);
  mach_timespec_t ts;
  ts.tv_sec = (unsigned int)(usecs / 1000000);
  ts.tv_nsec = (int)((usecs % 1000000) * 1000);

  // added in OSX 10.10:
  // https://developer.apple.com/library/prerelease/mac/documentation/General/Reference/APIDiffsMacOSX10_10SeedDiff/modules/Darwin.html
  return (semaphore_timedwait(*sema, ts) == KERN_SUCCESS) ? xTRUE : xFALSE;
}
xbool_t os_sema_try_wait(os_sema* sema) {
  assert(sema != NULL);
  return os_sema_timed_wait(sema, 0);
}
void os_sema_signal(os_sema* sema) {
  assert(sema != NULL);
  while (semaphore_signal(*sema) != KERN_SUCCESS);
}
void os_sema_signal_with_count(os_sema* sema, int count) {
  assert(sema != NULL);
  while (count-- > 0) {
    while (semaphore_signal(*sema) != KERN_SUCCESS);
  }
}
#elif defined(__unix__) || defined(__MVS__)
//---------------------------------------------------------
// POSIX, Linux, zOS
//---------------------------------------------------------
#include <errno.h>
#include <time.h>
#if defined(__GLIBC_PREREQ) && defined(_GNU_SOURCE)
#if __GLIBC_PREREQ(2, 30)
#define SEMA_MONOTONIC
#endif
#endif

void os_sema_init(os_sema* sema, int initial_value) {
  assert(sema != NULL);
  assert(initial_value >= 0);
  int rc = sem_init(sema, 0, initial_value);
  assert(rc == 0);
  xUNUSED(rc);
}
void os_sema_fini(os_sema* sema) {
  assert(sema != NULL);
  sem_destroy(sema);
}
xbool_t os_sema_wait(os_sema* sema) {
  // http://stackoverflow.com/questions/2013181/gdb-causes-sem-wait-to-fail-with-eintr-error
  assert(sema != NULL);
  int rc;
  do {
    rc = sem_wait(sema);
  } while (rc == -1 && errno == EINTR);
  return (rc == 0) ? xTRUE : xFALSE;
}
xbool_t os_sema_timed_wait(os_sema* sema, uint32_t usecs) {
  struct timespec ts;
  const int usecs_in_1_sec = 1000000;
  const int nsecs_in_1_sec = 1000000000;
#ifdef SEMA_MONOTONIC
  clock_gettime(CLOCK_MONOTONIC, &ts);
#else
  clock_gettime(CLOCK_REALTIME, &ts);
#endif
  ts.tv_sec += (time_t)(usecs / usecs_in_1_sec);
  ts.tv_nsec += (long)(usecs % usecs_in_1_sec) * 1000;
  // sem_timedwait bombs if you have more than 1e9 in tv_nsec
  // so we have to clean things up before passing it in
  if (ts.tv_nsec >= nsecs_in_1_sec) {
    ts.tv_nsec -= nsecs_in_1_sec;
    ++ts.tv_sec;
  }

  int rc;
  do {
#ifdef MOODYCAMEL_LIGHTWEIGHTSEMAPHORE_MONOTONIC
    rc = sem_clockwait(sema, CLOCK_MONOTONIC, &ts);
#else
    rc = sem_timedwait(sema, &ts);
#endif
  } while (rc == -1 && errno == EINTR);
  return (rc == 0) ? xTRUE : xFALSE;
}
xbool_t os_sema_try_wait(os_sema* sema) {
  assert(sema != NULL);
  int rc;
  do {
    rc = sem_trywait(sema);
  } while (rc == -1 && errno == EINTR);
  return (rc == 0) ? xTRUE : xFALSE;
}
void os_sema_signal(os_sema* sema) {
  assert(sema != NULL);
  while (sem_post(sema) == -1);
}
void os_sema_signal_with_count(os_sema* sema, int count) {
  assert(sema != NULL);
  while (count-- > 0) {
    while (sem_post(sema) == -1);
  }
}

#else

#error "platform currently not supported"
#endif

typedef struct os_mailbox_private {
  os_mailbox_message* buffer;
  size_t size;
  size_t head;
  size_t tail;
  os_mutex lock;
  os_sema sem_count;  // 消息计数
  os_sema sem_space;  // 空间计数
} os_mailbox_private;

os_mailbox os_mailbox_create(size_t size) {
  os_mailbox_private* mb = xbox_malloc(sizeof(os_mailbox_private));
  if (!mb) return NULL;
  mb->buffer = xbox_malloc(sizeof(os_mailbox_message) * size);
  if (!mb->buffer) {
    xbox_free(mb);
    return NULL;
  }
  mb->size = size;
  mb->head = 0;
  mb->tail = 0;
  mb->lock = os_mutex_create("mailbox_lock");
  os_sema_init(&mb->sem_count, 0);
  os_sema_init(&mb->sem_space, size);
  return (os_mailbox)mb;
}

err_t os_mailbox_destroy(os_mailbox self) {
  os_mailbox_private* mb = (os_mailbox_private*)self;
  if (!mb) return X_RET_INVAL;
  os_mutex_destroy(mb->lock);
  os_sema_fini(&mb->sem_count);
  os_sema_fini(&mb->sem_space);
  xbox_free(mb->buffer);
  xbox_free(mb);
  return X_RET_OK;
}

err_t os_mailbox_drain(os_mailbox self, void (*cleanup)(os_mailbox_message)) {
  os_mailbox_private* mb = (os_mailbox_private*)self;
  if (!mb) return X_RET_INVAL;
  os_mutex_lock(mb->lock);
  while (mb->head != mb->tail) {
    if (cleanup) cleanup(mb->buffer[mb->head]);
    mb->head = (mb->head + 1) % mb->size;
  }
  os_mutex_unlock(mb->lock);

  os_mailbox_destroy(self);
  return X_RET_OK;
}

err_t os_mailbox_send(os_mailbox self, os_mailbox_message msg) {
  os_mailbox_private* mb = (os_mailbox_private*)self;
  if (!mb) return X_RET_INVAL;
  if (os_sema_wait(&mb->sem_space) != xTRUE) return X_RET_TIMEOUT;

  os_mutex_lock(mb->lock);
  mb->buffer[mb->tail] = msg;
  mb->tail = (mb->tail + 1) % mb->size;
  os_mutex_unlock(mb->lock);

  os_sema_signal(&mb->sem_count);

  return X_RET_OK;
}

err_t os_mailbox_send_wait(os_mailbox self,
                           os_mailbox_message msg,
                           uint32_t timeout) {
  os_mailbox_private* mb = (os_mailbox_private*)self;
  if (!mb) return X_RET_INVAL;

  if (os_sema_timed_wait(&mb->sem_space, timeout * 1000) != xTRUE)
    return X_RET_TIMEOUT;

  os_mutex_lock(mb->lock);
  mb->buffer[mb->tail] = msg;
  mb->tail = (mb->tail + 1) % mb->size;
  os_mutex_unlock(mb->lock);

  os_sema_signal(&mb->sem_count);

  return X_RET_OK;
}

err_t os_mailbox_recv(os_mailbox self,
                      os_mailbox_message* msg,
                      uint32_t timeout) {
  os_mailbox_private* mb = (os_mailbox_private*)self;
  if (!mb || !msg) return X_RET_INVAL;

  if (os_sema_timed_wait(&mb->sem_count, timeout * 1000) != xTRUE)
    return X_RET_TIMEOUT;

  os_mutex_lock(mb->lock);
  *msg = mb->buffer[mb->head];
  mb->head = (mb->head + 1) % mb->size;
  os_mutex_unlock(mb->lock);

  os_sema_signal(&mb->sem_space);

  return X_RET_OK;
}
