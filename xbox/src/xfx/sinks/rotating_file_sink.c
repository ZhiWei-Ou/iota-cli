/**
 * @brief 滚动文件接收器
 * @file rotating_file_sink.c
 * @author Oswin
 * @date 2025-07-12
 * @details 滚动文件接收器
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "rotating_file_sink.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "basic_file_sink.h"
#include "os_file.h"
#include "os_time.h"

typedef struct {
  basic_file_sink_context_t* basic_file_sink_;
  rotating_file_options_t opts_;
} rotating_file_sink_context;

static const char* _lvl_string(xlog_lvl_e);
static const char* _lvl_color(xlog_lvl_e);
static const char* _lvl_color_reset(void);
static char* json_escape_string(const char* s);
static void _output(xlogger, xlog_sink, const xlog_message_t* const);
static void _flush(xlogger, xlog_sink);
static void _destory(xlog_sink);
static void _rotate_if_needed(rotating_file_sink_context* ctx);

xlog_sink rotating_file_sink(rotating_file_options_t* option) {
  if (option == NULL || option->file == NULL) return NULL;

  rotating_file_sink_context* ctx = xbox_malloc(
      sizeof(rotating_file_sink_context));
  if (ctx == NULL) return NULL;

  ctx->basic_file_sink_ = NULL;
  ctx->opts_.file = xbox_strdup(option->file);
  ctx->opts_.max_size = option->max_size * 1024;  // KB to Bytes
  ctx->opts_.backup = option->backup;
  ctx->opts_.color = option->color;
  ctx->opts_.json_fmt = option->json_fmt;
  if (ctx->opts_.json_fmt) ctx->opts_.color = xFALSE;

  xlog_sink sink = basic_file_sink(option->file);
  if (sink == NULL) {
    xbox_free(ctx);
    return NULL;
  }

  ctx->basic_file_sink_ = xlog_sink_ctx(sink);

  xlog_sink_set_ctx(sink, ctx);
  xlog_sink_set_output(sink, _output);
  xlog_sink_set_flush(sink, _flush);
  xlog_sink_set_destroy_ctx(sink, _destory);

  return sink;
}

static char* json_escape_string(const char* s) {
  if (!s) return NULL;
  const unsigned char* p;
  size_t needed = 0;

  for (p = (const unsigned char*)s; *p; ++p) {
    unsigned char c = *p;
    switch (c) {
      case '"':
      case '\\':
      case '\b':
      case '\f':
      case '\n':
      case '\r':
      case '\t':
        needed += 2;
        break;
      default:
        if (c < 0x20)
          needed += 6; /* \u00XX */
        else
          needed += 1;
    }
  }

  char* out = xbox_malloc(needed + 1);
  if (!out) return NULL;
  char* q = out;

  for (p = (const unsigned char*)s; *p; ++p) {
    unsigned char c = *p;
    switch (c) {
      case '"':
        *q++ = '\\';
        *q++ = '"';
        break;
      case '\\':
        *q++ = '\\';
        *q++ = '\\';
        break;
      case '\b':
        *q++ = '\\';
        *q++ = 'b';
        break;
      case '\f':
        *q++ = '\\';
        *q++ = 'f';
        break;
      case '\n':
        *q++ = '\\';
        *q++ = 'n';
        break;
      case '\r':
        *q++ = '\\';
        *q++ = 'r';
        break;
      case '\t':
        *q++ = '\\';
        *q++ = 't';
        break;
      default:
        if (c < 0x20) {
          int n = sprintf(q, "\\u%04X", (unsigned int)c);
          q += n;
        } else {
          *q++ = c;
        }
    }
  }
  *q = '\0';
  return out;
}

