/**
 * @brief 结构化日志框架
 * @file xlog.c
 * @author Oswin
 * @date 2025-06-25
 * @details A powerful, lightweight logging module for C.
 * @see examples in `test/unix/xlog_test.cpp`
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "xlog.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "stb_sprintf.h"

struct xlog_sink_private {
  void* ctx;                         /* sink context */
  sink_output_func output;           /* output func */
  sink_flush_func flush;             /* flush func */
  sink_ctx_destroy_func destroy_ctx; /* context destory func */
  xbool_t auto_destory; /* true: destroy context when sink deleted */
};

struct xlogger_private {
  void* ctx;              /* logger context */
  xlog_sink* sink;        /* sink array */
  size_t sink_used;       /* sink used */
  size_t sink_capacity;   /* sink capacity */
  xlog_lvl_e lvl;         /* logger level */
  xlog_lvl_e which_flush; /* flush level */
  xlog_output_func pipe;  /* pipe func */
};

/* /path/to/file.txt -> file.txt */
static err_t xlog_sink_del_advance(xlog_sink sink, xbool_t force);
const char* xlog_get_basename(const char* file);
char* xlog_dup_basename(const char* file);
void xlog_default_output(xlogger logger,
                         xlog_sink sinker,
                         const xlog_message_t* const message);
void xlog_default_flush(xlogger logger, xlog_sink sinker);

void xlog_output(xlogger self, const xlog_message_t* message);

static struct xlog_sink_private def_sinker = {
    .ctx = NULL,
    .output = xlog_default_output,
    .flush = xlog_default_flush,
};

static xlog_sink def_sinkers[] = {&def_sinker};
static struct xlogger_private xlog_logger_default_instance = {
    .ctx = NULL,
    .sink = def_sinkers,
    .sink_used = 1,
    .lvl = XLOG_LVL_INFO,
    .which_flush = XLOG_LVL_INFO,
    .pipe = xlog_output,
};
static xlogger xlog_global_logger_instance = &xlog_logger_default_instance;

xlogger xlog_create_logger_default(void) {
  static const xlog_options_t opt = {
      .lvl = XLOG_LVL_INFO,
      .which_flush = XLOG_LVL_INFO,
      .ctx = NULL,
      .redirect = NULL,
  };

  return xlog_create_logger_with_options((xlog_options_t*)&opt);
}

xlogger xlog_create_logger_with_options(xlog_options_t* opt) {
  if (opt == NULL) return NULL;

  struct xlogger_private* new_logger = xbox_malloc(
      sizeof(struct xlogger_private));
  if (new_logger == NULL) return NULL;

  new_logger->lvl = opt->lvl;
  new_logger->which_flush = opt->which_flush;
  new_logger->sink = NULL;
  new_logger->sink_used = 0;
  new_logger->sink_capacity = 0;
  new_logger->ctx = opt->ctx;
  new_logger->pipe = opt->redirect ? opt->redirect : xlog_output;

  return new_logger;
}

xlogger xlog_create_logger_with_sinks(xlog_options_t* opt,
                                      size_t sink_count,
                                      ...) {
  xlogger new_logger = xlog_create_logger(opt);
  if (new_logger == NULL) return NULL;

  va_list args;
  va_start(args, sink_count);

  for (size_t i = 0; i < sink_count; ++i) {
    xlog_sink sink = va_arg(args, xlog_sink);
    xlog_logger_append_sink(new_logger, sink);
  }

  va_end(args);

  return new_logger;
}

xlogger xlog_create_logger_with_sink_array(xlog_options_t* opt,
                                           size_t sink_count,
                                           xlog_sink* sinks) {
  xlogger new_logger = xlog_create_logger(opt);
  if (new_logger == NULL) return NULL;

  for (size_t i = 0; i < sink_count; ++i) {
    xlog_logger_append_sink(new_logger, sinks[i]);
  }

  return new_logger;
}

err_t xlog_logger_append_sink(xlogger self, xlog_sink sink) {
  if (self == NULL || sink == NULL) return X_RET_INVAL;

  if (self->sink == NULL) {
    self->sink = (xlog_sink*)xbox_calloc(3, sizeof(xlog_sink));
    if (self->sink == NULL) return X_RET_NOMEM;

    self->sink_capacity = 3;
  }

  if (self->sink_used >= self->sink_capacity) {
    size_t new_capacity = self->sink_capacity * 2;
    xlog_sink* new_sink = (xlog_sink*)xbox_realloc(self->sink,
                                                   new_capacity *
                                                       sizeof(xlog_sink));
    if (new_sink == NULL) return X_RET_NOMEM;

    self->sink = new_sink;
    self->sink_capacity = new_capacity;
  }

  self->sink[self->sink_used++] = sink;

  return X_RET_OK;
}

