/**
 * @brief 日志组件
 * @file log.h
 * @author Oswin
 * @date 2025-07-09
 * @details Provides a high-level configuration and lifecycle management API
 *           for the application-wide logging system. This builds upon the
 *           lower-level xlog library to provide a simple init/shutdown
 * mechanism.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef XFRAMEWORK_LOG__H_
#define XFRAMEWORK_LOG__H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "xdef.h"
#include "xjson.h"
#include "xlog.h"

/**
 * @brief The maximum number of log messages that can be queued when using asynchronous logging.
 */
#define LOG_ASYNC_QUEUE_MAX_SIZE (128)

// clang-format off
/**
 * @brief Holds all configuration options for the logging system.
 */
typedef struct {
  char level[8];   /**< Log level, e.g., "trace", "debug", "info", "warn", "error", "fatal". Default is "info". */ 
  char file[256];  /**< Path to the log file. If empty, logs to console. Default is empty. */
  xbool_t color;   /**< Enables or disables colorized console output. Default is true. */
  xbool_t quiet;   /**< Suppresses all log output if true. Default is false. */
  size_t max_size; /**< Maximum size of a single log file in megabytes before rotation. Default is 10 MB. */
  size_t backup;   /**< Maximum number of backup log files to keep. Default is 3. */
  xbool_t json;    /**< Toggles JSON-formatted output for file logging. Default is false. */
  xbool_t async;   /**< Enables or disables asynchronous logging. Default is false. */
} log_config_t;
// clang-format on

/**
 * @brief Deserializes a logging configuration from a JSON object.
 * @param root The JSON object containing the configuration.
 * @param[out] config The log_config_t struct to populate.
 * @return X_RET_OK  on success.
 */
err_t log_config_unmarshal(const xjson root, log_config_t* config);

/**
 * @brief Gets a log_config_t struct with default values.
 * @return A log_config_t struct initialized with default settings.
 */
log_config_t default_log_config(void);

/**
 * @brief Initializes the global logging system with the given configuration.
 * @details This function sets up the appropriate log sinks (e.g., console, file)
 *          and formatters based on the configuration and replaces the global
 *          xlog instance. This should be called once on application startup.
 * @param conf A pointer to the logging configuration.
 * @return X_RET_OK  on success.
 */
err_t log_init(const log_config_t* conf);

/**
 * @brief Shuts down the logging system.
 * @details This function flushes all pending log messages and releases any
 *          resources allocated by `log_init`. It should be called once
 *          on application exit.
 * @return X_RET_OK  on success.
 */
err_t log_fini();

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* XFRAMEWORK_LOG__H_ */
