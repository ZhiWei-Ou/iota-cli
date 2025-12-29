/**
 * @brief 结构化日志框架
 * @file xlog.h
 * @author Oswin
 * @date 2025-06-25
 * @details A powerful, lightweight logging module for C.
 * @see examples in `test/unix/xlog_test.cpp`
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef XTOOL_XLOG__H_
#define XTOOL_XLOG__H_

#include "xdef.h"
#include "xstring.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @def XLOG_MOD
 * @brief Defines the module name for the log message.
 */
#ifndef XLOG_MOD
#define XLOG_MOD ""
#endif /* XLOG_MOD */

// clang-format off

/**
 * @defgroup GlobalLoggerMacros Global Logger Macros
 * @brief Macros for logging with the default global logger instance.
 * @{
 */
/** @brief Logs a trace message using the global logger. */
#define XLOG_T(fmt, ...) XLOG_OUTPUT_MESSAGE(xlog_global_instance(), XLOG_LVL_TRACE, fmt, ##__VA_ARGS__)
/** @brief Logs a debug message using the global logger. */
#define XLOG_D(fmt, ...) XLOG_OUTPUT_MESSAGE(xlog_global_instance(), XLOG_LVL_DEBUG, fmt, ##__VA_ARGS__)
/** @brief Logs an info message using the global logger. */
#define XLOG_I(fmt, ...) XLOG_OUTPUT_MESSAGE(xlog_global_instance(), XLOG_LVL_INFO, fmt, ##__VA_ARGS__)
/** @brief Logs a warning message using the global logger. */
#define XLOG_W(fmt, ...) XLOG_OUTPUT_MESSAGE(xlog_global_instance(), XLOG_LVL_WARN, fmt, ##__VA_ARGS__)
/** @brief Logs an error message using the global logger. */
#define XLOG_E(fmt, ...) XLOG_OUTPUT_MESSAGE(xlog_global_instance(), XLOG_LVL_ERROR, fmt, ##__VA_ARGS__)
/** @brief Logs a fatal message using the global logger. */
#define XLOG_F(fmt, ...) XLOG_OUTPUT_MESSAGE(xlog_global_instance(), XLOG_LVL_FATAL, fmt, ##__VA_ARGS__)
/** @} */

/**
 * @defgroup SpecificLoggerMacros Specific Logger Macros
 * @brief Macros for logging with a specific logger instance.
 * @{
 */
/** @brief Logs a trace message using a specific logger. */
#define xlog_trace(logger, fmt, ...) XLOG_OUTPUT_MESSAGE(logger, XLOG_LVL_TRACE, fmt, ##__VA_ARGS__)
/** @brief Logs a debug message using a specific logger. */
#define xlog_debug(logger, fmt, ...) XLOG_OUTPUT_MESSAGE(logger, XLOG_LVL_DEBUG, fmt, ##__VA_ARGS__)
/** @brief Logs an info message using a specific logger. */
#define xlog_info(logger, fmt, ...) XLOG_OUTPUT_MESSAGE(logger, XLOG_LVL_INFO, fmt, ##__VA_ARGS__)
/** @brief Logs a warning message using a specific logger. */
#define xlog_warn(logger, fmt, ...) XLOG_OUTPUT_MESSAGE(logger, XLOG_LVL_WARN, fmt, ##__VA_ARGS__)
/** @brief Logs an error message using a specific logger. */
#define xlog_error(logger, fmt, ...) XLOG_OUTPUT_MESSAGE(logger, XLOG_LVL_ERROR, fmt, ##__VA_ARGS__)
/** @brief Logs a fatal message using a specific logger. */
#define xlog_fatal(logger, fmt, ...) XLOG_OUTPUT_MESSAGE(logger, XLOG_LVL_FATAL, fmt, ##__VA_ARGS__)
/** @} */

/**
 * @defgroup HexdumpMacros Hexdump Macros
 * @brief Macros for logging binary data in hexadecimal format.
 * @{
 */
/** @brief Logs a hexdump using the global logger. */
#define XLOG_HEX(buf, len) xlog_hexdump(NULL, (const uint8_t *)buf, len)
/** @brief Logs a hexdump with a prompt using the global logger. */
#define XLOG_HEX_DUMP(prompt, buf, len) xlog_hexdump(prompt, (const uint8_t *)buf, len)
/** @} */
// clang-format on

/**
 * @internal
 * @def XLOG_OUTPUT_MESSAGE
 * @brief Internal macro to construct and pipe a log message if the level is sufficient.
 */
