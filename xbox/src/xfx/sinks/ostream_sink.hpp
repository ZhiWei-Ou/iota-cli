/**
 * @brief C++ 标准输出流接收器
 * @file ostream_sink.hpp
 * @author Oswin
 * @date 2025-11-05
 * @details Provides a C++ log sink for writing log messages to any `std::ostream`.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef XFRAMEWORK_OSTREAM_SINK_H_
#define XFRAMEWORK_OSTREAM_SINK_H_
#ifdef __cplusplus

#include <ostream>
#include <string>

#include "xlog.h"

namespace xbox {
/**
 * @internal
 * @class ostream_sink_context
 * @brief The internal context for an ostream_sink.
 * @details This class holds the state (the ostream and flush policy) and provides
 *          static C-style callback functions to interface with the `xlog_sink`
 * API.
 */
class ostream_sink_context final {
 public:
  /**
   * @brief Constructs the ostream sink context.
   * @param os The output stream to write to.
   * @param force_flush If true, the stream will be flushed after every write.
   */
  explicit ostream_sink_context(std::ostream& os, bool force_flush = false)
      : os_(os), force_flush_(force_flush) {}

  /**
   * @brief The static output callback for the C `xlog_sink`.
   */
  static void output(xlogger logger,
                     xlog_sink sink,
                     const xlog_message_t* const message) {
    auto ctx = static_cast<ostream_sink_context*>(xlog_sink_ctx(sink));
    std::string msg = xlog_message_data(message);
    msg += "\n";

    ctx->os_.write(msg.c_str(), msg.size());
    if (ctx->force_flush_) {
      ctx->os_.flush();
    }
  }

  /**
   * @brief The static flush callback for the C `xlog_sink`.
   */
  static void flush(xlogger logger, xlog_sink sink) {
    xUNUSED(logger);

    auto ctx = static_cast<ostream_sink_context*>(xlog_sink_ctx(sink));
    ctx->os_.flush();
  }

  /**
   * @brief The static context destructor callback for the C `xlog_sink`.
   */
  static void destory(xlog_sink sink) {
    auto ctx = static_cast<ostream_sink_context*>(xlog_sink_ctx(sink));
    delete ctx;
  }

 private:
  std::ostream& os_;
  bool force_flush_;
};
}  // namespace xbox

/**
 * @brief Creates a new log sink that writes to a `std::ostream`.
 * @details This is a C++ helper function that simplifies the creation of a C++ stream sink.
 * @param os The `std::ostream` to write to (e.g., `std::cout`, `std::cerr`, `std::stringstream`).
 * @param force_flush If true, the stream will be flushed after every log message.
 * @return A handle to the new log sink (`xlog_sink`), or NULL on failure.
 */
static xlog_sink ostream_sink(std::ostream& os, bool force_flush = false) {
  auto ctx = new xbox::ostream_sink_context(os, force_flush);
  return xlog_sink_new_with_destory(static_cast<void*>(ctx),
                                    xbox::ostream_sink_context::output,
                                    xbox::ostream_sink_context::flush,
                                    xbox::ostream_sink_context::destory);
}

#endif /* __cplusplus */
#endif /* XFRAMEWORK_OSTREAM_SINK_H_ */