static void _output(xlogger logger,
                    xlog_sink sink,
                    const xlog_message_t* const message) {
  rotating_file_sink_context* ctx = xlog_sink_ctx(sink);

  char time_buf[64] = {0};
  os_time_string3(time_buf, sizeof(time_buf));

  const char* module = xlog_message_module(message);
  xbool_t has_module = strlen(module) > 0;
  const char* module_left = has_module ? " \033[1m" : "";
  const char* module_right = has_module ? "\033[0m > " : " > ";

  const char* color_start = ctx->opts_.color ? _lvl_color(message->lvl) : "";
  const char* color_end = ctx->opts_.color ? _lvl_color_reset() : "";

  os_mutex_lock(ctx->basic_file_sink_->mtx);

  assert(ctx->basic_file_sink_->fp != NULL);

  if (ctx->opts_.json_fmt) {
    const char* mod = xlog_message_module(message);
    const char* file = xlog_message_file(message);
    const char* func = xlog_message_function(message);
    const char* msg = xlog_message_data(message);

    char* mod_e = json_escape_string(mod ? mod : "");
    char* file_e = json_escape_string(file ? file : "");
    char* func_e = json_escape_string(func ? func : "");
    char* msg_e = json_escape_string(msg ? msg : "");

    fprintf(ctx->basic_file_sink_->fp,
            "{\"time\":\"%s\",\"level\":\"%s\",\"module\":\"%s\",\"file\":\"%"
            "s\",\"func\":\"%s\",\"line\":%d,\"message\":\"%s\"}\n",
            time_buf,
            _lvl_string(message->lvl),
            mod_e,
            file_e,
            func_e,
            xlog_message_line(message),
            msg_e);

    xbox_free(mod_e);
    xbox_free(file_e);
    xbox_free(func_e);
    xbox_free(msg_e);
  } else {
    fprintf(ctx->basic_file_sink_->fp,
            "%s [%s%s%s]%s%s%s%s\n",
            time_buf,
            color_start,
            _lvl_string(message->lvl),
            color_end,
            module_left,
            module,
            module_right,
            xlog_message_data(message));
  }

  _rotate_if_needed(ctx);

  os_mutex_unlock(ctx->basic_file_sink_->mtx);
}

static void _flush(xlogger logger, xlog_sink sink) {
  rotating_file_sink_context* ctx = xlog_sink_ctx(sink);

  os_mutex_lock(ctx->basic_file_sink_->mtx);
  fflush(ctx->basic_file_sink_->fp);
  os_mutex_unlock(ctx->basic_file_sink_->mtx);
}

static void _rotate_if_needed(rotating_file_sink_context* ctx) {
  size_t file_size = os_file_size(ctx->opts_.file);
  if (file_size < ctx->opts_.max_size) return;

  size_t current_file_name_lenght = strlen(ctx->opts_.file);
#define __ROTATING_FILE_NAME_RESERVE_BYTES__ 10
  char
      old_name[current_file_name_lenght + __ROTATING_FILE_NAME_RESERVE_BYTES__];
  char
      new_name[current_file_name_lenght + __ROTATING_FILE_NAME_RESERVE_BYTES__];

  // remove the oldest backup file
  snprintf(old_name,
           sizeof(old_name),
           "%s.%zu",
           ctx->opts_.file,
           ctx->opts_.backup);
  os_remove(old_name);

  // rotate backward
  for (size_t i = ctx->opts_.backup; i > 1; --i) {
    snprintf(old_name, sizeof(old_name), "%s.%zu", ctx->opts_.file, i - 1);
    snprintf(new_name, sizeof(new_name), "%s.%zu", ctx->opts_.file, i);
    os_rename(old_name, new_name);
  }

  // rename current log file to X.1
  snprintf(new_name, sizeof(new_name), "%s.1", ctx->opts_.file);
  os_rename(ctx->opts_.file, new_name);

  // reopen the log file
  os_file_close(ctx->basic_file_sink_->fp);
  ctx->basic_file_sink_->fp = os_file_open(ctx->opts_.file, "w");
}

static void _destory(xlog_sink sink) {
  rotating_file_sink_context* ctx = xlog_sink_ctx(sink);
  if (ctx == NULL) return;

  if (ctx->opts_.file) xbox_free(ctx->opts_.file);

  if (ctx->basic_file_sink_) {
    if (ctx->basic_file_sink_->fp) os_file_close(ctx->basic_file_sink_->fp);

    os_mutex_destroy(ctx->basic_file_sink_->mtx);

    xbox_free(ctx->basic_file_sink_);
  }

  xbox_free(ctx);
}

const char* _lvl_color(xlog_lvl_e lvl) {
  switch (lvl) {
    case XLOG_LVL_TRACE:
      return "\033[37m";
    case XLOG_LVL_DEBUG:
      return "\033[36m";
    case XLOG_LVL_INFO:
      return "\033[32m";
    case XLOG_LVL_WARN:
      return "\033[33m\033[1m" /*yellow-bold*/;
    case XLOG_LVL_ERROR:
      return "\033[31m\033[1m" /*red-bold*/;
    case XLOG_LVL_FATAL:
      return "\033[1m\033[41m";
    default:
      return "";
  }
}

const char* _lvl_color_reset(void) { return "\033[0m"; }

static const char* _lvl_string(xlog_lvl_e lvl) {
#if 1
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
      return "UNK";
  }
#else
  return xlog_lvl_str(lvl);
#endif
}
