#define XLOG_MOD "checkout"
#include "checkout.h"
#include "os_file.h"
#include "xlog.h"
#include "exec.h"
#include <string.h>
#include <unistd.h>

typedef struct {
    xoption this_option;
    struct {
        char *specified_script;
        xbool_t force;
        xbool_t need_reboot;
        int reboot_delay_second;
    } flags;
} checkout_context_t;

static checkout_context_t g_checkout_ctx = {
    .this_option = NULL,
    .flags = {
        .specified_script = NULL,
        .force = xFALSE,
        .need_reboot = xFALSE,
        .reboot_delay_second = 3,
    },
};

err_t checkout_usage_init(xoption root) {
    if (!root)
        return X_RET_INVAL;

    if (g_checkout_ctx.this_option)
        return X_RET_OK;

    extern err_t checkout_feature_entry(xoption self);

    xoption checkout = xoption_create_subcommand(root, "checkout", "Select another partition for the next boot.");
    xoption_set_context(checkout, &g_checkout_ctx);
    xoption_set_post_parse_callback(checkout, checkout_feature_entry);
    xoption_add_string(checkout, 'x', "script", "<script.sh>",
                       "Custom shell script to run after the partition switch",
                       &g_checkout_ctx.flags.specified_script, xFALSE);
    xoption_add_boolean(checkout, '\0', "reboot",
                        "Automatically restart the system after a successful checkout",
                        &g_checkout_ctx.flags.need_reboot); 
    xoption_add_number(checkout, '\0', "delay", "<seconds>",
                       "Time to wait (in seconds) before performing the reboot",
                       &g_checkout_ctx.flags.reboot_delay_second, xFALSE);
    xoption_add_boolean(checkout, 'f', "force",
                        "Force the checkout even if the target partition is already active",
                        &g_checkout_ctx.flags.force);

    g_checkout_ctx.this_option = checkout;

    return X_RET_OK;
}

void assert_requirements() {
    XLOG_D("Checking requirements for checkout feature...");

    assert_command("fw_setenv");
    assert_command("fw_printenv");
    assert_command("reboot");
}

static void run_script_with_check(const char *script) {
    if (script == NULL)
        return;

    XLOG_I("Running checkout script: %s", script);

    XLOG_D("Checking and running script: %s", script);

    if (script == NULL || strlen(script) == 0) {
        XLOG_W("No script or empty provided to run.");
        return;
    }

    if (!os_file_exist(script)) {
        XLOG_W("The script file does not exist: %s, skipping it.", script);
        return;
    }

    xstring cmd = xstring_init_format("/bin/bash %s", script);
    exec_t r = exec_command(xstring_to_string(&cmd));
    if (!exec_success(r)) {
        XLOG_W("The script execution failed. return code: %d", r.code);
    }
    exec_free(r);
    xstring_free(&cmd);

    XLOG_I("Script executed: %s", script);
}

static void reboot_with_check(checkout_context_t *ctx) {
    if (!ctx->flags.need_reboot) 
        return;

    int delay = ctx->flags.reboot_delay_second;
    if (delay <= 0) {
        XLOG_W("Rebooting system immediately...");
    } else {
        XLOG_W("Rebooting system after %d seconds...", delay);
        sleep(delay);
    }

    exec_t r = exec_command("reboot");
    if (!exec_success(r)) {
        XLOG_E("Failed to reboot the system. return code: %d", r.code);
    }

    exec_free(r);
}

err_t checkout_with_reboot(checkout_context_t *ctx, const char *part) {
    xstring cmd = xstring_init_format("fw_setenv %s %s", UBOOTENV_VAR_ROOTFS_PART, part);
    exec_t r = exec_command(xstring_to_string(&cmd));
    xstring_free(&cmd);

    if (!exec_success(r)) {
        XLOG_E("Failed to set rootfs_root to '%s'", part);
        exec_free(r);
        return X_RET_ERROR;
    }

    exec_free(r);

    XLOG_D("Checked out to partition: '%s'", part);

    XLOG_I("Partition switching successful");

    run_script_with_check(ctx->flags.specified_script);

    reboot_with_check(ctx);

    return X_RET_OK;
}

