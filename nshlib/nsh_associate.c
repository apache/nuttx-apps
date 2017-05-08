/****************************************************************************
 * apps/nshlib/nsh_associate.c
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <unistd.h>
#include <string.h>

#include <nuttx/wireless/wireless.h>

#include "wireless/wapi.h"
#include "nsh.h"

#ifdef CONFIG_WIRELESS_WAPI

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_associate
 ****************************************************************************/

int nsh_associate(FAR const char *ifname)
{
  static const char ssid[]       = CONFIG_NSH_WAPI_SSID;
  static const char passphrase[] = CONFIG_NSH_WAPI_PASSPHRASE;
  struct wpa_wconfig_s wconfig;
  int ret;

  /* Set up the network configuration */

  wconfig.sta_mode    = CONFIG_NSH_WAPI_STAMODE;
  wconfig.auth_wpa    = CONFIG_NSH_WAPI_AUTHWPA;
  wconfig.cipher_mode = CONFIG_NSH_WAPI_CIPHERMODE;
  wconfig.alg         = CONFIG_NSH_WAPI_ALG;
  wconfig.ifname      = ifname;
  wconfig.ssid        = (FAR const uint8_t *)ssid;
  wconfig.passphrase  = (FAR const uint8_t *)passphrase;

  wconfig.ssidlen     = strlen(ssid);
  wconfig.phraselen   = strlen(passphrase);

  /* Associate */

  sleep(2);
  ret = wpa_driver_wext_associate(&wconfig);
  sleep(2);
  return ret;
}

#endif /* CONFIG_WIRELESS_WAPI */
