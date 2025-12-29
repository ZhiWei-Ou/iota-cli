/**
 * @brief 文件接收器
 * @file basic_file_sink.c
 * @author Oswin
 * @date 2025-11-04
 * @details 文件接收器
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "basic_file_sink.h"

#include <string.h>

static void _output(xlogger, xlog_sink, const xlog_message_t* const);
static void _flush(xlogger, xlog_sink);
static void _destory(xlog_sink);

xlog_sink basic_file_sink(const char* path) {
  if (path == NULL) return NULL;

  basic_file_sink_context_t* ctx = xbox_malloc(
      sizeof(basic_file_sink_context_t));
  if (ctx == NULL) return NULL;

  ctx->fp = os_file_open(path, "a+");
  ctx->mtx = os_mutex_create("basic_file");

  return xlog_sink_new_with_destory(ctx, _output, _flush, _destory);
}

static void _output(xlogger logger,
                    xlog_sink sink,
                    const xlog_message_t* const message) {
  basic_file_sink_context_t* ctx = xlog_sink_ctx(sink);

  os_mutex_lock(ctx->mtx);
  fprintf(ctx->fp, "%s\n", xlog_message_data(message));
  os_mutex_unlock(ctx->mtx);
}

static void _flush(xlogger logger, xlog_sink sink) {
  basic_file_sink_context_t* ctx = xlog_sink_ctx(sink);

  os_mutex_lock(ctx->mtx);
  fflush(ctx->fp);
  os_mutex_unlock(ctx->mtx);
}

static void _destory(xlog_sink sink) {
  basic_file_sink_context_t* ctx = xlog_sink_ctx(sink);

  os_mutex_destroy(ctx->mtx);
  os_file_close(ctx->fp);
  xbox_free(ctx);
}
