/**
 * @brief iota checkout command implementation.
 *  This module implements the "iota checkout" subcommand, which is used to
 *  switch the target boot partition for the next system startup in an A/B
 *  partition environment.
 *
 * e.g.
 *  - iota checkout
 *  - iota checkout --reboot
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

#include "xoptions.h"

err_t checkout_usage_init(xoptions root);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CHECKOUT_H_ */
