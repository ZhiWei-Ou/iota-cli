#define XLOG_MOD "main"
#include <stdio.h>
#include "xlog.h"
#include "xoptions.h"
#include "version.h"
#include "checkout.h"
#include "upgrade.h"

xbool_t log_trace = xFALSE;
xbool_t log_debug = xFALSE;
static void show_version(xoptions context, void* user_data);

char *checkout_script = NULL;
xbool_t checkout_reboot = xFALSE;

char *upgrade_image = NULL;
xbool_t upgrade_skip_auth_tag = xFALSE;
xbool_t upgrade_reboot = xFALSE;
xbool_t upgrade_skip_verify = xFALSE;
xbool_t upgrade_in_place = xFALSE;
char *upgrade_public_key_pem = NULL;
int upgrade_stream_count = 4096;

int (*feature_entry)() = NULL;
static void use_checkout_feature(xoptions context);
static void use_upgrade_feature(xoptions context);

int main(int argc, char** argv){
    xoptions opts = xoptions_create_root();
    xoptions_add_boolean(opts, 'V', "verbose", "Enable verbose logging", &log_trace);
    xoptions_add_boolean(opts, 'D', "debug", "Enable debug logging", &log_debug);
    xoptions_add_action(opts, 'v', "version", "Show version information.", show_version, NULL);

    xoptions checkout = xoptions_create_subcommand(opts, "checkout", "Checkout to another partition.");
    xoptions_set_posthook(checkout, use_checkout_feature);
    xoptions_add_string(checkout, 'x', "script", "<script.sh>", "The script to execute before checkout", &checkout_script, xFALSE);
    xoptions_add_boolean(checkout, '\0', "reboot", "Reboot after checkout", &checkout_reboot); 

    xoptions update = xoptions_create_subcommand(opts, "update", "Update iota to the latest version.");
    xoptions_set_posthook(update, use_upgrade_feature);
    xoptions_add_string(update, 'i', "image", "<firmware.iota>", "The image file to update", &upgrade_image, xTRUE);
    xoptions_add_boolean(update, '\0', "reboot", "Reboot after update", &upgrade_reboot);
    xoptions_add_boolean(update, '\0', "skip-auth", "Skip auth tag", &upgrade_skip_auth_tag);
    xoptions_add_boolean(update, '\0', "skip-verify", "Skip signature verification", &upgrade_skip_verify);
    xoptions_add_number(update, 's', "stream-count", "<count>", "The stream count for updating process", &upgrade_stream_count, xFALSE);
    xoptions_add_string(update, '\0', "verify", "<public_key.pem>", "The public key PEM file for signature verification", &upgrade_public_key_pem, xFALSE);
    xoptions_add_boolean(update, '\0', "in-place", "Perform in-place update", &upgrade_in_place);

    err_t parse_err = xoptions_parse(opts, argc, argv);
    xoptions_destroy(opts);

    if (parse_err != X_RET_OK) {
        return parse_err;
    }

    if (log_debug) {
        xlog_global_set_lvl(XLOG_LVL_DEBUG);
    }

    if (log_trace) {
        xlog_global_set_lvl(XLOG_LVL_TRACE);
    }

    if (feature_entry) {
        return feature_entry();
    }

    return X_RET_OK;
}

static void show_version(xoptions context, void* user_data) {
    printf("%s\n", get_version());
    xoptions_done(context, xFALSE, NULL);
}

static void use_checkout_feature(xoptions context) {
    xUNUSED(context);
    feature_entry = checkout_feature_entry;
}

static void use_upgrade_feature(xoptions context) {
    xUNUSED(context);
    feature_entry = upgrade_feature_entry;
}
