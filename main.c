/**
 * @brief iota 
 * @file main.c
 * @author Oswin
 * @date 2025-12-26
 * @details
 */
#include <stdio.h>
#include "xlog.h"
#include "xoptions.h"
#include "version.h"
#include "checkout.h"
#include "update.h"

xbool_t verbose = xFALSE;
static void show_version(xoptions context, void* user_data);

char *checkout_script = NULL;
xbool_t checkout_reboot = xFALSE;

char *update_image = NULL;
xbool_t update_skip_checksum = xFALSE;
xbool_t update_reboot = xFALSE;

int (*feature_entry)() = NULL;
static void use_checkout_feature(xoptions context);
static void use_update_feature(xoptions context);

int main(int argc, char** argv){
    xoptions opts = xoptions_create_root();
    xoptions_add_boolean(opts, 'V', "verbose", "Enable verbose output.", &verbose);
    xoptions_add_action(opts, 'v', "version", "Show version information.", show_version, NULL);

    xoptions checkout = xoptions_create_subcommand(opts, "checkout", "Checkout to another partition.");
    xoptions_set_posthook(checkout, use_checkout_feature);
    xoptions_add_string(checkout, 'x', "script", "<script.sh>", "The script to execute before checkout.", &checkout_script, xFALSE);
    xoptions_add_boolean(checkout, '\0', "reboot", "Reboot after checkout.", &checkout_reboot); 

    xoptions update = xoptions_create_subcommand(opts, "update", "Update iota to the latest version.");
    xoptions_set_posthook(update, use_update_feature);
    xoptions_add_string(update, 'i', "image", "<image.iota>", "The image file to update.", &update_image, xTRUE);
    xoptions_add_boolean(update, '\0', "reboot", "Reboot after update.", &update_reboot);
    xoptions_add_boolean(update, '\0', "skip-checksum", "Skip checksum verification.", &update_skip_checksum);

    err_t parse_err = xoptions_parse(opts, argc, argv);
    xoptions_destroy(opts);

    if (parse_err != X_RET_OK) {
        return parse_err;
    }

    if (verbose) {
        xlog_global_set_lvl(XLOG_LVL_DEBUG);
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

static void use_update_feature(xoptions context) {
    xUNUSED(context);
    feature_entry = update_feature_entry;
}
