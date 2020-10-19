/****************************************************************************
 * apps/include/wireless/wapi.h
 *
 *   Copyright (C) 2017, 2019 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Adapted for NuttX from WAPI:
 *
 *   Copyright (c) 2010, Volkan YAZICI <volkan.yazici@gmail.com>
 *   All rights reserved.
 *
 * And includes WPA supplicant logic contributed by:
 *
 *   Author: Simon Piriou <spiriou31@gmail.com>
 *
 * Which was adapted to NuttX from driver_ext.h
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
 ****************************************************************************/

#ifndef __APPS_INCLUDE_WIRELESS_WAPI_H
#define __APPS_INCLUDE_WIRELESS_WAPI_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <netinet/in.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <nuttx/wireless/wireless.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Maximum allowed ESSID size. */

#define WAPI_ESSID_MAX_SIZE IW_ESSID_MAX_SIZE

/* Buffer size while reading lines from PROC_NET_ files. */

#define WAPI_PROC_LINE_SIZE  1024

/* Select options to successfully open a socket in this network
 * configuration.
 */

/* The address family that we used to create the socket really does not
 * matter.  It should, however, be valid in the current configuration.
 */

#if defined(CONFIG_NET_IPv4)
#  define PF_INETX PF_INET
#elif defined(CONFIG_NET_IPv6)
#  define PF_INETX PF_INET6
#endif

/* SOCK_DGRAM is the preferred socket type to use when we just want a
 * socket for performing driver ioctls.  However, we can't use SOCK_DRAM
 * if UDP is disabled.
 */

#ifdef CONFIG_NET_UDP
#  define SOCK_WAPI SOCK_DGRAM
#else
#  define SOCK_WAPI SOCK_STREAM
#endif

#ifndef CONFIG_WIRELESS_WAPI_INITCONF
#  define wapi_load_config(ifname, confname, conf) NULL
#  define wapi_unload_config(load)
#  define wapi_save_config(ifname, confname, conf) 0
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Frequency flags. */

enum wapi_freq_flag_e
{
  WAPI_FREQ_AUTO  = IW_FREQ_AUTO,
  WAPI_FREQ_FIXED = IW_FREQ_FIXED
};

/* Route target types. */

enum wapi_route_target_e
{
  WAPI_ROUTE_TARGET_NET,              /* The target is a network. */
  WAPI_ROUTE_TARGET_HOST              /* The target is a host. */
};

/* ESSID flags.  */

enum wapi_essid_flag_e
{
  WAPI_ESSID_OFF,
  WAPI_ESSID_ON
};

/* Supported operation modes. */

enum wapi_mode_e
{
  WAPI_MODE_AUTO    = IW_MODE_AUTO,    /* Driver decides. */
  WAPI_MODE_ADHOC   = IW_MODE_ADHOC,   /* Single cell network. */
  WAPI_MODE_MANAGED = IW_MODE_INFRA,   /* Multi cell network, roaming, ... */
  WAPI_MODE_MASTER  = IW_MODE_MASTER,  /* Synchronisation master or access point. */
  WAPI_MODE_REPEAT  = IW_MODE_REPEAT,  /* Wireless repeater, forwarder. */
  WAPI_MODE_SECOND  = IW_MODE_SECOND,  /* Secondary master/repeater, backup. */
  WAPI_MODE_MONITOR = IW_MODE_MONITOR, /* Passive monitor, listen only. */
  WAPI_MODE_MESH    = IW_MODE_MESH     /* Mesh (IEEE 802.11s) network */
};

/* Bitrate flags.
 *
 * At the moment, unicast (IW_BITRATE_UNICAST) and broadcast
 * (IW_BITRATE_BROADCAST) bitrate flags are not supported.
 */

enum wapi_bitrate_flag_e
{
  WAPI_BITRATE_AUTO,
  WAPI_BITRATE_FIXED
};

/* Transmit power (txpower) flags. */

enum wapi_txpower_flag_e
{
  WAPI_TXPOWER_DBM,                   /* Value is in dBm. */
  WAPI_TXPOWER_MWATT,                 /* Value is in mW. */
  WAPI_TXPOWER_RELATIVE               /* Value is in arbitrary units. */
};

