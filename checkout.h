/**
 * @brief iota checkout command implementation.
 *  This module implements the "iota checkout" subcommand, which is used to
 *  switch the target boot partition for the next system startup in an A/B
 *  partition environment.
 *
 * e.g.
 *  - iota-cli checkout
 *  - iota-cli checkout --reboot
 *
 * @file checkout.h
 * @author Oswin
 * @date 2025-12-26
 * @details
 */
#ifndef CHECKOUT_H_
#define CHECKOUT_H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define UBOOTENV_VAR_ROOTFS_PART "rootfs_part"
#define UBOOTENV_VAR_ROOTFS_AVAIL_PARTS "rootfs_avail_parts"
#define INACTIVE_PARTITION_MOUNT_POINT "/mnt/inactive_partition"

#include "xstring.h"
#include "xoption.h"

err_t checkout_usage_init(xoption root);

xstring get_inactive_partition(void);
xstring get_active_partition(void);
err_t mount_inactive_partition(void);
err_t unmount_inactive_partition(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CHECKOUT_H_ */
