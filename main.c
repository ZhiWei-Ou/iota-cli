#define XLOG_MOD "main"
#include <stdio.h>
#include "main.h"
#include "xlog.h"
#include "xoptions.h"
#include "version.h"
#include "checkout.h"
#include "upgrade.h"

xbool_t log_trace = xFALSE;
xbool_t log_debug = xFALSE;
static void show_version(xoptions context, void* user_data);

int (*feature_entry)() = NULL;
void register_feature_function(int (*entry)())
{ feature_entry = entry; }

int main(int argc, char** argv){
    xoptions root = xoptions_create_root();
    xoptions_set_prefix_prompt(root, CLI_PROMPT);

    // Add global options
    xoptions_add_boolean(root, 'V', "verbose", "Enable verbose logging", &log_trace);
    xoptions_add_boolean(root, 'D', "debug", "Enable debug logging", &log_debug);
    xoptions_add_action(root, 'v', "version", "Show version information.", show_version, NULL);

    // Add features
    checkout_usage_init(root);
    upgrade_usage_init(root);

    err_t parse_err = xoptions_parse(root, argc, argv);
    xoptions_destroy(root);

    if (parse_err != X_RET_OK) return parse_err;

    if (log_debug) xlog_global_set_lvl(XLOG_LVL_DEBUG);
    if (log_trace) xlog_global_set_lvl(XLOG_LVL_TRACE);

    // Use the specified function
    if (feature_entry) return feature_entry();

    return X_RET_OK;
}

void show_version(xoptions context, void* user_data) {
    printf("%s\n", get_version());
    xoptions_done(context, xFALSE, NULL);
}

