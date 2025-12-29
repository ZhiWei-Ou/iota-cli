/**
 * @brief 
 * @file checkout.c
 * @author Oswin
 * @date 2025-12-26
 * @details
 *
 * @copyright (c) 2025 Oswin. All Rights Reserved.
 */
#include "os_thread.h"
#include "os_file.h"
#include "xlog.h"
#include "exec.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern char *checkout_script;
extern xbool_t checkout_reboot;

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
    assert_command("fw_setenv");
    assert_command("fw_printenv");
    assert_command("reboot");
}

static void run_script_with_check(const char *script) {
    if (!checkout_script)
        return;

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
    const char *another_part = NULL;

    if (strncmp(part, "ubi0:rootfs_a", strlen("ubi0:rootfs_a")) == 0) {
        another_part = "ubi0:rootfs_b";
    } else if (strncmp(part, "ubi0:rootfs_b", strlen("ubi0:rootfs_b")) == 0) {
        another_part = "ubi0:rootfs_a";
    } else {
        XLOG_E("Invalid partition: %s", part);
        return -1;
    }

    xstring cmd = xstring_init_format("fw_setenv rootfs_root %s", another_part);
    exec_t r = exec_command(xstring_to_string(&cmd));
    xstring_free(&cmd);

    if (!exec_success(r)) {
        XLOG_E("Failed to set rootfs_root to %s.", another_part);
        exec_free(r);
        return -1;
    }

    exec_free(r);

    XLOG_I("Checked out to partition: %s", another_part);

    run_script_with_check(checkout_script);

    reboot_with_check();

    return 0;
}

int checkout_feature_entry() {
    assert_requirements();

    char *part = NULL;

    exec_t r = exec_command("fw_printenv rootfs_root");
    if (!exec_success(r)) {
        XLOG_E("Failed to get current rootfs_root. output: %s", exec_output(r));
        exec_free(r);
        return -1;
    } else {
        const char *token = NULL;
        int len = xstring_tokenize_by_substr(&r.output, "rootfs_root=", &token);
        asprintf(&part, "%.*s", len, token);
    }
    exec_free(r);

    int code = checkout(part);
    free(part);

    return code;
}
