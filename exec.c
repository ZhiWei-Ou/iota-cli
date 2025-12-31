#define XLOG_MOD "exec"
#include <stdio.h>
#include <stdlib.h>
#include "exec.h"
#include "xlog.h"

#ifdef __APPLE__
#include <string.h>
exec_t exec_command(const char *command) {
    exec_t result = {X_RET_OK, xstring_init_empty()};
    if (command == NULL) {
        return result;
    }

    if (!strncmp(command, "reboot", strlen("reboot"))) {
        XLOG_T("Simulating reboot command on macOS.");
        // exit(0);
    } else if (!strncmp(command, "fw_setenv", strlen("fw_setenv"))) {
        XLOG_T("Simulating %s command on macOS.", command);
    } else if (!strcmp(command, "fw_printenv -n rootfs_part")) {
        XLOG_T("Simulating fw_printenv command on macOS.");
        return (exec_t){0, xstring_init_format("a\n")};
    } else {
        XLOG_T("Simulating Run `%s`", command);
    }

    return result;
}

#else

exec_t exec_command(const char *command) {
    if (command == NULL) {
        return (exec_t){X_RET_INVAL, xstring_init_empty()};
    }

    exec_t result = {0};
    FILE *fp = NULL;

    fp = popen(command, "r");
    if (fp == NULL) {
        XLOG_D("Failed to run command: %s", command);
        return (exec_t){X_RET_ERROR, xstring_init_empty()};
    }

    char buf[32];
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        xstring_cat(&result.output, buf);
    }

    result.code = pclose(fp);

    return result;
}

#endif /* __APPLE__ */
