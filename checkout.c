#define XLOG_MOD "checkout"
#include "main.h"
#include "checkout.h"
#include "os_file.h"
#include "xlog.h"
#include "exec.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static char *specified_script = NULL;
static xbool_t need_reboot = xFALSE;
static int reboot_delay_second = 3;
static int checkout_feature_entry();
static void use_checkout_feature(xoptions context)
{  register_feature_function(checkout_feature_entry);  }

err_t checkout_usage_init(xoptions root) {
    if (!root)
        return X_RET_INVAL;

    xoptions checkout = xoptions_create_subcommand(root, "checkout", "Select another partition for the next boot.");
    xoptions_set_posthook(checkout, use_checkout_feature);
    xoptions_add_string(checkout, 'x', "script", "<script.sh>", "Custom shell script to run after the partition switch", &specified_script, xFALSE);
    xoptions_add_boolean(checkout, '\0', "reboot", "Automatically restart the system after a successful checkout", &need_reboot); 
    xoptions_add_number(checkout, '\0', "delay", "<seconds>", "Time to wait (in seconds) before performing the reboot", &reboot_delay_second, xFALSE);

    return X_RET_OK;
}

static void assert_command(const char *cmd) {
#ifdef __APPLE__
#else
    xstring c = xstring_init_format("which %s", cmd);
    exec_t r = exec_command(xstring_to_string(&c));
    if (!exec_success(r)) {
        XLOG_E("fw_printenv not found in PATH");
        exit(-1);
    }

    exec_free(r);
    xstring_free(&c);
#endif /* __APPLE__ */
}

void assert_requirements() {
    XLOG_D("Checking requirements for checkout feature...");

    assert_command("fw_setenv");
    assert_command("fw_printenv");
    assert_command("reboot");
}

static void run_script_with_check(const char *script) {
    if (!specified_script)
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

static void reboot_with_check() {
    if (!need_reboot) 
        return;

    XLOG_W("Rebooting system after %d seconds...", reboot_delay_second);
    sleep(reboot_delay_second);
    exec_t r = exec_command("reboot");
    if (!exec_success(r)) {
        XLOG_E("Failed to reboot the system. return code: %d", r.code);
    }

    exec_free(r);
}

int checkout(const char *part) {
    xstring cmd = xstring_init_format("fw_setenv %s %s", UBOOTENV_VAR_ROOTFS_PART, part);
    exec_t r = exec_command(xstring_to_string(&cmd));
    xstring_free(&cmd);

    if (!exec_success(r)) {
        XLOG_E("Failed to set rootfs_root to '%s'", part);
        exec_free(r);
        return -1;
    }

    exec_free(r);

    XLOG_D("Checked out to partition: '%s'", part);

    XLOG_I("Partition switching successful");

    run_script_with_check(specified_script);

    reboot_with_check();

    return 0;
}

int checkout_feature_entry() {
    assert_requirements();

#if 0
    // Get available partitions
    exec_t avails = exec_command("fw_printenv -n rootfs_avail_parts");
    if (!exec_success(avails)) {
        XLOG_E("Failed to get available rootfs parts. output: %s", exec_output(avails));
        exec_free(avails);
        return -1;
    }

    // Trim output
    xstring_trim(&avails.output);
    exec_free(avails);
#endif

    // Get current rootfs_part
    exec_t current_part = exec_command("fw_printenv -n " UBOOTENV_VAR_ROOTFS_PART);
    if (!exec_success(current_part)) {
        XLOG_E("Failed to get current rootfs_root. output: %s", exec_output(current_part));
        exec_free(current_part);
        return -1;
    }

    // Remove whitespace and breaklines
    const char *p = xstring_trim(&current_part.output);
    if (!p || strlen(p) == 0) {
        XLOG_E("Current rootfs_part is empty (No expect).");
        exec_free(current_part);
        return -1;
    }

    XLOG_D("Current partition: '%s'", p);

    if (!strcmp(p, "a")) {
        p = "b";
    } else if (!strcmp(p, "b")) {
        p = "a";
    } else {
        XLOG_E("Invalid current rootfs_part: %s", p);
        exec_free(current_part);
        return -1;
    }

    int code = checkout(p);

    exec_free(current_part);

    if (code == X_RET_OK) {
        XLOG_I("Successfully checked out to partition: '%s'", p);
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
        XLOG_E("Failed to mount inactive partition. output: %s", exec_output(r));
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

    xstring cmd = xstring_init_format("umount %s && rmdir %s",
                                      INACTIVE_PARTITION_MOUNT_POINT, INACTIVE_PARTITION_MOUNT_POINT);
    exec_t r = exec_command(xstring_to_string(&cmd));
    xstring_free(&cmd);
    if (!exec_success(r)) {
        XLOG_E("Failed to unmount inactive partition. output: %s", exec_output(r));
        exec_free(r);
        return X_RET_ERROR;
    }

    exec_free(r);

    return X_RET_OK;
}