xlog_lvl_e xlog_logger_lvl(xlogger self) {
  assert(self != NULL);

  return self->lvl;
}

void* xlog_logger_ctx(xlogger self) {
  if (self == NULL) return NULL;

  return self->ctx;
}

err_t xlog_logger_destroy(xlogger self) {
  if (self == NULL || self == &xlog_logger_default_instance) return X_RET_INVAL;

  if (self->sink) {
    for (size_t idx = 0; idx < self->sink_used; ++idx) {
      xlog_sink sinker = self->sink[idx];
      xlog_sink_del_advance(sinker, xTRUE);
    }

    xbox_free(self->sink);
  }

  xbox_free(self);
  return X_RET_OK;
}

xlogger xlog_global_instance(void) { return xlog_global_logger_instance; }

void* xlog_global_ctx(void) {
  void* ctx = NULL;

  if (xlog_global_logger_instance) ctx = xlog_global_logger_instance->ctx;

  return ctx;
}

xlog_lvl_e xlog_global_lvl(void) { return xlog_global_logger_instance->lvl; }

err_t xlog_global_set_ctx(void* ctx) {
  xlogger self = xlog_global_instance();

  if (self) self->ctx = ctx;

  return X_RET_OK;
}

err_t xlog_global_set_lvl(xlog_lvl_e lvl) {
  xlogger self = xlog_global_instance();

  if (self) self->lvl = lvl;

  return X_RET_OK;
}

err_t xlog_global_set_instance(xlogger new_logger) {
  if (new_logger == NULL) return X_RET_INVAL;

  xlog_global_logger_instance = new_logger;

  return X_RET_OK;
}

err_t xlog_global_reset(void) {
  xlogger curr = xlog_global_instance();

  xlog_global_set_instance(&xlog_logger_default_instance);

  if (curr != &xlog_logger_default_instance) xlog_logger_destroy(curr);

  return X_RET_OK;
}

void xlog_output(xlogger self, const xlog_message_t* message) {
  for (size_t idx = 0; idx < self->sink_used; ++idx) {
    xlog_sink sink = self->sink[idx];

    sink->output(self, sink, message);

    if (message->lvl >= self->which_flush) sink->flush(self, sink);
  }

  if (message->lvl == XLOG_LVL_FATAL) xbox_exit(1);
}

void xlog_pipe(xlogger self, xlog_message_t message) {
  /* check if log level is greater than current logger level */
  if (self == NULL || message.lvl < self->lvl) {
    return;
  }

  self->pipe(self, &message);

  xlog_message_release(&message);
}

const char* xlog_get_basename(const char* file) {
  if (file == NULL) return "";

  /*
   * { /path/to/xlog_test.c } to { xlog_test.c }
   */

  char* p = strrchr(file, '/');
  if (p != NULL)
    return p + 1;
  else
    return file;
}

char* xlog_dup_basename(const char* file) {
  if (file == NULL) return NULL;

  const char* p = strrchr(file, '/');
  if (p != NULL)
    return xbox_strdup(p + 1);
  else
    return xbox_strdup(file);
}

char xlog_lvl_char(xlog_lvl_e lvl) {
  switch (lvl) {
    case XLOG_LVL_TRACE:
      return 'T';
    case XLOG_LVL_DEBUG:
      return 'D';
    case XLOG_LVL_INFO:
      return 'I';
    case XLOG_LVL_WARN:
      return 'W';
    case XLOG_LVL_ERROR:
      return 'E';
    case XLOG_LVL_FATAL:
      return 'F';
    default:
      return 'U';
  }
}

const char* xlog_lvl_str(xlog_lvl_e lvl) {
  switch (lvl) {
    case XLOG_LVL_TRACE:
      return "TRC";
    case XLOG_LVL_DEBUG:
      return "DBG";
    case XLOG_LVL_INFO:
      return "INF";
    case XLOG_LVL_WARN:
      return "WRN";
    case XLOG_LVL_ERROR:
      return "ERR";
    case XLOG_LVL_FATAL:
      return "FTL";
    default:
      return "UNK";
  }
}

const char* xlog_lvl_full_str(xlog_lvl_e lvl) {
  switch (lvl) {
    case XLOG_LVL_TRACE:
      return "trace";
    case XLOG_LVL_DEBUG:
      return "debug";
    case XLOG_LVL_INFO:
      return "info";
    case XLOG_LVL_WARN:
      return "warn";
    case XLOG_LVL_ERROR:
      return "error";
    case XLOG_LVL_FATAL:
      return "fatal";
    default:
      return "unknown";
  }
}

