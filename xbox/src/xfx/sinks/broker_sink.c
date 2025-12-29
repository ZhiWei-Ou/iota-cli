/**
 * @brief 中间件接收器
 * @file nats_sink.c
 * @author Oswin
 * @date 2025-11-05
 * @details 中间件接收器
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "broker_sink.h"

static void _output(xlogger, xlog_sink, const xlog_message_t* const);
static void _flush(xlogger, xlog_sink);
static void _destory(xlog_sink);

struct broker_sink {
  broker brk;
  char* topic;
};

xlog_sink broker_sink(broker b, const char* topic) {
  if (b == NULL || topic == NULL) return NULL;

  struct broker_sink* ctx = xbox_malloc(sizeof(struct broker_sink));
  if (ctx == NULL) return NULL;

  ctx->brk = b;
  ctx->topic = xbox_strdup(topic);

  return xlog_sink_new_with_destory(ctx, _output, _flush, _destory);
}

static void _output(xlogger logger,
                    xlog_sink sink,
                    const xlog_message_t* const message) {
  struct broker_sink* ctx = xlog_sink_ctx(sink);

  if (ctx == NULL) return;

  broker_publish(ctx->brk, ctx->topic, xlog_message_data(message));
}

static void _flush(xlogger logger, xlog_sink sink) {
  xUNUSED(logger);
  xUNUSED(sink);
}

static void _destory(xlog_sink sink) {
  struct broker_sink* ctx = xlog_sink_ctx(sink);

  if (ctx == NULL) return;

  if (ctx->topic) xbox_free(ctx->topic);
  xbox_free(ctx);
}
