/**
 * @brief RT-Thread ulog 接收器
 * @file rtthread_ulog_sink.h
 * @author Oswin
 * @date 2025-11-26
 * @details xlog sink for RT-Thread's ulog
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef XFRAMEWORK_SINKS_RTTHREAD_ULOG_SINK__H_
#define XFRAMEWORK_SINKS_RTTHREAD_ULOG_SINK__H_

#include "xlog.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates a sink that forwards log messages to the RT-Thread ulog system.
 *
 * @return A new xlog_sink instance.
 */
xlog_sink rtthread_ulog_sink();

#ifdef __cplusplus
}
#endif

#endif /* XFRAMEWORK_SINKS_RTTHREAD_ULOG_SINK__H_ */
