/**
 * @brief 中间件接收器
 * @file broker_sink.h
 * @author Oswin
 * @date 2025-11-05
 * @details Provides a log sink that publishes log messages to a message broker.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef XFRAMEWORK_BROKER_SINK_H_
#define XFRAMEWORK_BROKER_SINK_H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "../broker.h"
#include "xlog.h"

/**
 * @brief Creates a new log sink that publishes log messages to a broker topic.
 * @param b The handle to the broker instance that will receive the log messages.
 * @param topic The topic on the broker where log messages will be published.
 * @return A handle to the new log sink (`xlog_sink`), or NULL on failure.
 */
xlog_sink broker_sink(broker b, const char* topic);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* XFRAMEWORK_BROKER_SINK_H_ */
