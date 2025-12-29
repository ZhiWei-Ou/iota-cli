/**
 * @brief 文件接收器
 * @file basic_file_sink.h
 * @author Oswin
 * @date 2025-11-04
 * @details Provides a basic log sink that writes log messages to a single file.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef XFRAMEWORK_BASIC_FILE_SINK_H_
#define XFRAMEWORK_BASIC_FILE_SINK_H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "os_file.h"
#include "os_mutex.h"
#include "xlog.h"

/**
 * @internal
 * @brief The internal context for a basic file sink.
 * @details Holds the file pointer and a mutex for thread-safe writes.
 */
typedef struct {
  FILE* fp;     /**< The file pointer to the log file. */
  os_mutex mtx; /**< Mutex to ensure thread-safe access to the file. */
} basic_file_sink_context_t;

/**
 * @brief Creates a new log sink that writes to a specified file.
 * @details The file is opened in "append" mode. If the file does not exist, it will be created.
 * @param path The full path to the log file.
 * @return A handle to the new log sink (`xlog_sink`), or NULL on failure (e.g., cannot open file).
 */
xlog_sink basic_file_sink(const char* path);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* XFRAMEWORK_BASIC_FILE_SINK_H_ */
