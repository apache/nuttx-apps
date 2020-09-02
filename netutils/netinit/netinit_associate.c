/****************************************************************************
 * apps/netutils/netinit/netinit_associate.c
 *
 *   Copyright (C) 2017, 2019 Gregory Nutt. All rights reserved.
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
#include <errno.h>

#include <nuttx/wireless/wireless.h>

#include "wireless/wapi.h"
#include "netutils/netinit.h"

#ifdef CONFIG_WIRELESS_WAPI

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netinit_associate
 ****************************************************************************/

int netinit_associate(FAR const char *ifname)
{
  struct wpa_wconfig_s conf;
  int ret = -EINVAL;
  FAR void *load;

  load = wapi_load_config(ifname, NULL, &conf);
  if (!load)
    {
      conf.ifname      = ifname;
      conf.sta_mode    = CONFIG_NETINIT_WAPI_STAMODE;
      conf.auth_wpa    = CONFIG_NETINIT_WAPI_AUTHWPA;
      conf.cipher_mode = CONFIG_NETINIT_WAPI_CIPHERMODE;
      conf.alg         = CONFIG_NETINIT_WAPI_ALG;
      conf.ssid        = CONFIG_NETINIT_WAPI_SSID;
      conf.passphrase  = CONFIG_NETINIT_WAPI_PASSPHRASE;
      conf.ssidlen     = strlen(conf.ssid);
      conf.phraselen   = strlen(conf.passphrase);
      conf.bssid       = NULL;
    }

  if (conf.ssidlen > 0)
    {
      ret = wpa_driver_wext_associate(&conf);
    }

  wapi_unload_config(load);

  return ret;
}

#endif /* CONFIG_WIRELESS_WAPI */
