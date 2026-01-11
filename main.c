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

int main(int argc, char** argv){
    signal(SIGINT, sigint_handler);

    xoption root = xoption_create_root();
    xoption_set_prefix_prompt(root, CLI_PROMPT);

    // Add global flag
    xoption_add_action(root, 'v', "version", "Show version information.", show_version, NULL);

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
    printf("%s\n", get_version());
    xoption_done(context, xFALSE, NULL);
}

