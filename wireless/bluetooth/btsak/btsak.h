/****************************************************************************
 * apps/wireless/bluetooth/btsak/btsak.h
 * Bluetooth Swiss Army Knife
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
 *   Author:  Gregory Nutt <gnutt@nuttx.org>
 *
 * Based loosely on the i8sak IEEE 802.15.4 program by Anthony Merlino and
 * Sebastien Lorquet.  Commands inspired for btshell example in the
 * Intel/Zephyr Arduino 101 package (BSD license).
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

#ifndef __APPS_EXAMPLES_WIRELESS_BLUETOOTH_BTSAK_BTSAK_H
#define __APPS_EXAMPLES_WIRELESS_BLUETOOTH_BTSAK_BTSAK_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netpacket/bluetooth.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BTSAK_MAX_IFNAME            12
#define BTSAK_DAEMONNAME_FMT        "btsak_%s"
#define BTSAK_DAEMONNAME_PREFIX_LEN 6
#define BTSAK_MAX_DAEMONNAME        BTSAK_DAEMONNAME_PREFIX_LEN + BTSAK_MAX_IFNAME
#define BTSAK_DEFAULT_EPADDR        {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct btsak_s
{
  FAR char *progname;                /* Program name */
  FAR char *ifname;                  /* Interface name */
  bt_addr_t ep_btaddr;               /* Blue tooth address */
#if defined(CONFIG_NET_BLUETOOTH)
  struct sockaddr_bt_s ep_sockaddr;  /* AF_BLUETOOTH endpoint address */
#elif defined(CONFIG_NET_6LOWPAN)
  struct sockaddr_in6 ep_sockaddr;   /* IPv6 endpoint address */
#endif
};

/****************************************************************************
 * Inline Functions
 ****************************************************************************/

#ifdef CONFIG_NET_6LOWPAN
static inline void btsak_update_ipv6addr(FAR struct btsak_s *btsak)
{  dev->d_ipv6addr[0]  = HTONS(0xfe80);
  dev->d_ipv6addr[1]  = 0;
  dev->d_ipv6addr[2]  = 0;
  dev->d_ipv6addr[3]  = 0;
  dev->d_ipv6addr[4]  = HTONS(0x0200);
  dev->d_ipv6addr[5]  = (uint16_t)addr[0] << 8 | (uint16_t)addr[1];
  dev->d_ipv6addr[6]  = (uint16_t)addr[2] << 8 | (uint16_t)addr[3];
  dev->d_ipv6addr[7]  = (uint16_t)addr[4] << 8 | (uint16_t)addr[5];

  btsak->ep_in6addr.sin6_addr.in6_u.u6_addr16[0] = HTONS(0xfe80);
  btsak->ep_in6addr.sin6_addr.in6_u.u6_addr16[1] = 0;
  btsak->ep_in6addr.sin6_addr.in6_u.u6_addr16[2] = 0;
  btsak->ep_in6addr.sin6_addr.in6_u.u6_addr16[3] = 0;
  btsak->ep_in6addr.sin6_addr.in6_u.u6_addr16[4] = HTONS(0x0200);
  btsak->ep_in6addr.sin6_addr.in6_u.u6_addr16[5] =
    ((uint16_t)btsak->ep_btaddr.val[0] << 8 | (uint16_t)btsak->ep_btaddr.val[1]);
  btsak->ep_in6addr.sin6_addr.in6_u.u6_addr16[6] =
    ((uint16_t)btsak->ep_btaddr.val[2] << 8 | (uint16_t)btsak->ep_btaddr.val[3]);
  btsak->ep_in6addr.sin6_addr.in6_u.u6_addr16[7] =
    ((uint16_t)btsak->ep_btaddr.val[4] << 8 | (uint16_t)btsak->ep_btaddr.val[5]);
}
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: btsak_char2nibble
 *
 * Description:
 *   Convert an hexadecimal character to a 4-bit nibble.
 *
 ****************************************************************************/

int btsak_char2nibble(char ch);

/****************************************************************************
 * Name: btsak_str2long
 *
 * Description:
 *   Convert a numeric string to an long value
 *
 ****************************************************************************/

long btsak_str2long(FAR const char *str);

/****************************************************************************
 * Name: btsak_str2uint8
 *
 * Description:
 *   Convert a numeric string to an uint8_t value
 *
 ****************************************************************************/

uint8_t btsak_str2uint8(FAR const char *str);

