/**
 * @brief Linux systemd journal 接收器
 * @file systemd_sink.c
 * @author Oswin
 * @date 2025-11-05
 * @details A log sink for writing log messages to the Linux systemd journal.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "systemd_sink.h"

#include <systemd/sd-journal.h>

static void _output(xlogger, xlog_sink, const xlog_message_t* const);
static void _flush(xlogger, xlog_sink);
static void _destory(xlog_sink);
static int _lvl_convert(xlog_lvl_e);

xlog_sink systemd_sink(const char* app) {
  return xlog_sink_new_with_destory(xbox_strdup(app),
                                    _output,
                                    _flush,
                                    _destory);
}

static void _output(xlogger logger,
                    xlog_sink sink,
                    const xlog_message_t* const message) {
  char* app = xlog_sink_ctx(sink);

  sd_journal_send("MESSAGE=%s",
                  xlog_message_data(message),
                  "PRIORITY=%d",
                  _lvl_convert(message->lvl),
                  "SYSLOG_IDENTIFIER=%s",
                  app,
                  "CODE_FILE=%s",
                  message->file_name,
                  "CODE_LINE=%d",
                  message->line,
                  "CODE_FUNC=%s",
                  message->func,
                  NULL);
}

static void _flush(xlogger logger, xlog_sink sink) {
  xUNUSED(logger);
  xUNUSED(sink);
}

static void _destory(xlog_sink sink) {
  char* app = xlog_sink_ctx(sink);
  if (app) xbox_free(app);
}

static int _lvl_convert(xlog_lvl_e lvl) {
  switch (lvl) {
    case XLOG_LVL_TRACE:
      return LOG_DEBUG;
    case XLOG_LVL_DEBUG:
      return LOG_DEBUG;
    case XLOG_LVL_INFO:
      return LOG_INFO;
    case XLOG_LVL_WARN:
      return LOG_WARNING;
    case XLOG_LVL_ERROR:
      return LOG_ERR;
    case XLOG_LVL_FATAL:
      return LOG_CRIT;
    default:
      return LOG_INFO;
  }
}
