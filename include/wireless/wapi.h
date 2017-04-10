/****************************************************************************
 * apps/include/wireless/wapi.h
 *
 *   Copyright (C) 2011, 2017Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Adapted for Nuttx from WAPI:
 *
 *   Copyright (c) 2010, Volkan YAZICI <volkan.yazici@gmail.com>
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of  source code must  retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of  conditions and the  following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND  FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Generic linked list (dummy) decleration. (No definition!) */

typedef struct wapi_list_t wapi_list_t;

/* Frequency flags. */

typedef enum
{
  WAPI_FREQ_AUTO  = IW_FREQ_AUTO,
  WAPI_FREQ_FIXED = IW_FREQ_FIXED
} wapi_freq_flag_t;

/* Route target types. */

typedef enum
{
  WAPI_ROUTE_TARGET_NET,              /* The target is a network. */
  WAPI_ROUTE_TARGET_HOST              /* The target is a host. */
} wapi_route_target_t;

/* ESSID flags.  */

typedef enum
{
  WAPI_ESSID_ON,
  WAPI_ESSID_OFF
} wapi_essid_flag_t;

/* Supported operation modes. */

typedef enum
{
  WAPI_MODE_AUTO    = IW_MODE_AUTO,   /* Driver decides. */
  WAPI_MODE_ADHOC   = IW_MODE_ADHOC,  /* Single cell network. */
  WAPI_MODE_MANAGED = IW_MODE_INFRA,  /* Multi cell network, roaming, ... */
  WAPI_MODE_MASTER  = IW_MODE_MASTER, /* Synchronisation master or access point. */
  WAPI_MODE_REPEAT  = IW_MODE_REPEAT, /* Wireless repeater, forwarder. */
  WAPI_MODE_SECOND  = IW_MODE_SECOND, /* Secondary master/repeater, backup. */
  WAPI_MODE_MONITOR = IW_MODE_MONITOR /* Passive monitor, listen only. */
} wapi_mode_t;

/* Bitrate flags.
 *
 * At the moment, unicast (IW_BITRATE_UNICAST) and broadcast
 * (IW_BITRATE_BROADCAST) bitrate flags are not supported.
 */

typedef enum
{
  WAPI_BITRATE_AUTO,
  WAPI_BITRATE_FIXED
} wapi_bitrate_flag_t;

/* Transmit power (txpower) flags. */

typedef enum
{
  WAPI_TXPOWER_DBM,                   /* Value is in dBm. */
  WAPI_TXPOWER_MWATT,                 /* Value is in mW. */
  WAPI_TXPOWER_RELATIVE               /* Value is in arbitrary units. */
} wapi_txpower_flag_t;

/* Linked list container for strings. */

typedef struct wapi_string_t
{
  FAR struct wapi_string_t *next;
  FAR char *data;
} wapi_string_t;

/* Linked list container for scan results. */

typedef struct wapi_scan_info_t
{
  FAR struct wapi_scan_info_t *next;
  struct ether_addr ap;
  int has_essid;
  char essid[WAPI_ESSID_MAX_SIZE + 1];
  wapi_essid_flag_t essid_flag;
  int has_freq;
  double freq;
  int has_mode;
  wapi_mode_t mode;
  int has_bitrate;
  int bitrate;
} wapi_scan_info_t;

/* Linked list container for routing table rows. */

typedef struct wapi_route_info_t
{
  FAR struct wapi_route_info_t *next;
  FAR char *ifname;
  struct in_addr dest;
  struct in_addr gw;

  unsigned int flags;/* See  RTF_* in  net/route.h for available values. */
  unsigned int refcnt;
  unsigned int use;
  unsigned int metric;
  struct in_addr netmask;
  unsigned int mtu;
  unsigned int window;
  unsigned int irtt;
} wapi_route_info_t;

/* A generic linked list container. For functions taking  wapi_list_t type of
 * argument, caller is resposible for releasing allocated memory.
 */