#define XLOG_OUTPUT_MESSAGE(logger, lvl, fmt, ...)                         \
  do {                                                                     \
    if (xlog_logger_lvl(logger) <= lvl)                                    \
      xlog_pipe(logger, XLOG_MESSAGE_MACRO_INIT(lvl, fmt, ##__VA_ARGS__)); \
  } while (0)

/**
 * @brief Defines the various levels of logging.
 */
typedef enum {
  XLOG_LVL_TRACE, /**< Trace level, for detailed debugging. */
  XLOG_LVL_DEBUG, /**< Debug level, for development-time messages. */
  XLOG_LVL_INFO,  /**< Info level, for informational messages. */
  XLOG_LVL_WARN,  /**< Warn level, for potential issues. */
  XLOG_LVL_ERROR, /**< Error level, for recoverable errors. */
  XLOG_LVL_FATAL, /**< Fatal level, for unrecoverable errors that may terminate
                     the application. */
} xlog_lvl_e;

/**
 * @brief Gets the single character representation of a log level.
 * @param lvl The log level.
 * @return A character ('T', 'D', 'I', 'W', 'E', 'F').
 */
char xlog_lvl_char(xlog_lvl_e lvl);

/**
 * @brief Gets the short string representation of a log level.
 * @param lvl The log level.
 * @return A string (e.g., "TRC", "DBG", "INF").
 */
const char* xlog_lvl_str(xlog_lvl_e lvl);

/**
 * @brief Gets the full string representation of a log level.
 * @param lvl The log level.
 * @return A string (e.g., "trace", "debug", "info").
 */
const char* xlog_lvl_full_str(xlog_lvl_e lvl);

/**
 * @brief Opaque handle to a logger instance.
 */
typedef struct xlogger_private* xlogger;

/**
 * @brief Opaque handle to a log sink.
 */
typedef struct xlog_sink_private* xlog_sink;

/**
 * @defgroup xlog_message_[...]
 * @brief Struct that represents a single log message.
 * @{
 */
/**
 * @brief Represents a single log message with all its metadata.
 */
// clang-format off
typedef struct {
  const char* module;           /**< Caller module name, from macro `XLOG_MOD`. */
  const char* full_file_name;   /**< Caller full file path, from macro `__FILE__`. */
  const char* file_name;        /**< Caller file name (basename of `full_file_name`). */
  const char* func;             /**< Caller function name. */
  int         line;             /**< Caller line number. */
  xlog_lvl_e  lvl;              /**< Log level. */
  xstring     data;             /**< Formatted log message. */
  xbool_t     need_free;        /**< Whether the `data` field needs to be freed. */
} xlog_message_t;
// clang-format on

/**
 * @internal
 * @def XLOG_MESSAGE_MACRO_INIT
 * @brief Internal macro to initialize a log message struct from the call site.
 */
// clang-format off
#define XLOG_MESSAGE_MACRO_INIT(lvl, fmt, ...) \
    xlog_message_init(XLOG_MOD,__FILE__,__func__,__LINE__,lvl,fmt,##__VA_ARGS__)
// clang-format on

/**
 * @brief Initializes a log message struct with metadata and formatted message.
 * @param module The module name.
 * @param file The full file path.
 * @param func The function name.
 * @param line The line number.
 * @param lvl The log level.
 * @param format The printf-style format string.
 * @param ... The arguments for the format string.
 * @return An initialized xlog_message_t object.
 */
xlog_message_t xlog_message_init(const char* module,
                                 const char* file,
                                 const char* func,
                                 int line,
                                 xlog_lvl_e lvl,
                                 const char* format,
                                 ...);
/**
 * @brief Duplicates a log message.
 * @param other The message to duplicate.
 * @return A new dynamically allocated message, or NULL on failure.
 */
xlog_message_t* xlog_message_dup(const xlog_message_t* other);

/**
 * @brief Moves a log message's ownership.
 * @param other The message to move.
 * @return A new dynamically allocated message with content moved from `other`.
 */
xlog_message_t* xlog_message_move(xlog_message_t* other);

/**
 * @brief Releases resources held by a log message.
 * @param message The message to release.
 * @return X_RET_OK  on success.
 */
err_t xlog_message_release(xlog_message_t* message);

// clang-format off
static inline const char* xlog_message_data(const xlog_message_t* message)
{ return xstring_to_string(&message->data); }
static inline size_t xlog_message_length(const xlog_message_t* message)
{ return xstring_length(&message->data); }
static inline const char* xlog_message_module(const xlog_message_t* message)
{ return message->module; }
static inline const char* xlog_message_function(const xlog_message_t* message)
{ return message->func; }
static inline const char* xlog_message_file(const xlog_message_t* message)
{ return message->file_name; }
static inline const char* xlog_message_full_file(const xlog_message_t* message)
{ return message->full_file_name; }
static inline int xlog_message_line(const xlog_message_t* message)
{ return message->line; }
// clang-format on
/** @} */

/**
 * @brief Function pointer for a sink's output handler.
 * @param logger The logger instance.
 * @param sink The sink instance.
 * @param message The log message to be processed.
 */
// clang-format off
typedef void (*sink_output_func)(xlogger l, xlog_sink s, const xlog_message_t* const m);
// clang-format on
/**
 * @brief Function pointer for a sink's flush handler.
 * @param logger The logger instance.
 * @param sink The sink instance.
 */
typedef void (*sink_flush_func)(xlogger l, xlog_sink s);

/**
 * @brief Function pointer for destroying a sink's custom context.
 * @param sink The sink whose context should be destroyed.
 */
typedef void (*sink_ctx_destroy_func)(xlog_sink s);

/**
 * @brief Logs binary data as a hexdump.
 * @param prefix_nullable Optional prefix string for each line of the hexdump.
 * @param buf The buffer to dump.
 * @param len The length of the buffer.
 */
void xlog_hexdump(const char* prefix_nullable, const uint8_t* buf, size_t len);

/**
 * @brief Creates a new log sink.
 * @param ctx_nullable Optional user-defined context for the sink.(use xlog_sink_ctx(s) to get it)
 * @param output_func The function to call for outputting a log message.
 * @param flush_func The function to call to flush the sink's buffer.
 * @return A new sink handle, or NULL on failure.
 */
xlog_sink xlog_sink_new(void*, sink_output_func, sink_flush_func);

/**
 * @brief Creates a new log sink with a custom context destructor.
 * @param ctx_nullable Optional user-defined context for the sink.(use xlog_sink_ctx(s) to get it)
 * @param output_func The function to call for outputting a log message.
 * @param flush_func The function to call to flush the sink's buffer.
 * @param destroy_func The function to call to destroy the context when the sink is deleted.
 * @return A new sink handle, or NULL on failure.
 */
xlog_sink xlog_sink_new_with_destory(void*,
                                     sink_output_func,
                                     sink_flush_func,
                                     sink_ctx_destroy_func);
/**
 * @brief Duplicates a sink handle (increases reference count).
 * @param sink The sink to duplicate.
 * @return The duplicated sink handle.
 */
xlog_sink xlog_sink_dup(xlog_sink sink);

/**
 * @brief Sets the output function for a sink.
 * @param sink The sink to modify.
 * @param output_func The new output function.
 * @return X_RET_OK  on success.
 */
err_t xlog_sink_set_output(xlog_sink sink, sink_output_func output_func);

/**
 * @brief Sets the flush function for a sink.
 * @param sink The sink to modify.
 * @param flush_func The new flush function.
 * @return X_RET_OK  on success.
 */
err_t xlog_sink_set_flush(xlog_sink sink, sink_flush_func flush_func);

/**
 * @brief Sets the context destructor function for a sink.
 * @param s The sink to modify.
 * @param d The new context destructor function.
 * @return X_RET_OK  on success.
 */
err_t xlog_sink_set_destroy_ctx(xlog_sink s, sink_ctx_destroy_func d);

/**
 * @brief Sets the user-defined context for a sink.
 * @param sink The sink to modify.
 * @param ctx The new context pointer.
 * @return X_RET_OK  on success.
 */
err_t xlog_sink_set_ctx(xlog_sink sink, void* ctx);

/**
 * @brief Gets the user-defined context from a sink.
 * @param sink The sink.
 * @return The context pointer.
 */
void* xlog_sink_ctx(xlog_sink sink);

/**
 * @brief Deletes a sink (decreases reference count and frees if zero).
 * @param sink The sink to delete.
 * @return X_RET_OK  on success.
 */
err_t xlog_sink_del(xlog_sink sink);

/**
 * @brief Function pointer for redirecting logger output.
 */
typedef void (*xlog_output_func)(xlogger, const xlog_message_t*);

/**
 * @brief Options for creating a logger.
 */
// clang-format off
typedef struct {
  xlog_lvl_e lvl;               /**< The minimum level of messages to be logged. */
  xlog_lvl_e which_flush;       /**< The minimum level at which to automatically flush sinks. */
  void* ctx;                    /**< User-defined context for the logger. */
  xlog_output_func redirect;    /**< Optional function to redirect all output, bypassing sinks. */
} xlog_options_t;
// clangf-format on

/**
 * @brief Initializes logger options with default values.
 * @param lvl The minimum log level.
 * @param which_flush The level at which to trigger a flush.
 * @return An initialized options struct.
 */
xlog_options_t xlog_options_init(xlog_lvl_e lvl, xlog_lvl_e which_flush);

/** @internal */
#define _xlog_create_logger_function(_0, _1, _Func, ...) _Func
/**
 * @brief Creates a logger instance.
 * @details This is a macro that dispatches to `xlog_create_logger_default()`
 *          if called with no arguments, or `xlog_create_logger_with_options()`
 *          if called with an options struct.
 * @param ... Optional: A pointer to an `xlog_options_t` struct.
 * @return A new logger handle.
 */
#define xlog_create_logger(...)                                 \
  _xlog_create_logger_function(dummy, ##__VA_ARGS__,            \
                               xlog_create_logger_with_options, \
                               xlog_create_logger_default)(__VA_ARGS__)

/**
 * @brief Creates a logger with default options.
 * @return A new logger handle.
 */
xlogger xlog_create_logger_default(void);

/**
 * @brief Creates a logger with custom options.
 * @param opt Pointer to the options struct.
 * @return A new logger handle.
 */
xlogger xlog_create_logger_with_options(xlog_options_t* opt);

/**
 * @brief Creates a logger with a variable number of sinks.
 * @param opt Logger options.
 * @param sink_count The number of sinks provided.
 * @param ... The sink handles.
 * @return A new logger handle.
 */
xlogger xlog_create_logger_with_sinks(xlog_options_t* opt, size_t sink_count, ...);

/**
 * @brief Creates a logger with an array of sinks.
 * @param opt Logger options.
 * @param sink_count The number of sinks in the array.
 * @param sinks An array of sink handles.
 * @return A new logger handle.
 */
xlogger xlog_create_logger_with_sink_array(xlog_options_t* opt, size_t sink_count, xlog_sink* sinks);

/**
 * @brief Appends a sink to a logger.
 * @param logger The logger instance.
 * @param sink The sink to add.
 * @return X_RET_OK  on success.
 */
err_t xlog_logger_append_sink(xlogger logger, xlog_sink sink);

/**
 * @brief Gets the current log level of a logger.
 * @param logger The logger instance.
 * @return The current log level.
 */
xlog_lvl_e xlog_logger_lvl(xlogger logger);

/**
 * @brief Gets the user-defined context of a logger.
 * @param logger The logger instance.
 * @return The context pointer.
 */
void* xlog_logger_ctx(xlogger logger);

/**
 * @brief Destroys a logger instance and frees associated resources.
 * @param logger The logger to destroy.
 * @return X_RET_OK  on success.
 */
err_t xlog_logger_destroy(xlogger logger);

/**
 * @brief Gets the global logger instance. A default one is created if it doesn't exist.
 * @return The global logger instance.
 */
xlogger xlog_global_instance(void);

/**
 * @brief Gets the user-defined context from the global logger.
 * @return The context pointer.
 */
void* xlog_global_ctx(void);

/**
 * @brief Gets the log level of the global logger.
 * @return The current log level.
 */
xlog_lvl_e xlog_global_lvl(void);

/**
 * @brief Sets the user-defined context for the global logger.
 * @param ctx The context pointer.
 * @return X_RET_OK  on success.
 */
err_t xlog_global_set_ctx(void* ctx);

/**
 * @brief Sets the log level for the global logger.
 * @param lvl The new log level.
 * @return X_RET_OK  on success.
 */
err_t xlog_global_set_lvl(xlog_lvl_e lvl);

/**
 * @brief Replaces the global logger instance with a custom one.
 * @param logger The new logger instance. The global logger takes ownership.
 * @return X_RET_OK  on success.
 */
err_t xlog_global_set_instance(xlogger logger);

/**
 * @brief Resets the global logger to its default state.
 * @return X_RET_OK  on success.
 */
err_t xlog_global_reset(void);

/**
 * @brief Passes a log message to the logger's sinks or redirect function.
 * @param self The logger instance.
 * @param message The log message.
 */
void xlog_output(xlogger self, const xlog_message_t* message);

/**
 * @brief The core logging pipeline function.
 * @details This function takes a message, checks the logger's level,
 *          and passes it to the output sinks.
 * @param logger The logger instance.
 * @param message The log message object (passed by value).
 */
void xlog_pipe(xlogger logger, xlog_message_t message);

#ifdef __cplusplus
}
#endif

#endif /* XTOOL_XLOG__H_ */
