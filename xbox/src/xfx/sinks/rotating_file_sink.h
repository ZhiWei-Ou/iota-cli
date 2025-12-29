/**
 * @brief 滚动文件接收器
 * @file rotating_file_sink.h
 * @author Oswin
 * @date 2025-07-12
 * @details Provides a log sink that writes to a file and rotates it when it
 *           reaches a maximum size.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef XFRAMEWORK_ROTATING_FILE_SINK_H_
#define XFRAMEWORK_ROTATING_FILE_SINK_H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "xlog.h"

/**
 * @brief Configuration options for the rotating file sink.
 */
// clang-format off
typedef struct {
  char* file; /**< The base path for the log file. */
  size_t max_size; /**< The maximum size of a single log file in kilobytes (KB) before rotation. */
  size_t backup; /**< The maximum number of backup files to keep (e.g., log.1, log.2, ...). */
  xbool_t color;    /**< This option is typically ignored for file sinks. */
  xbool_t json_fmt; /**< If true, log messages are written in JSON format. */
} rotating_file_options_t;
// clang-format on

/**
 * @brief Creates a new rotating file log sink.
 * @details When the log file reaches `max_size`, it is renamed (e.g., `log.txt` -> `log.1.txt`),
 *          and a new empty log file is created. This process continues up to
 * the `backup` limit.
 * @param option A pointer to the options struct that configures the sink's behavior.
 * @return A handle to the new log sink (`xlog_sink`), or NULL on failure.
 */
xlog_sink rotating_file_sink(rotating_file_options_t* option);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* XFRAMEWORK_ROTATING_FILE_SINK_H_ */
