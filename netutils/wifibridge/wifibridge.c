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
#include <fcntl.h>
#include <stddef.h>
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

#ifdef CONFIG_NETUTILS_WIFIBRIDGE_PERSIST
#  include <nuttx/crc32.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define WIFIBRIDGE_SAVED_VERSION  1
#define WIFIBRIDGE_RECORD_MAGIC   0x42574657  /* 'WFWB' */

/****************************************************************************
 * Private Types
 ****************************************************************************/

#ifdef CONFIG_NETUTILS_WIFIBRIDGE_PERSIST

/* What is written to the storage: a record that is checked before it is
 * used.
 */

struct wifibridge_record_s
{
  uint32_t magic;                     /* WIFIBRIDGE_RECORD_MAGIC */
  uint32_t crc;                       /* CRC-32 of the fields below */
  uint8_t version;                    /* WIFIBRIDGE_SAVED_VERSION */
  uint8_t channel;                    /* 2.4 GHz channel, 1..13 */
  uint8_t security;                   /* enum wifibridge_security_e */
  uint8_t dhcpd;                      /* Run the DHCP server on the bridge */
  uint32_t ipaddr;                    /* Bridge address, network byte order */
  char ssid[WIFIBRIDGE_SSID_MAXLEN + 1];
  char passphrase[WIFIBRIDGE_PASSPHRASE_MAXLEN + 1];
};

#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: wifibridge_validate
 *
 * Description:
 *   Check the parameters of the SoftAP.
 *
 ****************************************************************************/

static int wifibridge_validate(FAR const struct wifibridge_config_s *cfg)
{
  size_t ssidlen = strnlen(cfg->ssid, WIFIBRIDGE_SSID_MAXLEN + 1);
  size_t passlen = strnlen(cfg->passphrase,
                           WIFIBRIDGE_PASSPHRASE_MAXLEN + 1);

  if (ssidlen < 1 || ssidlen > WIFIBRIDGE_SSID_MAXLEN || passlen < 8 ||
      passlen > WIFIBRIDGE_PASSPHRASE_MAXLEN ||
      cfg->channel < 1 || cfg->channel > 13)
    {
      return -EINVAL;
    }

  return OK;
}

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
  int version;
  int ret;

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
                                    cfg->passphrase,
                                    strlen(cfg->passphrase));
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
      config->wlanif == NULL)
    {
      return -EINVAL;
    }

  ret = wifibridge_validate(config);
  if (ret < 0)
    {
      return ret;
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

#ifdef CONFIG_NETUTILS_WIFIBRIDGE_PERSIST

/****************************************************************************
 * Name: wifibridge_record_crc
 *
 * Description:
 *   CRC-32 of the record after the CRC field.
 *
 ****************************************************************************/

static uint32_t wifibridge_record_crc(
                              FAR const struct wifibridge_record_s *record)
{
  return crc32((FAR const uint8_t *)&record->version,
               sizeof(*record) - offsetof(struct wifibridge_record_s,
                                          version));
}

/****************************************************************************
 * Name: wifibridge_record_io
 *
 * Description:
 *   Read or write the record at the start of the storage.
 *
 ****************************************************************************/

static int wifibridge_record_io(FAR struct wifibridge_record_s *record,
                                bool store)
{
  ssize_t nbytes;
  int ret = OK;
  int fd;

  fd = open(CONFIG_NETUTILS_WIFIBRIDGE_STORAGE,
            store ? O_WRONLY | O_CREAT | O_TRUNC : O_RDONLY, 0600);
  if (fd < 0)
    {
      return -errno;
    }

  if (store)
    {
      nbytes = write(fd, record, sizeof(*record));
      if (nbytes == sizeof(*record))
        {
          ret = fsync(fd);
          if (ret < 0)
            {
              ret = -errno;
            }
        }
      else
        {
          ret = nbytes < 0 ? -errno : -EIO;
        }
    }
  else
    {
      nbytes = read(fd, record, sizeof(*record));
      if (nbytes != sizeof(*record))
        {
          ret = nbytes < 0 ? -errno : -ENOENT;
        }
    }

  close(fd);
  return ret;
}

/****************************************************************************
 * Name: wifibridge_save
 ****************************************************************************/

int wifibridge_save(FAR const struct wifibridge_config_s *config)
{
  struct wifibridge_record_s record;
  int ret;

  if (config == NULL)
    {
      return -EINVAL;
    }

  ret = wifibridge_validate(config);
  if (ret < 0)
    {
      return ret;
    }

  memset(&record, 0, sizeof(record));
  record.magic    = WIFIBRIDGE_RECORD_MAGIC;
  record.version  = WIFIBRIDGE_SAVED_VERSION;
  record.channel  = config->channel;
  record.security = config->security;
  record.dhcpd    = config->dhcpd;
  record.ipaddr   = config->ipaddr;
  strlcpy(record.ssid, config->ssid, sizeof(record.ssid));
  strlcpy(record.passphrase, config->passphrase,
          sizeof(record.passphrase));
  record.crc = wifibridge_record_crc(&record);

  return wifibridge_record_io(&record, true);
}

/****************************************************************************
 * Name: wifibridge_load
 ****************************************************************************/

int wifibridge_load(FAR struct wifibridge_config_s *config)
{
  struct wifibridge_config_s old_config;
  struct wifibridge_record_s record;
  int ret;

  if (config == NULL)
    {
      return -EINVAL;
    }

  ret = wifibridge_record_io(&record, false);
  if (ret < 0)
    {
      return ret;
    }

  /* A blank or cleared storage has no record */

  if (record.magic != WIFIBRIDGE_RECORD_MAGIC)
    {
      return -ENOENT;
    }

  /* Do not trust the rest: it can be damaged or come from another version */

  if (record.crc != wifibridge_record_crc(&record) ||
      record.version != WIFIBRIDGE_SAVED_VERSION)
    {
      return -EINVAL;
    }

  /* The configuration keeps its values if the record is not valid */

  old_config = *config;

  memcpy(config->ssid, record.ssid, sizeof(config->ssid));
  config->ssid[WIFIBRIDGE_SSID_MAXLEN] = '\0';
  memcpy(config->passphrase, record.passphrase, sizeof(config->passphrase));
  config->passphrase[WIFIBRIDGE_PASSPHRASE_MAXLEN] = '\0';
  config->channel  = record.channel;
  config->security = record.security;
  config->ipaddr   = record.ipaddr;
  config->dhcpd    = record.dhcpd != 0;

  ret = wifibridge_validate(config);
  if (ret < 0)
    {
      *config = old_config;
    }

  return ret;
}

/****************************************************************************
 * Name: wifibridge_clear
 ****************************************************************************/

int wifibridge_clear(void)
{
  struct wifibridge_record_s record;

  /* A record that is not one: a storage that is a device has no way to
   * delete it.
   */

  memset(&record, 0, sizeof(record));
  return wifibridge_record_io(&record, true);
}

#endif /* CONFIG_NETUTILS_WIFIBRIDGE_PERSIST */
