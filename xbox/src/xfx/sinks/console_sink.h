/**
 * @brief 终端接收器
 * @file console_sink.h
 * @author Oswin
 * @date 2025-07-12
 * @details Provides log sinks for writing log messages to the console (stdout/stderr).
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef XFRAMEWORK_CONSOLE_SINK_H_
#define XFRAMEWORK_CONSOLE_SINK_H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdio.h>

#include "xlog.h"

/** @brief Creates a sink that writes to stdout without color. */
#define stdout_sink() console_sink_new(stdout, xFALSE)
/** @brief Creates a sink that writes to stdout with ANSI color codes. */
#define stdout_color_sink() console_sink_new(stdout, xTRUE)
/** @brief Creates a sink that writes to stderr without color. */
#define stderr_sink() console_sink_new(stderr, xFALSE)
/** @brief Creates a sink that writes to stderr with ANSI color codes. */
#define stderr_color_sink() console_sink_new(stderr, xTRUE)
/** @brief Creates a null sink that discards all log messages. */
#define null_sink() console_sink_new(NULL, xFALSE)

/**
 * @brief Creates a new console log sink.
 * @param dst The destination stream. Can be `stdout`, `stderr`, or `NULL`.
 *            If `NULL`, a null sink is created which discards all messages.
 * @param color If true, the output will be formatted with ANSI color codes based on log level.
 * @return A handle to the new log sink (`xlog_sink`), or NULL on failure.
 */
xlog_sink console_sink_new(FILE* dst, xbool_t color);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* XFRAMEWORK_CONSOLE_SINK_H_ */
