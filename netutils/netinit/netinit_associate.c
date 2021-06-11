/****************************************************************************
 * apps/netutils/netinit/netinit_associate.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
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