err_t checkout_feature_entry(xoption self) {
    assert_requirements();

    checkout_context_t *ctx = xoption_get_context(self);
    if (ctx == NULL) {
        XLOG_E("Invalid checkout context.");
        return X_RET_INVAL;
    }

    // Get current rootfs partition from rootfs mount
    exec_t current_rootfs_part = exec_command("awk '$2==\"/\" {print $1}' /proc/self/mounts");
    if (!exec_success(current_rootfs_part)) {
        XLOG_D("mount output: %s", exec_output(current_rootfs_part));
        XLOG_E("Failed to get current rootfs mount info.");
        return X_RET_ERROR;
    }

    // Get current boot partition from U-Boot env
    exec_t current_env_part = exec_command("fw_printenv -n " UBOOTENV_VAR_ROOTFS_PART);
    if (!exec_success(current_env_part)) {
        XLOG_E("Failed to get current rootfs_root. output: %s", exec_output(current_env_part));
        return X_RET_ERROR;
    }

    // Remove whitespace and breaklines
    const char *env_part = xstring_trim(&current_env_part.output);
    const char *rootfs_part = xstring_trim(&current_rootfs_part.output);
    const char *checkout_part = NULL;

    XLOG_D("Current rootfs source: '%s' and env partition: '%s'", rootfs_part, env_part);

    // Determine the inactive partition
    if (!strcmp(env_part, "a")) {
        checkout_part = "b";
    } else if (!strcmp(env_part, "b")) {
        checkout_part = "a";
    } else {
        XLOG_E("Invalid current rootfs_part: %s", env_part);
        exec_free(current_env_part);
        return -1;
    }

    if ((!strcmp(checkout_part, "a") && !strcmp(rootfs_part, "ubi0:a")) ||
        (!strcmp(checkout_part, "b") && !strcmp(rootfs_part, "ubi0:b"))) {

        XLOG_W("The checkout partition '%s' is already the active partition.", checkout_part);

        if (!ctx->flags.force) {
            XLOG_W("Skipping checkout. Use --force to override if you really want to checkout to the same partition.");
            return X_RET_EXIST;
        }
    }

    err_t code = checkout_with_reboot(ctx, checkout_part);

    if (code == X_RET_OK) {
        XLOG_I("Successfully checked out to partition: '%s'", checkout_part);
    }

    return code;
}

xstring get_inactive_partition(void) {
    // Get current rootfs_part
    exec_t current_part = exec_command("fw_printenv -n " UBOOTENV_VAR_ROOTFS_PART);
    if (!exec_success(current_part)) {
        XLOG_E("Failed to get current rootfs_root. output: %s", exec_output(current_part));
        exec_free(current_part);
        return xstring_init_empty();
    }

    // Remove whitespace and breaklines
    const char *p = xstring_trim(&current_part.output);
    if (!p || strlen(p) == 0) {
        XLOG_E("Current rootfs_part is empty (No expect).");
        exec_free(current_part);
        return xstring_init_empty();
    }

    if (!strcmp(p, "a")) {
        return xstring_init_iter("b");
    } else if (!strcmp(p, "b")) {
        return xstring_init_iter("a");
    } else {
        XLOG_E("Invalid current rootfs_part: %s", p);
        exec_free(current_part);
        return xstring_init_empty();
    }
}

