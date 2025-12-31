#define XLOG_MOD "checkout"
#include "main.h"
#include "checkout.h"
#include "os_thread.h"
#include "os_file.h"
#include "xlog.h"
#include "exec.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define UBOOTENV_VAR_ROOTFS_PART "rootfs_part"
#define UBOOTENV_VAR_ROOTFS_AVAIL_PARTS "rootfs_avail_parts"

static char *checkout_script = NULL;
static xbool_t checkout_reboot = xFALSE;
static int checkout_feature_entry();
static void use_checkout_feature(xoptions context)
{  register_feature_function(checkout_feature_entry);  }

err_t checkout_usage_init(xoptions root) {
    if (!root)
        return X_RET_INVAL;

    xoptions checkout = xoptions_create_subcommand(root, "checkout", "Checkout to another partition.");
    xoptions_set_posthook(checkout, use_checkout_feature);
    xoptions_add_string(checkout, 'x', "script", "<script.sh>", "The script to execute before checkout", &checkout_script, xFALSE);
    xoptions_add_boolean(checkout, '\0', "reboot", "Reboot after checkout", &checkout_reboot); 

    return X_RET_OK;
}

static void assert_command(const char *cmd) {
#ifdef __APPLE__
#else
    xstring c = xstring_init_format("which %s", cmd);
    exec_t r = exec_command(xstring_to_string(&c));
    if (!exec_success(r)) {
        XLOG_E("fw_printenv not found in PATH.");
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
    if (!checkout_script)
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
    if (!checkout_reboot) 
        return;

    XLOG_W("Rebooting system after 3 seconds...");
    os_sleep(3);
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

    run_script_with_check(checkout_script);

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
