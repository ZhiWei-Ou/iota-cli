/**
 * @brief 通用定义
 * @file xdef.c
 * @author Oswin
 * @date 2025-09-08
 * @details
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "xdef.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_SPRINTF_NOUNALIGNED
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

#ifdef XBOX_ENABLE_XJSON
#include "xjson.h"
#endif

static xbox_hook_t g_xbox_hook = {
    .malloc = malloc,
    .calloc = calloc,
    .realloc = realloc,
    .free = free,
    .exit = exit,
};

const char* xbool_str(xbool_t b) {
  if (b == xFALSE)
    return "false";
  else if (b == xTRUE)
    return "true";
  else
    return "unknown";
}

const char* err_str(err_t err) {
  if (err >= 0) return "Ok";

  switch (err) {
    case X_RET_ERROR:
      return "Error";
    case X_RET_INVAL:
      return "InvalidArgument";
    case X_RET_FULL:
      return "Full";
    case X_RET_EXIST:
      return "Exist";
    case X_RET_NOMEM:
      return "MemoryError";
    case X_RET_NOTENT:
      return "NotFound";
    case X_RET_TIMEOUT:
      return "Timeout";
    case X_RET_NOTSUP:
      return "Unsupported";
    case X_RET_EMPTY:
      return "Empty";
    case X_RET_MISMATCH:
      return "Mismatch";
    case X_RET_BADFMT:
      return "FormatError";
    case X_RET_OVERFLOW:
      return "Overflow";
    default:
      return "UnknownError";
  }
}

void xbox_init_hooks(const xbox_hook_t* hook) {
  if (hook == NULL) return;

  if (hook->malloc) g_xbox_hook.malloc = hook->malloc;

  if (hook->calloc) g_xbox_hook.calloc = hook->calloc;

  if (hook->realloc) g_xbox_hook.realloc = hook->realloc;

  if (hook->free) g_xbox_hook.free = hook->free;

  if (hook->exit) g_xbox_hook.exit = hook->exit;

#ifdef XBOX_ENABLE_XJSON
  xjson_init_hooks(g_xbox_hook.malloc, g_xbox_hook.free);
#endif
}

char* xbox_strdup(const char* str) {
  if (str == NULL) str = "";

  size_t len = strlen(str);
  char* dup = g_xbox_hook.malloc(len + 1);
  if (dup != NULL) memcpy(dup, str, len + 1);
  return dup;
}

int xbox_asprintf(char** strp, const char* fmt, ...) {
  assert(strp);
  assert(fmt);

  va_list ap;
  va_start(ap, fmt);
  int ret = xbox_vasprintf(strp, fmt, ap);
  va_end(ap);
  return ret;
}

int xbox_vasprintf(char** strp, const char* fmt, va_list ap) {
  assert(strp);
  assert(fmt);

  va_list ap_copy;
  va_copy(ap_copy, ap);

  int len = vsnprintf(NULL, 0, fmt, ap_copy);
  va_end(ap_copy);
  if (len < 0) {
    return X_RET_BADFMT;
  }

  *strp = (char*)xbox_malloc(len + 1);
  if (!*strp) {
    return X_RET_NOMEM;
  }

  return vsnprintf(*strp, len + 1, fmt, ap);
}

void* __xbox_malloc(size_t __s) { return g_xbox_hook.malloc(__s); }
void* __xbox_calloc(size_t __c, size_t __s) {
  return g_xbox_hook.calloc(__c, __s);
}
void* __xbox_realloc(void* __p, size_t __s) {
  return g_xbox_hook.realloc(__p, __s);
}
void __xbox_free(void* ptr) { g_xbox_hook.free(ptr); }
void __xbox_exit(int code) { g_xbox_hook.exit(code); }

void* __xbox_malloc_backtrace(const char* __f, int __l, size_t __s) {
  if (!__s) {
    printf("[Mem ASSERT] Invalid allocation size (0 bytes) at %s:%d\n",
           __f,
           __l);
    return NULL;
  }
  void* ptr = g_xbox_hook.malloc(__s);
  if (!ptr) {
    printf("[Mem ASSERT] Failed to allocate %zu bytes at %s:%d\n",
           __s,
           __f,
           __l);
    return NULL;
  }
  return ptr;
}
void* __xbox_calloc_backtrace(const char* __f,
                              int __l,
                              size_t __c,
                              size_t __s) {
  if (!__s) {
    printf("[Mem ASSERT] Invalid allocation size (0 bytes) at %s:%d\n",
           __f,
           __l);
    return NULL;
  }
  void* ptr = g_xbox_hook.calloc(__c, __s);
  if (!ptr) {
    printf("[Mem ASSERT] Failed to allocate %zu bytes at %s:%d\n",
           __s,
           __f,
           __l);
    return NULL;
  }
  return ptr;
}
void* __xbox_realloc_backtrace(const char* __f,
                               int __l,
                               void* __p,
                               size_t __s) {
  if (!__s) {
    printf("[Mem ASSERT] Invalid allocation size (0 bytes) at %s:%d\n",
           __f,
           __l);
    return NULL;
  }
  void* ptr = g_xbox_hook.realloc(__p, __s);
  if (!ptr) {
    printf("[Mem ASSERT] Failed to allocate %zu bytes at %s:%d\n",
           __s,
           __f,
           __l);
    return NULL;
  }
  return ptr;
}
void __xbox_free_backtrace(const char* __f, int __l, void* __p) {
  if (!__p) {
    printf("[Mem ASSERT] Invalid free pointer at %s:%d\n", __f, __l);
    g_xbox_hook.exit(1);
  }
  g_xbox_hook.free(__p);
}

void __xbox_exit_backtrace(const char* __f, int __l, int __c) {
  printf("[EXIT] %s:%d\n", __f, __l);
  g_xbox_hook.exit(__c);
}