/****************************************************************************
 * Name: btsak_str2uint16
 *
 * Description:
 *   Convert a numeric string to an uint16_t value
 *
 ****************************************************************************/

uint16_t btsak_str2uint16(FAR const char *str);

/****************************************************************************
 * Name: btsak_str2bool
 *
 * Description:
 *   Convert a boolean name to a boolean value.
 *
 ****************************************************************************/

bool btsak_str2bool(FAR const char *str);

/****************************************************************************
 * Name : btsak_str2payload
 *
 * Description :
 *   Parse string to get buffer of data. Buf is expected to be of size
 *   BLUETOOTH_SMP_MTU or larger.
 *
 * Returns:
 *   Positive length value of frame payload
 ****************************************************************************/

int btsak_str2payload(FAR const char *str, FAR uint8_t *buf);

/****************************************************************************
 * Name: btsak_str2addr
 *
 * Description:
 *   Convert a string of the form "xx:xx:xx:xx:xx:xx" 6-byte Bluetooth
 *   address (where xx is a one or two character hexadecimal number sub-
 *   string)
 *
 ****************************************************************************/

int btsak_str2addr(FAR const char *str, FAR uint8_t *addr);

/****************************************************************************
 * Name: btsak_str2addrtype
 *
 * Description:
 *   Convert a string to an address type.  String options are "public" or
 *   "private".
 *
 ****************************************************************************/

int btsak_str2addrtype(FAR const char *str, FAR uint8_t *addrtype);

/****************************************************************************
 * Name: btsak_str2seclevel
 *
 * Description:
 *   Convert a string to a security level.  String options are "low",
 *   "medium", "high", or "fips"
 *
 ****************************************************************************/

int btsak_str2seclevel(FAR const char *str, FAR enum bt_security_e *level);

/****************************************************************************
 * Name: btsak_socket
 *
 * Description:
 *   Create a socket on the selected device.
 *
 ****************************************************************************/

int btsak_socket(FAR struct btsak_s *btsak);

/****************************************************************************
 * Name: btsak_showusage
 *
 * Description:
 *   Show program usage.
 *
 ****************************************************************************/

void btsak_showusage(FAR const char *progname, int exitcode);

/****************************************************************************
 * Name: btsak_gatt_showusage
 *
 * Description:
 *   Show gatt command usage.
 *
 ****************************************************************************/

void btsak_gatt_showusage(FAR const char *progname, FAR const char *cmd,
                          int exitcode);

/****************************************************************************
 * Command handlers
 ****************************************************************************/

void btsak_cmd_info(FAR struct btsak_s *btsak, int argc, FAR char *argv[]);
void btsak_cmd_features(FAR struct btsak_s *btsak, int argc,
       FAR char *argv[]);
void btsak_cmd_scan(FAR struct btsak_s *btsak, int argc, FAR char *argv[]);
void btsak_cmd_advertise(FAR struct btsak_s *btsak, int argc,
       FAR char *argv[]);
void btsak_cmd_security(FAR struct btsak_s *btsak, int argc,
       FAR char *argv[]);
void btsak_cmd_gatt_exchange_mtu(FAR struct btsak_s *btsak, int argc,
       FAR char *argv[]);
void btsak_cmd_gatt_exchange_mtu_result(FAR struct btsak_s *btsak, int argc,
       FAR char *argv[]);
void btsak_cmd_discover(FAR struct btsak_s *btsak, int argc,
       FAR char *argv[]);
void btsak_cmd_gatt_discover_characteristic(FAR struct btsak_s *btsak,
       int argc, FAR char *argv[]);
void btsak_cmd_gatt_discover_descriptor(FAR struct btsak_s *btsak,
       int argc, FAR char *argv[]);
void btsak_cmd_gatt_discover_get(FAR struct btsak_s *btsak,
       int argc, FAR char *argv[]);
void btsak_cmd_gatt_read(FAR struct btsak_s *btsak, int argc,
       FAR char *argv[]);
void btsak_cmd_gatt_read_multiple(FAR struct btsak_s *btsak, int argc,
       FAR char *argv[]);
void btsak_cmd_gatt_read_get(FAR struct btsak_s *btsak, int argc,
       FAR char *argv[]);
void btsak_cmd_gatt_write(FAR struct btsak_s *btsak, int argc,
       FAR char *argv[]);
void btsak_cmd_gatt_write_get(FAR struct btsak_s *btsak, int argc,
       FAR char *argv[]);

#endif /* __APPS_EXAMPLES_WIRELESS_BLUETOOTH_BTSAK_BTSAK_H */