xlog_sink xlog_sink_new(void* ctx_nullable,
                        sink_output_func output,
                        sink_flush_func flush) {
  return xlog_sink_new_with_destory(ctx_nullable, output, flush, NULL);
}

xlog_sink xlog_sink_new_with_destory(
    void* ctx_nullable,
    sink_output_func output,
    sink_flush_func flush,
    sink_ctx_destroy_func destory_ctx_nullable) {
  if (output == NULL || flush == NULL) return NULL;

  xlog_sink new_sink = xbox_malloc(sizeof(struct xlog_sink_private));
  if (new_sink == NULL) return NULL;

  new_sink->ctx = ctx_nullable;
  new_sink->output = output;
  new_sink->flush = flush;
  new_sink->destroy_ctx = destory_ctx_nullable;
  new_sink->auto_destory = xFALSE;

  return new_sink;
}

xlog_sink xlog_sink_dup(xlog_sink other) {
  if (other == NULL) return NULL;

  return xlog_sink_new(other->ctx, other->output, other->flush);
}

err_t xlog_sink_set_output(xlog_sink self, sink_output_func func) {
  if (self == NULL || func == NULL) return X_RET_INVAL;

  self->output = func;

  return X_RET_OK;
}

err_t xlog_sink_set_flush(xlog_sink self, sink_flush_func func) {
  if (self == NULL || func == NULL) return X_RET_INVAL;

  self->flush = func;

  return X_RET_OK;
}

err_t xlog_sink_set_destroy_ctx(xlog_sink self, sink_ctx_destroy_func func) {
  if (self == NULL || func == NULL) return X_RET_INVAL;

  self->destroy_ctx = func;

  return X_RET_OK;
}

err_t xlog_sink_set_ctx(xlog_sink self, void* ctx) {
  if (self == NULL) return X_RET_INVAL;

  self->ctx = ctx;

  return X_RET_OK;
}

void* xlog_sink_ctx(xlog_sink self) {
  if (self == NULL) return NULL;

  return self->ctx;
}

static err_t xlog_sink_del_advance(xlog_sink self, xbool_t force) {
  if (self == NULL) return X_RET_INVAL;

  if (force == xTRUE) {
    goto cleanup;
  }

  if (self->auto_destory == xFALSE) goto cleanup;

  return X_RET_OK;
cleanup:
  if (self->destroy_ctx) self->destroy_ctx(self);

  xbox_free(self);

  return X_RET_OK;
}

err_t xlog_sink_del(xlog_sink self) {
  return xlog_sink_del_advance(self, xFALSE);
}

xlog_message_t xlog_message_init(const char* module,
                                 const char* file,
                                 const char* func,
                                 int line,
                                 xlog_lvl_e lvl,
                                 const char* format,
                                 ...) {
  va_list ap, ap_copy;
  va_start(ap, format);
  va_copy(ap_copy, ap);
  xlog_message_t message = {
      .module = module,
      .full_file_name = file,
      .file_name = xlog_get_basename(file),
      .func = func,
      .line = line,
      .lvl = lvl,
      .data =
          {
              .b = {0},
              .s = NULL,
          },
      .need_free = xFALSE,
  };

  int need_len = stbsp_vsnprintf(NULL, 0, format, ap_copy) + 1;  // reserve '\0'
  va_end(ap_copy);

  if (need_len > XLOG_SBO_SIZE) {
    message.data.s = xbox_malloc(need_len);
    assert(message.data.s != NULL);
    stbsp_vsnprintf(message.data.s, need_len, format, ap);
  } else {
    stbsp_vsnprintf(message.data.b, XLOG_SBO_SIZE, format, ap);
  }

  va_end(ap);

  return message;
}

xlog_message_t* xlog_message_dup(const xlog_message_t* other) {
  if (other == NULL) return NULL;

  xlog_message_t* message = xbox_malloc(sizeof(xlog_message_t));
  if (message == NULL) return NULL;

  message->module = other->module;
  message->full_file_name = other->full_file_name;
  message->file_name = other->file_name;
  message->func = other->func;
  message->line = other->line;
  message->lvl = other->lvl;
  memset(&message->data.b, 0, XLOG_SBO_SIZE);
  message->data.s = NULL;
  message->need_free = xTRUE;

  if (other->data.s) {
    message->data.s = xbox_strdup(other->data.s);
    assert(message->data.s != NULL);
  } else {
    memcpy(message->data.b, other->data.b, XLOG_SBO_SIZE);
  }

  return message;
}

err_t xlog_message_release(xlog_message_t* message) {
  if (message == NULL) return X_RET_INVAL;

  if (message->data.s) {
    xbox_free(message->data.s);
    message->data.s = NULL;
  }

  if (message->need_free) xbox_free(message);

  return X_RET_OK;
}