xstring get_active_partition(void) {
    // Get current rootfs_part
    exec_t current_part = exec_command("fw_printenv -n " UBOOTENV_VAR_ROOTFS_PART);
    if (!exec_success(current_part)) {
        XLOG_E("Failed to get current rootfs_root. output: %s", exec_output(current_part));
        exec_free(current_part);
        return xstring_init_empty();
    }

    // Remove whitespace and breaklines
    const char *p = xstring_trim(&current_part.output);
    if (!p || strlen(p) == 0) {
        XLOG_E("Current rootfs_part is empty (No expect).");
        exec_free(current_part);
        return xstring_init_empty();
    }

    return xstring_init_iter(p);
}

static xbool_t checkout_mount_already(const char *part) {
    if (part == NULL)
        return xFALSE;

    xstring cmd = xstring_init_format("mount | grep '%s'",
                                      part);
    exec_t r = exec_command(xstring_to_string(&cmd));
    xstring_free(&cmd);
    if (exec_success(r)) {
        exec_free(r);
        return xTRUE;
    } else {
        exec_free(r);
        return xFALSE;
    }
}

err_t mount_inactive_partition(void) {
    xstring inactive_part = get_inactive_partition();
    if (xstring_is_empty(&inactive_part)) {
        XLOG_E("Cannot get inactive partition.");
        xstring_free(&inactive_part);
        return X_RET_ERROR;
    }

    const char *ubi_dev = NULL;
    if (xstring_equal(&inactive_part, "a", X_CASE)) {
        ubi_dev = "/dev/ubi0_0";
    } else if (xstring_equal(&inactive_part, "b", X_CASE)) {
        ubi_dev = "/dev/ubi0_1";
    } else {
        XLOG_E("Invalid inactive partition: %s", xstring_to_string(&inactive_part));
        xstring_free(&inactive_part);
        return X_RET_ERROR;
    }

    // If already mounted, report error
    if (checkout_mount_already(ubi_dev)) {
        XLOG_W("Inactive partition already mounted. please unmount it first.");
        return X_RET_EXIST;
    }

    xstring cmd = xstring_init_format("mkdir -p %s; mount -t ubifs %s %s",
                                      INACTIVE_PARTITION_MOUNT_POINT,
                                      ubi_dev,
                                      INACTIVE_PARTITION_MOUNT_POINT);

    exec_t r = exec_command(xstring_to_string(&cmd));
    xstring_free(&cmd);
    xstring_free(&inactive_part);
    if (!exec_success(r)) {
        exec_free(r);
        return X_RET_ERROR;
    }

    exec_free(r);
    return X_RET_OK;
}

err_t unmount_inactive_partition(void) {
    xstring inactive_part = get_inactive_partition();
    if (xstring_is_empty(&inactive_part)) {
        XLOG_E("Cannot get inactive partition.");
        xstring_free(&inactive_part);
        return X_RET_ERROR;
    }

    const char *ubi_dev = NULL;
    if (xstring_equal(&inactive_part, "a", X_CASE)) {
        ubi_dev = "/dev/ubi0_0";
    } else if (xstring_equal(&inactive_part, "b", X_CASE)) {
        ubi_dev = "/dev/ubi0_1";
    } else {
        XLOG_E("Invalid inactive partition: %s", xstring_to_string(&inactive_part));
        xstring_free(&inactive_part);
        return X_RET_ERROR;
    }

    xstring_free(&inactive_part);

    xstring cmd = xstring_init_format("test -d %s || exit 0;sync && umount -l %s && rmdir %s",
                                      INACTIVE_PARTITION_MOUNT_POINT,
                                      INACTIVE_PARTITION_MOUNT_POINT,
                                      INACTIVE_PARTITION_MOUNT_POINT);
    exec_t r = exec_command(xstring_to_string(&cmd));
    if (!exec_success(r)) {
        XLOG_E("Failed to unmount inactive partition. command: `%s`, error code: %d",
               xstring_to_string(&cmd), exec_code(r));
        xstring_free(&cmd);
        exec_free(r);
        return X_RET_ERROR;
    }

    xstring_free(&cmd);
    exec_free(r);

    return X_RET_OK;
}
