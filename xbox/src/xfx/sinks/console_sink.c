/**
 * @brief 终端接收器
 * @file console_sink.c
 * @author Oswin
 * @date 2025-07-12
 * @details 终端接收器
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "console_sink.h"

#include <string.h>

#include "os_time.h"

// printf(PRIblod("%s %.10s") " [%s%s%s] %s\n", ...);
#define PRIblod(x) "\033[1m" x "\033[0m"

static void _output(xlogger, xlog_sink, const xlog_message_t* const);
static void _output_color(xlogger, xlog_sink, const xlog_message_t* const);
static void _flush(xlogger, xlog_sink);
static void _destory(xlog_sink);
static const char* _lvl_string(xlog_lvl_e);
static const char* _lvl_color(xlog_lvl_e);
static const char* _lvl_color_reset(void);

xlog_sink console_sink_new(FILE* dst, xbool_t color) {
  return xlog_sink_new_with_destory(dst,
                                    color ? _output_color : _output,
                                    _flush,
                                    _destory);
}

static void _output(xlogger logger,
                    xlog_sink sink,
                    const xlog_message_t* const message) {
  FILE* dst = xlog_sink_ctx(sink);
  if (dst == NULL) return;

  char time_buf[64] = {0};
  os_time_string_with_ms(time_buf, sizeof(time_buf));

  const char* module = xlog_message_module(message);
  xbool_t has_module = strlen(message->module) > 0;
  const char* module_left = has_module ? " " : "";
  const char* module_right = has_module ? " > " : " > ";

  fprintf(dst,
          "%s [%s]%s%s%s%s\n",
          time_buf,
          _lvl_string(message->lvl),
          module_left,
          module,
          module_right,
          xlog_message_data(message));
}

static void _output_color(xlogger logger,
                          xlog_sink sink,
                          const xlog_message_t* const message) {
  FILE* dst = xlog_sink_ctx(sink);
  if (dst == NULL) return;

  char time_buf[64] = {0};
  os_time_string_with_ms(time_buf, sizeof(time_buf));

  const char* module = xlog_message_module(message);
  xbool_t has_module = strlen(module) > 0;
  const char* module_left = has_module ? " \033[1m" : "";
  const char* module_right = has_module ? "\033[0m > " : " > ";

  fprintf(dst,
          "%s [%s%s%s]%s%s%s%s\n",
          time_buf,
          _lvl_color(message->lvl),
          _lvl_string(message->lvl),
          _lvl_color_reset(),
          module_left,
          module,
          module_right,
          xlog_message_data(message));
}

static void _flush(xlogger logger, xlog_sink sink) {
  xUNUSED(logger);

  FILE* fp = xlog_sink_ctx(sink);

  if (fp == NULL) return;

#ifdef _WIN32
  fflush(fp);
#elif __linux__
  fflush(fp);
#elif __APPLE__
  fflush(fp);
#endif
}

static void _destory(xlog_sink sink) { _flush(NULL, sink); }

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
      return "TRACE";
    case XLOG_LVL_DEBUG:
      return "DEBUG";
    case XLOG_LVL_INFO:
      return "INFO";
    case XLOG_LVL_WARN:
      return "WARN";
    case XLOG_LVL_ERROR:
      return "ERROR";
    case XLOG_LVL_FATAL:
      return "FATAL";
    default:
      return "UNK";
  }
#else
  return xlog_lvl_str(lvl);
#endif
}
