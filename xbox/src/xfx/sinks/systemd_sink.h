/**
 * @brief Linux systemd journal 接收器
 * @file systemd_sink.h
 * @author Oswin
 * @date 2025-11-05
 * @details Provides a log sink that writes log messages to the Linux systemd journal.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef XFRAMEWORK_SYSTEMD_SINK_H_
#define XFRAMEWORK_SYSTEMD_SINK_H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "xlog.h"

/**
 * @brief Creates a new log sink that sends messages to the systemd journal daemon.
 * @param app The application name, which will be used as the `SYSLOG_IDENTIFIER`
 *            in the journal entry. If NULL, a default may be used.
 * @return A handle to the new log sink (`xlog_sink`), or NULL on failure.
 */
xlog_sink systemd_sink(const char* app);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* XFRAMEWORK_SYSTEMD_SINK_H_ */
