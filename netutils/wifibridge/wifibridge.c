/****************************************************************************
 * apps/netutils/wifibridge/wifibridge.c
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include <sys/socket.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "netutils/netlib.h"
#include "netutils/wifibridge.h"
#include "wireless/wapi.h"

#ifdef CONFIG_NETUTILS_WIFIBRIDGE_DHCPD
#  include "netutils/dhcpd.h"
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: wifibridge_setup_softap
 *
 * Description:
 *   Put the Wi-Fi interface in master mode with the requested security,
 *   channel and SSID.
 *
 ****************************************************************************/

static int wifibridge_setup_softap(int sock,
                                   FAR const struct wifibridge_config_s *cfg)
{
  size_t passlen = strnlen(cfg->passphrase, 64);
  int version;
  int ret;

  if (passlen < 8 || passlen > 63 || cfg->channel < 1 || cfg->channel > 13)
    {
      return -EINVAL;
    }

  version = cfg->security == WIFIBRIDGE_SEC_WPA3 ?
            IW_AUTH_WPA_VERSION_WPA3 : IW_AUTH_WPA_VERSION_WPA2;

  ret = wapi_set_mode(sock, cfg->wlanif, WAPI_MODE_MASTER);
  if (ret < 0)
    {
      return ret;
    }

  ret = wpa_driver_wext_set_auth_param(sock, cfg->wlanif,
                                       IW_AUTH_WPA_VERSION, version);
  if (ret < 0)
    {
      return ret;
    }

  ret = wpa_driver_wext_set_auth_param(sock, cfg->wlanif,
                                       IW_AUTH_CIPHER_PAIRWISE,
                                       IW_AUTH_CIPHER_CCMP);
  if (ret < 0)
    {
      return ret;
    }

  ret = wpa_driver_wext_set_key_ext(sock, cfg->wlanif, WPA_ALG_CCMP,
                                    cfg->passphrase, passlen);
  if (ret < 0)
    {
      return ret;
    }

  ret = wapi_set_freq(sock, cfg->wlanif, cfg->channel, WAPI_FREQ_FIXED);
  if (ret < 0)
    {
      return ret;
    }

  return wapi_set_essid(sock, cfg->wlanif, cfg->ssid, WAPI_ESSID_ON);
}

/****************************************************************************
 * Name: wifibridge_teardown
 *
 * Description:
 *   Undo the steps of wifibridge_start() in reverse order.  Each step is
 *   tried even if an earlier one failed, since this also cleans up a
 *   partially started bridge.
 *
 ****************************************************************************/

static int wifibridge_teardown(int sock,
                               FAR const struct wifibridge_config_s *cfg)
{
  int ret;

#ifdef CONFIG_NETUTILS_WIFIBRIDGE_DHCPD
  dhcpd_stop();
#endif

  netlib_ifdown(cfg->bridge);

  netlib_bridge_delif(cfg->bridge, cfg->wlanif);
  netlib_bridge_delif(cfg->bridge, cfg->ethif);

  ret = netlib_del_bridge(cfg->bridge);

  /* Bringing the interface down does not stop the AP, an ESSID set with the
   * off flag does.
   */

  wapi_set_essid(sock, cfg->wlanif, "", WAPI_ESSID_OFF);
  wapi_set_ifdown(sock, cfg->wlanif);

  return ret;
}

/****************************************************************************
 * Name: wifibridge_isup
 ****************************************************************************/

static bool wifibridge_isup(int sock, FAR const char *ifname)
{
  int is_up;

  return wapi_get_ifup(sock, ifname, &is_up) >= 0 && is_up != 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: wifibridge_start
 ****************************************************************************/

int wifibridge_start(FAR const struct wifibridge_config_s *config)
{
  struct in_addr none;
  struct in_addr addr;
  int sock;
  int ret;

  if (config == NULL || config->bridge == NULL || config->ethif == NULL ||
      config->wlanif == NULL || config->ssid == NULL ||
      config->passphrase == NULL)
    {
      return -EINVAL;
    }

  if (if_nametoindex(config->bridge) != 0)
    {
      return -EEXIST;
    }

  sock = wapi_make_socket();
  if (sock < 0)
    {
      return sock;
    }

  ret = wifibridge_setup_softap(sock, config);
  if (ret < 0)
    {
      goto errout;
    }

  /* The ports carry no address, only the bridge does */

  none.s_addr = INADDR_ANY;

  ret = netlib_set_ipv4addr(config->wlanif, &none);
  if (ret < 0)
    {
      goto errout;
    }

  ret = netlib_set_ipv4addr(config->ethif, &none);
  if (ret < 0)
    {
      goto errout;
    }

  /* A bridge port must be up to carry traffic */

  ret = netlib_ifup(config->ethif);
  if (ret < 0)
    {
      goto errout;
    }

  ret = netlib_ifup(config->wlanif);
  if (ret < 0)
    {
      goto errout;
    }

  ret = netlib_add_bridge(config->bridge);
  if (ret < 0)
    {
      goto errout;
    }

  ret = netlib_bridge_addif(config->bridge, config->ethif);
  if (ret < 0)
    {
      goto errout_bridge;
    }

  ret = netlib_bridge_addif(config->bridge, config->wlanif);
  if (ret < 0)
    {
      goto errout_bridge;
    }

  addr.s_addr = config->ipaddr;
  ret = netlib_set_ipv4addr(config->bridge, &addr);
  if (ret < 0)
    {
      goto errout_bridge;
    }

  addr.s_addr = config->netmask;
  ret = netlib_set_ipv4netmask(config->bridge, &addr);
  if (ret < 0)
    {
      goto errout_bridge;
    }

  ret = netlib_ifup(config->bridge);
  if (ret < 0)
    {
      goto errout_bridge;
    }

#ifdef CONFIG_NETUTILS_WIFIBRIDGE_DHCPD
  if (config->dhcpd)
    {
      ret = dhcpd_start(config->bridge);
      if (ret < 0)
        {
          goto errout_bridge;
        }
    }
#endif

  close(sock);
  return OK;

errout_bridge:
  wifibridge_teardown(sock, config);

errout:
  close(sock);
  return ret;
}

/****************************************************************************
 * Name: wifibridge_stop
 ****************************************************************************/

int wifibridge_stop(FAR const struct wifibridge_config_s *config)
{
  int sock;
  int ret;

  if (config == NULL || config->bridge == NULL || config->ethif == NULL ||
      config->wlanif == NULL)
    {
      return -EINVAL;
    }

  sock = wapi_make_socket();
  if (sock < 0)
    {
      return sock;
    }

  ret = wifibridge_teardown(sock, config);

  close(sock);
  return ret;
}

/****************************************************************************
 * Name: wifibridge_status
 ****************************************************************************/

int wifibridge_status(FAR const struct wifibridge_config_s *config,
                      FAR struct wifibridge_status_s *status)
{
  enum wapi_essid_flag_e flag;
  int sock;

  if (config == NULL || status == NULL)
    {
      return -EINVAL;
    }

  memset(status, 0, sizeof(*status));

  sock = wapi_make_socket();
  if (sock < 0)
    {
      return sock;
    }

  status->exists = netlib_get_ipv4addr(config->bridge, &status->ipaddr) >= 0;

  status->eth_up  = wifibridge_isup(sock, config->ethif);
  status->wlan_up = wifibridge_isup(sock, config->wlanif);

  wapi_get_essid(sock, config->wlanif, status->ssid, &flag);
  status->ssid[sizeof(status->ssid) - 1] = '\0';

  close(sock);
  return OK;
}
