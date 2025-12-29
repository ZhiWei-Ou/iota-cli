/**
 * @brief Esp-IDF log 接收器
 * @file esp_log_sink.h
 * @author Oswin
 * @date 2025-11-26
 * @details Provides an ESP-LOG compatible sink for the logging framework.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef XFRAMEWORK_ESP_LOG_SINK_H_
#define XFRAMEWORK_ESP_LOG_SINK_H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "xlog.h"

/**
 * @brief Creates a new log sink that outputs log messages using ESP-LOG.
 * @return A handle to the new log sink (`xlog_sink`), or NULL on failure.
 */
xlog_sink esp_log_sink();

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* XFRAMEWORK_ESP_LOG_SINK_H_ */