struct wapi_list_t
{
  union wapi_list_head_t
  {
    FAR wapi_string_t *string;
    FAR wapi_scan_info_t *scan;
    FAR wapi_route_info_t *route;
  } head;
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

int wapi_get_ifup(int sock, const char *ifname, int *is_up);

/****************************************************************************
 * Name: wapi_set_ifup
 *
 * Description:
 *   Activates the interface.
 *
 ****************************************************************************/

int wapi_set_ifup(int sock, const char *ifname);

/****************************************************************************
 * Name: wapi_set_ifdown
 *
 * Description:
 *   Shuts down the interface.
 *
 ****************************************************************************/

int wapi_set_ifdown(int sock, const char *ifname);

/****************************************************************************
 * Name: wapi_get_ip
 *
 * Description:
 *   Gets IP address of the given network interface.
 *
 ****************************************************************************/

int wapi_get_ip(int sock, const char *ifname, struct in_addr *addr);

/****************************************************************************
 * Name: wapi_set_ip
 *
 * Description:
 *   Sets IP adress of the given network interface.
 *
 ****************************************************************************/

int wapi_set_ip(int sock, const char *ifname, const struct in_addr *addr);

/****************************************************************************
 * Name: wapi_get_netmask
 *
 * Description:
 *   Gets netmask of the given network interface.
 *
 ****************************************************************************/

int wapi_get_netmask(int sock, const char *ifname, struct in_addr *addr);

/****************************************************************************
 * Name: wapi_set_netmask
 *
 * Description:
 *   Sets netmask of the given network interface.
 *
 ****************************************************************************/

int wapi_set_netmask(int sock, const char *ifname, const struct in_addr *addr);

/****************************************************************************
 * Name: wapi_add_route_gw
 *
 * Description:
 *   Adds gateway for the given target network.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_ROUTE
int wapi_add_route_gw(int sock, wapi_route_target_t targettype,
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
int wapi_del_route_gw(int sock, wapi_route_target_t targettype,
                      FAR const struct in_addr *target,
                      FAR const struct in_addr *netmask,
                      FAR const struct in_addr *gw);
#endif

/****************************************************************************
 * Name: wapi_get_we_version
 *
 * Description:
 *   Gets kernel WE (Wireless Extensions) version.
 *
 * Input Parameters:
 *   we_version Set to  we_version_compiled of range information.
 *
 * Returned Value:
 *   Zero on success.
 *
 ****************************************************************************/

int wapi_get_we_version(int sock, const char *ifname, FAR int *we_version);

/****************************************************************************
 * Name: wapi_get_freq
 *
 * Description:
 *   Gets the operating frequency of the device.
 *
 ****************************************************************************/

int wapi_get_freq(int sock, FAR const char *ifname, FAR double *freq,
                  FAR wapi_freq_flag_t *flag);

/****************************************************************************
 * Name: wapi_set_freq
 *
 * Description:
 *   Sets the operating frequency of the device.
 *
 ****************************************************************************/

int wapi_set_freq(int sock, FAR const char *ifname, double freq,
                  wapi_freq_flag_t flag);

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
                   FAR wapi_essid_flag_t *flag);

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
                   wapi_essid_flag_t flag);

/****************************************************************************
 * Name: wapi_get_mode
 *
 * Description:
 *   Gets the operating mode of the device.
 *
 ****************************************************************************/

int wapi_get_mode(int sock, FAR const char *ifname, FAR wapi_mode_t *mode);

/****************************************************************************
 * Name: wapi_set_mode
 *
 * Description:
 *   Sets the operating mode of the device.
 *
 ****************************************************************************/

int wapi_set_mode(int sock, FAR const char *ifname, wapi_mode_t mode);

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
                     FAR int *bitrate, FAR wapi_bitrate_flag_t *flag);

/****************************************************************************
 * Name: wapi_set_bitrate
 *
 * Description:
 *   Sets bitrate of the device.
 *
 ****************************************************************************/

int wapi_set_bitrate(int sock, FAR const char *ifname, int bitrate,
                     wapi_bitrate_flag_t flag);

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
                     FAR wapi_txpower_flag_t *flag);

/****************************************************************************
 * Name: wapi_set_txpower
 *
 * Description:
 *   Sets txpower of the device.
 *
 ****************************************************************************/

int wapi_set_txpower(int sock, FAR const char *ifname, int power,
                     wapi_txpower_flag_t flag);

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
 *   Starts a scan on the given interface. Root privileges are required to start a
 *   scan.
 *
 ****************************************************************************/

int wapi_scan_init(int sock, const char *ifname);

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
 *   aps - Pushes collected  wapi_scan_info_t into this list.
 *
 ****************************************************************************/

int wapi_scan_coll(int sock, FAR const char *ifname, FAR wapi_list_t *aps);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_WIRELESS_WAPI_H */
