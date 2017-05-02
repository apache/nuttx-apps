/****************************************************************************
 * apps/include/wireless/wapi.h
 * WPA Supplicant - wext exported functions
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
 *   Author: Simon Piriou <spiriou31@gmail.com>
 *           Gregory Nutt <gnutt@nuttx.org>
 *
 * Adapted to NuttX from driver_ext.h
 *
 *   Copyright (c) 2003-2005, Jouni Malinen <j@w1.fi>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ************************************************************************************/

#ifndef __APPS_INCLUDE_WIRELESS_WEXT_H
#define __APPS_INCLUDE_WIRELESS_WEXT_H

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <nuttx/config.h>
#include <stdint.h>

/************************************************************************************
 * Pre-processor Definitions
 ************************************************************************************/

#ifdef CONFIG_NET_UDP
# define NETLIB_SOCK_IOCTL SOCK_DGRAM
#else
# define NETLIB_SOCK_IOCTL SOCK_STREAM
#endif

#define SSID_MAX_LEN 32
#define PF_INETX     PF_INET

/************************************************************************************
 * Public Types
 ************************************************************************************/

enum wpa_alg
{
  WPA_ALG_NONE,
  WPA_ALG_WEP,
  WPA_ALG_TKIP,
  WPA_ALG_CCMP,
  WPA_ALG_IGTK,
  WPA_ALG_PMK,
  WPA_ALG_GCMP,
  WPA_ALG_SMS4,
  WPA_ALG_KRK,
  WPA_ALG_GCMP_256,
  WPA_ALG_CCMP_256,
  WPA_ALG_BIP_GMAC_128,
  WPA_ALG_BIP_GMAC_256,
  WPA_ALG_BIP_CMAC_256
};

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/

int wpa_driver_wext_set_ssid(int sockfd, FAR char *ifname,
                             FAR const uint8_t *ssid, size_t ssid_len);

int wpa_driver_wext_set_mode(int sockfd, FAR char *ifname, int mode);

int wpa_driver_wext_set_key_ext(int sockfd, FAR char *ifname, enum wpa_alg alg,
                                FAR const uint8_t *key, size_t key_len);

int wpa_driver_wext_associate(void);

int wpa_driver_wext_set_auth_param(int sockfd, FAR char *ifname,
                                   int idx, uint32_t value);

#endif /* __APPS_INCLUDE_WIRELESS_WEXT_H */
