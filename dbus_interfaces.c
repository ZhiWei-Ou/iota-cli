#include "dbus_interfaces.h"

#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>

#define DBUS_OBJECT_PATH "/com/iota/status"
#define DBUS_INTERFACE_NAME "com.iota.status.Interface"
#define DBUS_SIGNAL_NAME "UpdateStatus"

static DBusConnection *conn = NULL;

static int init_dbus_sender();
static void update_status(const char *status, int percent, int total, int current);

int init_dbus_sender() {
    DBusError err;
    dbus_error_init(&err);

    conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Connection Error (%s)\n", err.message);
        dbus_error_free(&err);
        return -1;
    }
    return 0;
}

void register_dbus_notify_operators() {
    static notify_operators_t dbus_ops = {
        .update_status = update_status
    };

    if (conn == NULL) {
        if (init_dbus_sender() < 0) return;
    }

    register_notify_operators(&dbus_ops);
}

void update_status(const char *status, int percent, int total, int current) {
    DBusMessage *sig;
    DBusMessageIter iter;
    dbus_uint32_t serial = 0;
    
    dbus_int32_t dbus_percent = (dbus_int32_t)percent;
    dbus_int64_t dbus_total = (dbus_int64_t)total;
    dbus_int64_t dbus_current = (dbus_int64_t)current;

    sig = dbus_message_new_signal(DBUS_OBJECT_PATH, DBUS_INTERFACE_NAME, DBUS_SIGNAL_NAME);
    if (sig == NULL) {
        return;
    }

    dbus_message_iter_init_append(sig, &iter);

    // String: status
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &status);
    // Int32: percent
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &dbus_percent);
    // Int64: total
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT64, &dbus_total);
    // Int64: current
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT64, &dbus_current);

    if (!dbus_connection_send(conn, sig, &serial)) {
        fprintf(stderr, "Out Of Memory!\n");
    }

    dbus_connection_flush(conn);

    dbus_message_unref(sig);
}