void xlog_hexdump(const char* prefix, const uint8_t* buf, size_t len) {
  size_t ___i, ___j, ___k;
  char* ___data = (char*)buf;

  if (prefix != NULL && prefix[0] != '\0') printf("%s\n", prefix);

  for (___i = 0; ___i < len; ___i += 16) {
    printf("%.8x  ", (uint16_t)___i);
    for (___j = ___i; ___j < ___i + 8 && ___j < len; ___j++)
      printf("%.2x ", (uint8_t)___data[___j]);
    printf(" ");
    for (; ___j < ___i + 16 && ___j < len; ___j++)
      printf("%.2x ", (uint8_t)___data[___j]);
    if (___j % 16) {
      for (___k = 0; ___k < 16 - (___j % 16); ___k++) printf("   ");
    }
    printf(" |");
    for (___j = ___i; ___j < ___i + 16 && ___j < len; ___j++)
      printf("%c",
             (___data[___j] >= ' ' && ___data[___j] <= '~')
                 ? (uint8_t)___data[___j]
                 : '.');
    printf("|\n");
  }
}

xlog_options_t xlog_options_init(xlog_lvl_e lvl, xlog_lvl_e which_flush) {
  xlog_options_t opt;
  opt.lvl = lvl;
  opt.which_flush = which_flush;
  opt.ctx = NULL;
  opt.redirect = NULL;
  return opt;
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((constructor)) void xlog_check_env() {
  char* lvl = getenv("XLOG_LVL");
  if (!lvl) return;

  if (!strcasecmp(lvl, "trace"))
    xlog_logger_default_instance.lvl = XLOG_LVL_TRACE;
  else if (!strcasecmp(lvl, "debug"))
    xlog_logger_default_instance.lvl = XLOG_LVL_DEBUG;
  else if (!strcasecmp(lvl, "info"))
    xlog_logger_default_instance.lvl = XLOG_LVL_INFO;
  else if (!strcasecmp(lvl, "warn"))
    xlog_logger_default_instance.lvl = XLOG_LVL_WARN;
  else if (!strcasecmp(lvl, "error"))
    xlog_logger_default_instance.lvl = XLOG_LVL_ERROR;
  else if (!strcasecmp(lvl, "fatal"))
    xlog_logger_default_instance.lvl = XLOG_LVL_FATAL;
}

#include <time.h>
static inline uint64_t __now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ull + ts.tv_nsec;
}
void xlog_default_output(xlogger lgr,
                         xlog_sink s,
                         const xlog_message_t* const msg) {
  uint64_t ns = __now_ns();
  time_t sec = ns / 1000000000ull;
  int ms = (int)((ns / 1000000ull) % 1000);
  struct tm tm;
  localtime_r(&sec, &tm);

  static const char* lvl_color[] = {
      [XLOG_LVL_TRACE] = "\x1b[36m",
      [XLOG_LVL_DEBUG] = "\x1b[37m",
      [XLOG_LVL_INFO] = "\x1b[32m",
      [XLOG_LVL_WARN] = "\x1b[33m",
      [XLOG_LVL_ERROR] = "\x1b[91m",
      [XLOG_LVL_FATAL] = "\x1b[91;1m",
  };

  fprintf(stderr,
          "%s[%d-%02d-%02d %02d:%02d:%02d.%03d] [%s] %s[%s:%d] %s\x1b[0m\n",
          lvl_color[msg->lvl],
          tm.tm_year + 1900,
          tm.tm_mon + 1,
          tm.tm_mday,
          tm.tm_hour,
          tm.tm_min,
          tm.tm_sec,
          ms,
          xlog_lvl_str(msg->lvl),
          lvl_color[msg->lvl],
          xlog_message_file(msg),
          xlog_message_line(msg),
          xlog_message_data(msg));
}

void xlog_default_flush(xlogger logger, xlog_sink sinker) {
  xUNUSED(logger);
  xUNUSED(sinker);
}
#else
void xlog_default_output(xlogger lgr,
                         xlog_sink s,
                         const xlog_message_t* const msg) {
  xUNUSED(lgr);
  xUNUSED(s);

  if (msg->lvl <= XLOG_LVL_DEBUG) {
    printf("[%s] [%s:%d] %s\n",
           xlog_lvl_str(msg->lvl),
           xlog_message_file(msg),
           xlog_message_line(msg),
           xlog_message_data(msg));
  } else {
    printf("[%s] %s\n", xlog_lvl_str(msg->lvl), xlog_message_data(msg));
  }
}

void xlog_default_flush(xlogger logger, xlog_sink sinker) {
  xUNUSED(logger);
  xUNUSED(sinker);
}
#endif /* __GNUC__ || __clang__ */
