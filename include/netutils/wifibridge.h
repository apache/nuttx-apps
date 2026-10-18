/****************************************************************************
 * apps/include/netutils/wifibridge.h
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

#ifndef __APPS_INCLUDE_NETUTILS_WIFIBRIDGE_H
#define __APPS_INCLUDE_NETUTILS_WIFIBRIDGE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <stdint.h>
#include <netinet/in.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define WIFIBRIDGE_SSID_MAXLEN        32
#define WIFIBRIDGE_PASSPHRASE_MAXLEN  63

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Security of the SoftAP */

enum wifibridge_security_e
{
  WIFIBRIDGE_SEC_WPA2 = 0,    /* WPA2-Personal (PSK), CCMP */
  WIFIBRIDGE_SEC_WPA3         /* WPA3-Personal (SAE), CCMP */
};

/* The bridge, its two ports and the SoftAP parameters */

struct wifibridge_config_s
{
  FAR const char *bridge;     /* Bridge interface to create, e.g. "br0" */
  FAR const char *ethif;      /* Ethernet port, e.g. "eth0" */
  FAR const char *wlanif;     /* Wi-Fi port, e.g. "wlan0" */

  /* SoftAP SSID and passphrase (8..63 characters) */

  char ssid[WIFIBRIDGE_SSID_MAXLEN + 1];
  char passphrase[WIFIBRIDGE_PASSPHRASE_MAXLEN + 1];

  uint8_t channel;            /* 2.4 GHz channel, 1..13 */
  enum wifibridge_security_e security;
  in_addr_t ipaddr;           /* Bridge address, network byte order */
  in_addr_t netmask;          /* Bridge netmask, network byte order */
  bool dhcpd;                 /* Run the DHCP server on the bridge */
};

struct wifibridge_status_s
{
  bool exists;                /* The bridge interface exists */
  bool eth_up;                /* The Ethernet port is up */
  bool wlan_up;               /* The Wi-Fi port is up */
  struct in_addr ipaddr;      /* Bridge address */

  /* SSID the Wi-Fi port is set to */

  char ssid[WIFIBRIDGE_SSID_MAXLEN + 1];
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Name: wifibridge_start
 *
 * Description:
 *   Configure the Wi-Fi interface as a SoftAP, create the bridge, add the
 *   Ethernet and Wi-Fi interfaces to it and give the bridge its address.
 *   On failure everything done so far is undone.
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int wifibridge_start(FAR const struct wifibridge_config_s *config);

/****************************************************************************
 * Name: wifibridge_stop
 *
 * Description:
 *   Stop the DHCP server, remove the ports from the bridge, delete it and
 *   bring the Wi-Fi interface down.
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int wifibridge_stop(FAR const struct wifibridge_config_s *config);

/****************************************************************************
 * Name: wifibridge_status
 *
 * Description:
 *   Report the state of the bridge and of its ports.
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int wifibridge_status(FAR const struct wifibridge_config_s *config,
                      FAR struct wifibridge_status_s *status);

#ifdef CONFIG_NETUTILS_WIFIBRIDGE_PERSIST

/****************************************************************************
 * Name: wifibridge_save
 *
 * Description:
 *   Keep the SSID, passphrase, channel, security, address and DHCP server
 *   option of the configuration in the storage
 *   (CONFIG_NETUTILS_WIFIBRIDGE_STORAGE), to survive a reset.
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int wifibridge_save(FAR const struct wifibridge_config_s *config);

/****************************************************************************
 * Name: wifibridge_load
 *
 * Description:
 *   Read what wifibridge_save() kept and set the matching fields of the
 *   configuration.  The other fields of the configuration are left alone.
 *
 * Returned Value:
 *   Zero (OK) on success; -ENOENT if nothing is saved, or another negated
 *   errno value on failure.
 *
 ****************************************************************************/

int wifibridge_load(FAR struct wifibridge_config_s *config);

/****************************************************************************
 * Name: wifibridge_clear
 *
 * Description:
 *   Forget what wifibridge_save() kept.
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int wifibridge_clear(void);

#endif /* CONFIG_NETUTILS_WIFIBRIDGE_PERSIST */

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_NETUTILS_WIFIBRIDGE_H */
