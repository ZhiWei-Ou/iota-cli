#ifndef NOTIFY_H_
#define NOTIFY_H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "xdef.h"

typedef struct {
    err_t (*progress_changed)(const char *step, int precent, int total, int current);
    err_t (*message_logged)(const char *msg);
    err_t (*error_occurred)(int err_code, const char *err_msg);

    // Additional notification operators can be added here
    // void (*firmware_info)(const char *version, const char *date);
    // void (*usb_event)(const char *event, const char *device);
    // void (*reboot_event)(const char *reason, int delay_seconds);
} notify_operators_t;

err_t register_notify_operators(notify_operators_t *ops);
notify_operators_t *get_notify_operators();


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* NOTIFY_H_ */
