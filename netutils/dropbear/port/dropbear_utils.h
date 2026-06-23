/****************************************************************************
 * apps/netutils/dropbear/port/dropbear_utils.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __APPS_NETUTILS_DROPBEAR_PORT_DROPBEAR_UTILS_H
#define __APPS_NETUTILS_DROPBEAR_PORT_DROPBEAR_UTILS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "config.h"

#include <stddef.h>
#include <stdint.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void dropbear_hex_encode(FAR char *dst, FAR const uint8_t *src,
                         size_t srclen);

int dropbear_hex_decode(FAR const char *src, size_t srclen,
                        FAR uint8_t *dst, size_t dstlen);

int dropbear_try_prepare_parent(FAR const char *path);

#endif /* __APPS_NETUTILS_DROPBEAR_PORT_DROPBEAR_UTILS_H */
