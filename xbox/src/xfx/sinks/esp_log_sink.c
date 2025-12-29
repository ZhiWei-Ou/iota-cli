/**
 * @brief Esp-IDF log 接收器
 * @file esp_log_sink.c
 * @author Oswin
 * @date 2025-11-26
 * @details Provides an ESP-LOG compatible sink for the logging framework.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "esp_log_sink.h"

#include <stdlib.h>
#include <string.h>

#include "esp_log.h"

static void _output(xlogger, xlog_sink, const xlog_message_t* const);
static void _flush(xlogger, xlog_sink);
static void _destory(xlog_sink);

xlog_sink esp_log_sink() {
  /* set esp log level to minimal.
   * xlog will take over the log level control */
  esp_log_level_set("*", ESP_LOG_VERBOSE);
  return xlog_sink_new_with_destory(NULL, _output, _flush, _destory);
}

static void _output(xlogger __l,
                    xlog_sink __s,
                    const xlog_message_t* const __m) {
  switch (__m->lvl) {
    case XLOG_LVL_TRACE:
      ESP_LOGV(__m->module, "%s", __m->msg);
      break;
    case XLOG_LVL_DEBUG:
      ESP_LOGD(__m->module, "%s", __m->msg);
      break;
    case XLOG_LVL_INFO:
      ESP_LOGI(__m->module, "%s", __m->msg);
      break;
    case XLOG_LVL_WARN:
      ESP_LOGW(__m->module, "%s", __m->msg);
      break;
    case XLOG_LVL_ERROR:
      ESP_LOGE(__m->module, "%s", __m->msg);
      break;
    case XLOG_LVL_FATAL:
      ESP_LOGE(__m->module, "FATAL: %s", __m->msg);
      break;
    default:
      ESP_LOGI(__m->module, "%s", __m->msg);
      break;
  }
}
static void _flush(xlogger __l, xlog_sink __s) {
  // ESP-LOG does not have a flush mechanism, so this is a no-op.
  xUNUSED(__l);
  xUNUSED(__s);
}
static void _destory(xlog_sink __s) { xUNUSED(__s); }
