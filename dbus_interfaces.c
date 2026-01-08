/**
 * D-Bus Signal Emitter Implementation
 * 
 * Service Name: com.iota.status
 * Object Path:   /com/iota/status
 * Interface:     com.iota.status. Interface
 */
#define XLOG_MOD "dbus"
#include "xlog.h"
#include "dbus_interfaces.h"
#include <dbus/dbus.h>

#define DBUS_SERVICE_NAME    "com.iota.status"
#define DBUS_OBJECT_PATH     "/com/iota/status"
#define DBUS_INTERFACE_NAME  "com.iota.status.Interface"

#define SIGNAL_PROGRESS_CHANGED  "ProgressChanged"
#define SIGNAL_MESSAGE_LOGGED    "MessageLogged"
#define SIGNAL_ERROR_OCCURRED    "ErrorOccurred"

static DBusConnection *g_dbus_conn = NULL;

static void init(void);
static err_t progress_changed(const char *step, int percent, int total, int current);
static err_t message_logged(const char *log_msg);
static err_t error_occurred(int err_code, const char *err_msg);
static void fini(void);

void register_dbus_notify_operators() {
    static notify_operators_t dbus_notify_ops = {
        .progress_changed = progress_changed,
        .message_logged = message_logged,
        .error_occurred = error_occurred,
    };

    init();

    register_notify_operators(&dbus_notify_ops);
}

void init(void)
{
    DBusError err;
    int ret;

    dbus_error_init(&err);

    g_dbus_conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if (dbus_error_is_set(&err)) {
        XLOG_E("D-Bus Connection Error: %s", err.message);
        dbus_error_free(&err);
        return;
    }

    if (g_dbus_conn == NULL) {
        XLOG_E("D-Bus Connection is NULL");
        return;
    }

#if 0
    ret = dbus_bus_request_name(g_dbus_conn,
                                DBUS_SERVICE_NAME,
                                DBUS_NAME_FLAG_REPLACE_EXISTING,
                                &err);
    if (dbus_error_is_set(&err)) {
        XLOG_E("D-Bus Request Name Error: %s", err.message);
        dbus_error_free(&err);
        return;
    }

    if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
        XLOG_W("D-Bus Not Primary Owner (%d)", ret);
        // return;
    }

    XLOG_D("D-Bus initialized successfully.  Service:  %s", DBUS_SERVICE_NAME);
#endif
}

/**
 * 发送进度变化信号
 * Signal:  ProgressChanged
 * Signature: sitt (string, int32, int64, int64)
 * 
 * @param step    步骤名称
 * @param percent 百分比
 * @param total   总大小
 * @param current 已处理大小
 */
err_t progress_changed(const char *step, int percent, int total, int current)
{
    DBusMessage *msg;
    dbus_int32_t d_percent = (dbus_int32_t)percent;
    dbus_int64_t d_total = (dbus_int64_t)total;
    dbus_int64_t d_current = (dbus_int64_t)current;

    if (g_dbus_conn == NULL) {
        return X_RET_INVAL;
    }

    msg = dbus_message_new_signal(DBUS_OBJECT_PATH,
                                  DBUS_INTERFACE_NAME,
                                  SIGNAL_PROGRESS_CHANGED);
    if (msg == NULL) {
        return X_RET_ERROR;
    }

    if (! dbus_message_append_args(msg,
                                   DBUS_TYPE_STRING, &step,
                                   DBUS_TYPE_INT32, &d_percent,
                                   DBUS_TYPE_INT64, &d_total,
                                   DBUS_TYPE_INT64, &d_current,
                                   DBUS_TYPE_INVALID)) {
        dbus_message_unref(msg);
        return X_RET_ERROR;
    }

    if (! dbus_connection_send(g_dbus_conn, msg, 0)) {
        dbus_message_unref(msg);
        return X_RET_ERROR;
    }

    dbus_connection_flush(g_dbus_conn);
    dbus_message_unref(msg);
    return X_RET_OK;
}

/**
 * 发送消息日志信号
 * Signal: MessageLogged
 * Signature: s (string)
 * 
 * @param msg 无进度的描述性文本/日志
 */
err_t message_logged(const char *log_msg)
{
    DBusMessage *msg;

    if (g_dbus_conn == NULL) {
        return X_RET_INVAL;
    }

    msg = dbus_message_new_signal(DBUS_OBJECT_PATH,
                                  DBUS_INTERFACE_NAME,
                                  SIGNAL_MESSAGE_LOGGED);
    if (msg == NULL) {
        return X_RET_ERROR;
    }

    if (!dbus_message_append_args(msg,
                                  DBUS_TYPE_STRING, &log_msg,
                                  DBUS_TYPE_INVALID)) {
        dbus_message_unref(msg);
        return X_RET_ERROR;
    }

    if (!dbus_connection_send(g_dbus_conn, msg, NULL)) {
        dbus_message_unref(msg);
        return X_RET_ERROR;
    }

    dbus_connection_flush(g_dbus_conn);
    dbus_message_unref(msg);

    return X_RET_OK;
}

/**
 * 发送错误发生信号
 * Signal:  ErrorOccurred
 * Signature: is (int32, string)
 * 
 * @param err_code 错误码
 * @param err_msg  错误详细描述
 */
err_t error_occurred(int err_code, const char *err_msg)
{
    DBusMessage *msg;
    dbus_int32_t d_err_code = (dbus_int32_t)err_code;

    if (g_dbus_conn == NULL) {
        return X_RET_INVAL;
    }

    msg = dbus_message_new_signal(DBUS_OBJECT_PATH,
                                   DBUS_INTERFACE_NAME,
                                   SIGNAL_ERROR_OCCURRED);
    if (msg == NULL) {
        return X_RET_ERROR;
    }

    if (! dbus_message_append_args(msg,
                                   DBUS_TYPE_INT32, &d_err_code,
                                   DBUS_TYPE_STRING, &err_msg,
                                   DBUS_TYPE_INVALID)) {
        dbus_message_unref(msg);
        return X_RET_ERROR;
    }

    if (! dbus_connection_send(g_dbus_conn, msg, NULL)) {
        dbus_message_unref(msg);
        return X_RET_ERROR;
    }

    dbus_connection_flush(g_dbus_conn);
    dbus_message_unref(msg);

    return X_RET_OK;
}

void fini(void)
{
    if (g_dbus_conn != NULL) {
        DBusError err;
        dbus_error_init(&err);

        dbus_bus_release_name(g_dbus_conn, DBUS_SERVICE_NAME, &err);
        if (dbus_error_is_set(&err)) {
            XLOG_E("D-Bus Release Name Error: %s", err.message);
            dbus_error_free(&err);
        }

        dbus_connection_unref(g_dbus_conn);
        g_dbus_conn = NULL;

        XLOG_I("D-Bus connection closed");
    }
}