/* Linked list container for strings. */

struct wapi_string_s
{
  FAR struct wapi_string_s *next;
  FAR char *data;
};

/* Linked list container for scan results. */

struct wapi_scan_info_s
{
  FAR struct wapi_scan_info_s *next;
  struct ether_addr ap;
  int has_essid;
  char essid[WAPI_ESSID_MAX_SIZE + 1];
  enum wapi_essid_flag_e essid_flag;
  int has_freq;
  double freq;
  int has_mode;
  enum wapi_mode_e mode;
  int has_bitrate;
  int bitrate;
  int has_rssi;
  int rssi;
};

/* Linked list container for routing table rows. */

struct wapi_route_info_s
{
  FAR struct wapi_route_info_s *next;
  FAR char *ifname;
  struct in_addr dest;
  struct in_addr gw;

  unsigned int flags;  /* See  RTF_* in  net/route.h for available values. */
  unsigned int refcnt;
  unsigned int use;
  unsigned int metric;
  struct in_addr netmask;
  unsigned int mtu;
  unsigned int window;
  unsigned int irtt;
};

/* A generic linked list container. For functions taking  struct wapi_list_s
 * type of argument, caller is responsible for releasing allocated memory.
 */

struct wapi_list_s
{
  union wapi_list_head_t
  {
    FAR struct wapi_string_s     *string;
    FAR struct wapi_scan_info_s  *scan;
    FAR struct wapi_route_info_s *route;
  } head;
};

/* WPA **********************************************************************/

enum wpa_alg_e
{
  WPA_ALG_NONE = 0,
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

/* This structure provides the wireless configuration to
 * wpa_driver_wext_associate().
 */

struct wpa_wconfig_s
{
  uint8_t sta_mode;              /* Mode of operation, e.g. IW_MODE_INFRA */
  uint8_t auth_wpa;              /* IW_AUTH_WPA_VERSION values, e.g.
                                  * IW_AUTH_WPA_VERSION_WPA2 */
  uint8_t cipher_mode;           /* IW_AUTH_PAIRWISE_CIPHER and
                                  * IW_AUTH_GROUP_CIPHER values, e.g.,
                                  * IW_AUTH_CIPHER_CCMP */
  uint8_t alg;                   /* See enum wpa_alg_e above, e.g.
                                  * WPA_ALG_CCMP */
  uint8_t ssidlen;               /* Length of the SSID */
  uint8_t phraselen;             /* Length of the passphrase */
  FAR const char *ifname;        /* E.g., "wlan0" */
  FAR const char *ssid;          /* E.g., "myApSSID" */
  FAR const char *bssid;         /* Options to associate with bssid */
  FAR const char *passphrase;    /* E.g., "mySSIDpassphrase" */
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

