/**
 * @brief 
 *  - iota update -i <firmware.iota> [--skip-auth] [--verify <public_key.pem>] [--in-place] [--reboot]
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

#include "xoptions.h"

err_t upgrade_usage_init(xoptions root);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* UPDATE_H_ */
