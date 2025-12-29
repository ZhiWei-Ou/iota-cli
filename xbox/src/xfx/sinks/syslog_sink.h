/**
 * @brief Linux syslog 接收器
 * @file syslog_sink.h
 * @author Oswin
 * @date 2025-11-05
 * @details Provides a log sink that writes log messages to the Linux syslog service.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef XFRAMEWORK_SYSLOG_SINK_H_
#define XFRAMEWORK_SYSLOG_SINK_H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "xlog.h"

/**
 * @brief Creates a new log sink that sends messages to the syslog daemon.
 * @param app The application name (ident) that will appear in syslog entries.
 *            If NULL, a default name may be used.
 * @return A handle to the new log sink (`xlog_sink`), or NULL on failure.
 */
xlog_sink syslog_sink(const char* app);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* XFRAMEWORK_SYSLOG_SINK_H_ */
