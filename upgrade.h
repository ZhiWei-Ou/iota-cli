/**
 * @brief iota-cli upgrade -i <firmware.iota> [--skip-auth] [--verify <public_key.pem>] [--in-place] [--reboot]
 * Usage:
 *   - iota-cli upgrade -i firmware.iota --verify public_key.pem
 *       install firmware to inactive partition with signature verification
 *   - iota-cli upgrade -i firmware.iota --skip-verify
 *       install firmware to inactive partition without signature verification
 *   - iota-cli upgrade -i firmware.iota --in-place --skip-verify
 *       install firmware in-place without signature verification
 * @file upgrade.h
 * @author Oswin
 * @date 2025-12-26
 * @details
 */
#ifndef UPDATE_H_
#define UPDATE_H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "xoption.h"

err_t upgrade_usage_init(xoption root);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* UPDATE_H_ */
