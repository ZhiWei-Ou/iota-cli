/**
 * @brief Linux syslog 接收器
 * @file syslog_sink.c
 * @author Oswin
 * @date 2025-11-05
 * @details Provides a log sink that writes log messages to the Linux syslog service.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "syslog_sink.h"

#include <syslog.h>
#include <unistd.h>

static void _output(xlogger, xlog_sink, const xlog_message_t* const);
static void _flush(xlogger, xlog_sink);
static void _destory(xlog_sink);

xlog_sink syslog_sink(const char* app) {
  openlog(app, LOG_PID | LOG_CONS, LOG_USER);

  return xlog_sink_new_with_destory(NULL, _output, _flush, _destory);
}

static void _output(xlogger logger,
                    xlog_sink sink,
                    const xlog_message_t* const message) {
  switch (message->lvl) {
    case XLOG_LVL_TRACE:
    case XLOG_LVL_DEBUG:
      syslog(LOG_DEBUG, "%s", xlog_message_data(message));
      break;

    case XLOG_LVL_INFO:
      syslog(LOG_INFO, "%s", xlog_message_data(message));
      break;

    case XLOG_LVL_WARN:
      syslog(LOG_WARNING, "%s", xlog_message_data(message));
      break;

    case XLOG_LVL_ERROR:
      syslog(LOG_ERR, "%s", xlog_message_data(message));
      break;

    case XLOG_LVL_FATAL:
      syslog(LOG_CRIT, "%s", xlog_message_data(message));
      break;

    default:
      syslog(LOG_INFO, "%s", xlog_message_data(message));
      break;
  }
}

static void _flush(xlogger logger, xlog_sink sink) {
  xUNUSED(logger);
  xUNUSED(sink);
}

static void _destory(xlog_sink sink) {
  xUNUSED(sink);

  closelog();
}
