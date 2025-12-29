/**
 * @brief 日志组件
 * @file log.c
 * @author Oswin
 * @date 2025-07-09
 * @details modern logging framework implementation
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "log.h"

#include <assert.h>
#include <string.h>

#include "os_mailbox.h"
#include "os_thread.h"
#include "sinks/console_sink.h"
#include "sinks/rotating_file_sink.h"
#include "xjson.h"
#include "xlog.h"

xbool_t async_thread_exit_ = xTRUE;
os_thread aync_thread_handle = NULL;

err_t log_config_unmarshal(const xjson root, log_config_t* conf) {
  if (root == NULL || conf == NULL) return X_RET_INVAL;

  const char* level = xjson_query_string(root, "/log/level", "info");
  const char* path = xjson_get_string(root, "/log/file");

  strncpy(conf->level, level, sizeof(conf->level));
  strcpy(conf->file, path ? path : "");
  conf->color = xjson_query_bool(root, "/log/color", xTRUE);
  conf->quiet = xjson_query_bool(root, "/log/quiet", xFALSE);
  conf->backup = xjson_get_int(root, "/log/backup");
  conf->max_size = xjson_get_int(root, "/log/max_size");
  conf->json = xjson_query_bool(root, "/log/json", xFALSE);
  conf->async = xjson_query_bool(root, "/log/async", xFALSE);

  return X_RET_OK;
}

log_config_t default_log_config(void) {
  log_config_t conf = {
      .level = "info",
      .file = "",
      .color = xTRUE,
      .quiet = xFALSE,
      .max_size = 10,
      .backup = 3,
      .json = xFALSE,
      .async = xFALSE,
  };

  return conf;
}

static xlog_lvl_e _lvl_parse(const char* level) {
  if (level == NULL) return XLOG_LVL_INFO;

  if (strcasecmp(level, "trace") == 0)
    return XLOG_LVL_TRACE;
  else if (strcasecmp(level, "debug") == 0)
    return XLOG_LVL_DEBUG;
  else if (strcasecmp(level, "info") == 0)
    return XLOG_LVL_INFO;
  else if (strcasecmp(level, "warn") == 0)
    return XLOG_LVL_WARN;
  else if (strcasecmp(level, "error") == 0)
    return XLOG_LVL_ERROR;
  else if (strcasecmp(level, "fatal") == 0)
    return XLOG_LVL_FATAL;
  else
    return XLOG_LVL_INFO;
}

static void async_thread(void* arg) {
  xlogger logger = (xlogger)arg;
  os_mailbox mb = xlog_logger_ctx(logger);

  while (!async_thread_exit_) {
    os_mailbox_message msg;
    err_t err = os_mailbox_recv(mb, &msg, 100);
    if (err != X_RET_OK) continue;

    xlog_message_t* log_msg = (xlog_message_t*)msg;
    if (log_msg == NULL) continue;

    xlog_output(logger, log_msg);

    xlog_message_release(log_msg);
  }
}
static void start_aync_thread_once(xlogger self) {
  if (aync_thread_handle) return;

  async_thread_exit_ = xFALSE;
  aync_thread_handle = os_thread_create("log_async_thread",
                                        async_thread,
                                        self,
                                        OS_THREAD_DEFAULT_STACK_SIZE,
                                        OS_THREAD_PRIO_NORMAL);
}

static void aync_pipe(xlogger self, const xlog_message_t* message) {
  xlog_message_t* msg_copy = xlog_message_dup(message);
  if (msg_copy == NULL) return;

  os_mailbox mb = xlog_logger_ctx(self);

  err_t err = os_mailbox_send(mb, (os_mailbox_message)msg_copy);
  if (err != X_RET_OK) {
    xlog_message_release(msg_copy);
  }
}

err_t log_init(const log_config_t* conf) {
  log_config_t def = default_log_config();
  if (conf == NULL) {
    conf = &def;
  }

  xlog_options_t opts = xlog_options_init(_lvl_parse(conf->level),
                                          XLOG_LVL_INFO);

  if (conf->async) {
    opts.redirect = aync_pipe;
    opts.ctx = os_mailbox_create(LOG_ASYNC_QUEUE_MAX_SIZE);
    if (opts.ctx == NULL) return X_RET_ERROR;
  }

  xlogger logger = xlog_create_logger(&opts);

  // check if not quiet mode, and then create console sink
  if (!conf->quiet) {
    if (conf->color) {
      xlog_logger_append_sink(logger, stdout_color_sink());
    } else {
      xlog_logger_append_sink(logger, stdout_sink());
    }
  }

  // check if file path is set, and then create rotating file sink
  if (strlen(conf->file) > 0) {
    rotating_file_options_t rotating_opts = {
        .file = (char*)conf->file,
        .max_size = conf->max_size * 1024,  // MB to KB
        .backup = conf->backup,
        .color = conf->color,
        .json_fmt = conf->json,
    };
    xlog_logger_append_sink(logger, rotating_file_sink(&rotating_opts));
  }

  xlog_global_set_instance(logger);

  if (conf->async) {
    start_aync_thread_once(logger);
  }

  return X_RET_OK;
}

err_t log_fini() {
  if (aync_thread_handle) {
    // wait 500ms to allow the async logging thread to process most messages.
    // this helps during unit tests and benchmarks.
    // in production, calling `log_fini` means the program is shutting down,
    // so there should be little or no pending log activity.
    os_msleep(500);

    async_thread_exit_ = xTRUE;
    os_thread_destroy(aync_thread_handle);

    xlogger logger = xlog_global_instance();
    os_mailbox mb = xlog_logger_ctx(logger);
    os_mailbox_destroy(mb);
  }

  return xlog_global_reset();
}