 #ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/* Frequency flag names. */

EXTERN FAR const char *g_wapi_freq_flags[];

/* ESSID flag names. */

EXTERN FAR const char *g_wapi_essid_flags[];

/* Passphrase algorithm flag names. */

EXTERN FAR const char *g_wapi_alg_flags[];

/* Supported operation mode names. */

EXTERN FAR const char *g_wapi_modes[];

/* Bitrate flag names. */

EXTERN FAR const char *g_wapi_bitrate_flags[];

/* Transmit power flag names. */

EXTERN FAR const char *g_wapi_txpower_flags[];

/****************************************************************************
 * Public Function Prototyppes
 ****************************************************************************/

/****************************************************************************
 * Name: wapi_get_ifup
 *
 * Description:
 *   Gets the interface up status.
 *
 * Input Parameters:
 *   is_up Set to 0, if up; 1, otherwise.
 *
 ****************************************************************************/

int wapi_get_ifup(int sock, FAR const char *ifname, FAR int *is_up);

/****************************************************************************
 * Name: wapi_set_ifup
 *
 * Description:
 *   Activates the interface.
 *
 ****************************************************************************/

int wapi_set_ifup(int sock, FAR const char *ifname);

/****************************************************************************
 * Name: wapi_set_ifdown
 *
 * Description:
 *   Shuts down the interface.
 *
 ****************************************************************************/

int wapi_set_ifdown(int sock, FAR const char *ifname);

/****************************************************************************
 * Name: wapi_get_ip
 *
 * Description:
 *   Gets IP address of the given network interface.
 *
 ****************************************************************************/

int wapi_get_ip(int sock, FAR const char *ifname, struct in_addr *addr);

/****************************************************************************
 * Name: wapi_set_ip
 *
 * Description:
 *   Sets IP address of the given network interface.
 *
 ****************************************************************************/

int wapi_set_ip(int sock, FAR const char *ifname,
                FAR const struct in_addr *addr);

/****************************************************************************
 * Name: wapi_get_netmask
 *
 * Description:
 *   Gets netmask of the given network interface.
 *
 ****************************************************************************/

int wapi_get_netmask(int sock, FAR const char *ifname,
                     FAR struct in_addr *addr);

/****************************************************************************
 * Name: wapi_set_netmask
 *
 * Description:
 *   Sets netmask of the given network interface.
 *
 ****************************************************************************/

int wapi_set_netmask(int sock, FAR const char *ifname,
                     FAR const struct in_addr *addr);

/****************************************************************************
 * Name: wapi_add_route_gw
 *
 * Description:
 *   Adds gateway for the given target network.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_ROUTE
int wapi_add_route_gw(int sock, enum wapi_route_target_e targettype,
                      FAR const struct in_addr *target,
                      FAR const struct in_addr *netmask,
                      FAR const struct in_addr *gw);
#endif

/****************************************************************************
 * Name: wapi_del_route_gw
 *
 * Description:
 *   Deletes gateway for the given target network.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_ROUTE
int wapi_del_route_gw(int sock, enum wapi_route_target_e targettype,
                      FAR const struct in_addr *target,
                      FAR const struct in_addr *netmask,
                      FAR const struct in_addr *gw);
#endif

/****************************************************************************
 * Name: wapi_get_freq
 *
 * Description:
 *   Gets the operating frequency of the device.
 *
 ****************************************************************************/

int wapi_get_freq(int sock, FAR const char *ifname, FAR double *freq,
                  FAR enum wapi_freq_flag_e *flag);

/****************************************************************************
 * Name: wapi_set_freq
 *
 * Description:
 *   Sets the operating frequency of the device.
 *
 ****************************************************************************/

int wapi_set_freq(int sock, FAR const char *ifname, double freq,
                  enum wapi_freq_flag_e flag);

/****************************************************************************
 * Name: wapi_freq2chan
 *
 * Description:
 *   Finds corresponding channel for the supplied freq.
 *
 * Returned Value:
 *   0, on success; -2, if not found; otherwise, ioctl() return value.
 *
 ****************************************************************************/

int wapi_freq2chan(int sock, FAR const char *ifname, double freq,
                   FAR int *chan);

/****************************************************************************
 * Name: wapi_chan2freq
 *
 * Description:
 *   Finds corresponding frequency for the supplied chan.
 *
 * Returned Value:
 *   0, on success; -2, if not found; otherwise, ioctl() return value.
 *
 ****************************************************************************/

int wapi_chan2freq(int sock, FAR const char *ifname, int chan,
                   FAR double *freq);

/****************************************************************************
 * Name: wapi_get_essid
 *
 * Description:
 *   Gets ESSID of the device.
 *
 * Input Parameters:
 *   essid - Used to store the ESSID of the device. Buffer must have
 *           enough space to store WAPI_ESSID_MAX_SIZE+1 characters.
 *
 ****************************************************************************/

int wapi_get_essid(int sock, FAR const char *ifname, FAR char *essid,
                   FAR enum wapi_essid_flag_e *flag);

/****************************************************************************
 * Name: wapi_set_essid
 *
 * Description:
 *    Sets ESSID of the device.
 *
 *    essid At most WAPI_ESSID_MAX_SIZE characters are read.
 *
 ****************************************************************************/

int wapi_set_essid(int sock, FAR const char *ifname, FAR const char *essid,
                   enum wapi_essid_flag_e flag);

/****************************************************************************
 * Name: wapi_get_mode
 *
 * Description:
 *   Gets the operating mode of the device.
 *
 ****************************************************************************/

int wapi_get_mode(int sock, FAR const char *ifname,
                  FAR enum wapi_mode_e *mode);

/****************************************************************************
 * Name: wapi_set_mode
 *
 * Description:
 *   Sets the operating mode of the device.
 *
 ****************************************************************************/

int wapi_set_mode(int sock, FAR const char *ifname, enum wapi_mode_e mode);

/****************************************************************************
 * Name: wapi_make_broad_ether
 *
 * Description:
 *   Creates an ethernet broadcast address.
 *
 ****************************************************************************/

int wapi_make_broad_ether(FAR struct ether_addr *sa);

/****************************************************************************
 * Name: wapi_make_null_ether
 *
 * Description:
 *   Creates an ethernet NULL address.
 *
 ****************************************************************************/

int wapi_make_null_ether(FAR struct ether_addr *sa);

/****************************************************************************
 * Name: wapi_get_ap
 *
 * Description:
 *   Gets access point address of the device.
 *
 * Input Parameters:
 *   ap - Set the to MAC address of the device. (For "any", a broadcast
 *        ethernet address; for "off", a null ethernet address is used.)
 *
 ****************************************************************************/

int wapi_get_ap(int sock, FAR const char *ifname, FAR struct ether_addr *ap);

/****************************************************************************
 * Name: wapi_set_ap
 *
 * Description:
 *   Sets access point address of the device.
 *
 ****************************************************************************/

int wapi_set_ap(int sock, FAR const char *ifname,
                FAR const struct ether_addr *ap);

/****************************************************************************
 * Name: wapi_get_bitrate
 *
 * Description:
 *   Gets bitrate of the device.
 *
 ****************************************************************************/

int wapi_get_bitrate(int sock, FAR const char *ifname,
                     FAR int *bitrate, FAR enum wapi_bitrate_flag_e *flag);

/****************************************************************************
 * Name: wapi_set_bitrate
 *
 * Description:
 *   Sets bitrate of the device.
 *
 ****************************************************************************/

int wapi_set_bitrate(int sock, FAR const char *ifname, int bitrate,
                     enum wapi_bitrate_flag_e flag);

/****************************************************************************
 * Name: wapi_dbm2mwatt
 *
 * Description:
 *   Converts a value in dBm to a value in milliWatt.
 *
 ****************************************************************************/

int wapi_dbm2mwatt(int dbm);

/****************************************************************************
 * Name: wapi_mwatt2dbm
 *
 * Description:
 *   Converts a value in milliWatt to a value in dBm.
 *
 ****************************************************************************/

int wapi_mwatt2dbm(int mwatt);

/****************************************************************************
 * Name: wapi_get_txpower
 *
 * Description:
 *   Gets txpower of the device.
 *
 ****************************************************************************/

int wapi_get_txpower(int sock, FAR const char *ifname, FAR int *power,
                     FAR enum wapi_txpower_flag_e *flag);

/****************************************************************************
 * Name: wapi_set_txpower
 *
 * Description:
 *   Sets txpower of the device.
 *
 ****************************************************************************/

int wapi_set_txpower(int sock, FAR const char *ifname, int power,
                     enum wapi_txpower_flag_e flag);

/****************************************************************************
 * Name: wapi_make_socket
 *
 * Description:
 *   Creates an AF_INET socket to be used in ioctl() calls.
 *
 * Returned Value:
 *   Non-negative on success.
 *
 ****************************************************************************/

int wapi_make_socket(void);

/****************************************************************************
 * Name: wapi_scan_init
 *
 * Description:
 *   Starts a scan on the given interface. Root privileges are required to
 *   start a scan.
 *
 ****************************************************************************/

int wapi_scan_init(int sock, FAR const char *ifname, FAR const char *essid);

/****************************************************************************
 * Name: wapi_scan_channel_init
 *
 * Description:
 *   Starts a scan on the given interface. Root privileges are required to
 *   start a scan with specified channels.
 *
 ****************************************************************************/

int wapi_scan_channel_init(int sock, FAR const char *ifname,
                           FAR const char *essid,
                           uint8_t *channels, int num_channels);

/****************************************************************************
 * Name: wapi_scan_stat
 *
 * Description:
 *   Checks the status of the scan process.
 *
 * Returned Value:
 *   Zero, if data is ready; 1, if data is not ready; negative on failure.
 *
 ****************************************************************************/

int wapi_scan_stat(int sock, FAR const char *ifname);

/****************************************************************************
 * Name: wapi_scan_coll
 *
 * Description:
 *   Collects the results of a scan process.
 *
 * Input Parameters:
 *   aps - Pushes collected  struct wapi_scan_info_s into this list.
 *
 ****************************************************************************/

int wapi_scan_coll(int sock, FAR const char *ifname,
                   FAR struct wapi_list_s *aps);

/****************************************************************************
 * Name: wapi_scan_coll_free
 *
 * Description:
 *   Free the scan results.
 *
 * Input Parameters:
 *   aps - Release the collected struct wapi_scan_info_s.
 *
 ****************************************************************************/

void wapi_scan_coll_free(FAR struct wapi_list_s *aps);

#ifdef CONFIG_WIRELESS_WAPI_INITCONF
/****************************************************************************
 * Name: wapi_load_config
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *   Return a pointer to the hold the config resource, NULL On error.
 *
 ****************************************************************************/

FAR void *wapi_load_config(FAR const char *ifname,
                           FAR const char *confname,
                           FAR struct wpa_wconfig_s *conf);

/****************************************************************************
 * Name: wapi_unload_config
 *
 * Description:
 *
 * Input Parameters:
 *  load - Config resource handler, allocate by wapi_load_config()
 *
 * Returned Value:
 *
 ****************************************************************************/

void wapi_unload_config(FAR void *load);

/****************************************************************************
 * Name: wapi_save_config
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

int wapi_save_config(FAR const char *ifname,
                     FAR const char *confname,
                     FAR const struct wpa_wconfig_s *conf);
#endif

/****************************************************************************
 * Name: wpa_driver_wext_set_key_ext
 *
 * Description:
 *
 * Input Parameters:
 *   sockfd - Opened network socket
 *   ifname - Interface name
 *
 * Returned Value:
 *
 ****************************************************************************/

int wpa_driver_wext_set_key_ext(int sockfd, FAR const char *ifname,
                                enum wpa_alg_e alg, FAR const char *key,
                                size_t key_len);

/****************************************************************************
 * Name: wpa_driver_wext_get_key_ext
 *
 * Description:
 *
 * Input Parameters:
 *   sockfd - Opened network socket
 *   ifname - Interface name
 *
 * Returned Value:
 *
 ****************************************************************************/

int wpa_driver_wext_get_key_ext(int sockfd, FAR const char *ifname,
                                enum wpa_alg_e *alg, FAR char *key,
                                size_t *req_len);

/****************************************************************************
 * Name: wpa_driver_wext_associate
 *
 * Description:
 *
 * Input Parameters:
 *   wconfig - Describes the wireless configuration.
 *
 * Returned Value:
 *
 ****************************************************************************/

int wpa_driver_wext_associate(FAR struct wpa_wconfig_s *wconfig);

/****************************************************************************
 * Name: wpa_driver_wext_set_auth_param
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

int wpa_driver_wext_set_auth_param(int sockfd, FAR const char *ifname,
                                   int idx, uint32_t value);

/****************************************************************************
 * Name: wpa_driver_wext_get_auth_param
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

int wpa_driver_wext_get_auth_param(int sockfd, FAR const char *ifname,
                                   int idx, uint32_t *value);

/****************************************************************************
 * Name: wpa_driver_wext_disconnect
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

void wpa_driver_wext_disconnect(int sockfd, FAR const char *ifname);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_WIRELESS_WAPI_H */
