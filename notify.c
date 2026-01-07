#include "notify.h"

static notify_operators_t *notify_ops = NULL;

err_t register_notify_operators(notify_operators_t *ops) {
    if (ops == NULL) {
        return X_RET_INVAL;
    }
    notify_ops = ops;
    return X_RET_OK;
}

notify_operators_t *get_notify_operators() {
    return notify_ops;
}
