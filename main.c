#define XLOG_MOD "main"
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include "main.h"
#include "xoption.h"
#include "version.h"
#include "checkout.h"
#include "upgrade.h"

static void sigint_handler(int sig);
static void show_version(xoption context, void* user_data);
static void show_full_version(xoption context, void* user_data);

int main(int argc, char** argv){
    signal(SIGINT, sigint_handler);

    xoption root = xoption_create_root();
    xoption_set_prefix_prompt(root, CLI_PROMPT);

    // Add global flag
    xoption_add_action(root, 'v', "", "Show version information.", show_version, NULL);
    xoption_add_action(root, '\0', "version", "Show full version information.", show_full_version, NULL);

    // Add subcommand
    checkout_usage_init(root);
    upgrade_usage_init(root);

    err_t err = xoption_parse(root, argc, argv);
    xoption_destroy(root);

    return err;
}

void sigint_handler(int sig) {
    exit(sig);
}

void show_version(xoption context, void* user_data) {
    printf("%d.%d.%d-%s\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, GIT_DESCRIBE);

    xoption_done(context, xFALSE, NULL);
}

void show_full_version(xoption context, void* user_data) {
    printf("IOTA Version %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    printf("Git Branch:       %s\n", GIT_BRANCH);
    printf("Git Commit Hash:  %s\n", GIT_COMMIT_HASH);
    printf("Git Commit Date:  %s\n", GIT_COMMIT_DATE);
    printf("Git Describe:     %s\n", GIT_DESCRIBE);
    printf("Build Time:       %s\n", BUILD_TIME);
    printf("Build Type:       %s\n", BUILD_TYPE);

    xoption_done(context, xFALSE, NULL);
}

