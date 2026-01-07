#ifndef NOTIFY_H_
#define NOTIFY_H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "xdef.h"

typedef struct {
    void (*update_status)(const char *status, int precent, int total, int current);
} notify_operators_t;

err_t register_notify_operators(notify_operators_t *ops);
notify_operators_t *get_notify_operators();


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* NOTIFY_H_ */
