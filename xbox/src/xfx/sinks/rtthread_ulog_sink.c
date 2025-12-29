/**
 * @brief RT-Thread ulog 接收器
 * @file rtthread_ulog_sink.c
 * @author Oswin
 * @date 2025-11-26
 * @details xlog sink for RT-Thread's ulog
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "sinks/rtthread_ulog_sink.h"

#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>

// Forward declarations for sink functions
static void _ulog_output_fn(xlogger logger,
                            xlog_sink sink,
                            const xlog_message_t* const message);
static void _ulog_flush_fn(xlogger logger, xlog_sink sink);

xlog_sink rtthread_ulog_sink() {
  // Create the sink using the xlog factory function
  return xlog_sink_new(NULL, _ulog_output_fn, _ulog_flush_fn);
}

// The actual sink output function
static void _ulog_output_fn(xlogger logger,
                            xlog_sink sink,
                            const xlog_message_t* const message) {
  (void)logger;
  (void)sink;

  // The ulog macros (ulog_d, ulog_i, etc.) are the standard way to log.
  // They take the tag and a printf-style format string. We will use the
  // message's module as the tag.
  switch (message->lvl) {
    case XLOG_LVL_FATAL:
      // Prepend "FATAL" to the message for clarity as ulog has no FATAL level
      ulog_e(message->module, "FATAL: %s", message->msg);
      break;
    case XLOG_LVL_ERROR:
      ulog_e(message->module, "%s", message->msg);
      break;
    case XLOG_LVL_WARN:
      ulog_w(message->module, "%s", message->msg);
      break;
    case XLOG_LVL_INFO:
      ulog_i(message->module, "%s", message->msg);
      break;
    case XLOG_LVL_DEBUG:
    case XLOG_LVL_TRACE:
      // ulog's lowest level is DEBUG. Map both TRACE and DEBUG to it.
      ulog_d(message->module, "%s", message->msg);
      break;
    default:
      ulog_i(message->module, "%s", message->msg);
      break;
  }
}

// ulog does not have a manual flush mechanism.
static void _ulog_flush_fn(xlogger logger, xlog_sink sink) {
  (void)logger;
  (void)sink;
}
