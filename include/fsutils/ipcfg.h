/****************************************************************************
 * apps/include/fsutils/ipcfg.h
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

#ifndef __APPS_INCLUDE_FSUTILS_IPCFG_H
#define __APPS_INCLUDE_FSUTILS_IPCFG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <netinet/in.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Prototype enumerations are bit encoded */

#define _IPCFG_STATIC   (1 << 0)  /* Bit 0: Have static addresses */
#define _IPCFG_DHCP     (1 << 1)  /* Bit 1: Use DHCP (IPv4) */
#define _IPCFG_AUTOCONF (1 << 1)  /* Bit 1: Use ICMPv4 auto-configuration */

#define IPCFG_HAVE_STATIC(p)  (((p) & _IPCFG_STATIC) != 0)
#define IPCFG_USE_DHCP(p)     (((p) & _IPCFG_DHCP) != 0)
#define IPCFG_USE_AUTOCONF(p) (((p) & _IPCFG_AUTOCONF) != 0)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* The structure contains the parsed content of the ipcfg-<dev> file.
 * Summary of file content:
 *
 * Common Settings:
 *
 * DEVICE=name
 *   where name is the name of the physical device.
 *
 * IPv4 Settings:
 *
 * IPv4PROTO=protocol
 *   where protocol is one of the following:
 *
 *     none     - No protocol selected
 *     static   - Use static IP
 *     dhcp     - The DHCP protocol should be used
 *     fallback - Use DHCP with fall back static IP
 *
 * All of the following addresses are in network order.  The special value
 * zero is used to indicate that the address is not available:
 *
 * IPv4IPADDR=address
 *    where address is the IPv4 address.  Used only with static or fallback
 *    protocols.
 *
 * IPv4NETMASK=address
 *    where address is the netmask.  Used only with static or fallback
 *    protocols.
 *
 * IPv4ROUTER=address
 *    where address is the IPv4 default router address.  Used only with
 *    static or fallback protocols.
 *
 * IPv4DNS=address
 *   where address is a (optional) name server address.
 */

/* Values for the IPv4PROTO setting */

enum ipv4cfg_bootproto_e
{
  IPv4PROTO_NONE     = 0, /* 00: No protocol assigned */
  IPv4PROTO_STATIC   = 1, /* 01: Use static IP */
  IPv4PROTO_DHCP     = 2, /* 10: Use DHCP */
  IPv4PROTO_FALLBACK = 3  /* 11: Use DHCP with fall back static IP */
};

struct ipv4cfg_s
{
  enum ipv4cfg_bootproto_e proto; /* Configure for static and/or DHCP */

  /* The following fields are required for static/fallback configurations */

  in_addr_t ipaddr;               /* IPv4 address */
  in_addr_t netmask;              /* Network mask */
  in_addr_t router;               /* Default router */

  /* The following fields are optional for dhcp and fallback configurations */

  in_addr_t dnsaddr;              /* Name server address */
};

/* IPv6 Settings:
 *
 * IPv6BOOTPROTO=protocol
 *   where protocol is one of the following:
 *
 *     none     - No protocol selected
 *     static   - Use static IP
 *     autoconf - ICMPv6 auto-configuration should be used
 *     fallback - Use auto-configuration with fall back static IP
 *
 * All of the following addresses are in network order.  The special value
 * zero is used to indicate that the address is not available:
 *
 * IPv6IPADDR=address
 *    where address is the IPv6 address.  Used only with static or fallback
 *    protocols.
 *
 * IPv6NETMASK=address
 *    where address is the netmask.  Used only with static or fallback
 *    protocols.
 *
 * IPv6ROUTER=address
 *    where address is the IPv6 default router address.  Used only with
 *    static or fallback protocols.
 */

/* Values for the IPv6BOOTPROTO setting */

enum ipv6cfg_bootproto_e
{
  IPv6PROTO_NONE     = 0, /* 00: No protocol assigned */
  IPv6PROTO_STATIC   = 1, /* 01: Use static IP */
  IPv6PROTO_AUTOCONF = 2, /* 10: Use ICMPv6 auto-configuration */
  IPv6PROTO_FALLBACK = 3  /* 11: Use auto-configuration with fall back static IP */
};

struct ipv6cfg_s
{
  enum ipv6cfg_bootproto_e proto; /* Configure for static and/or autoconfig */

  /* The following fields are required for static/fallback configurations */

  struct in6_addr ipaddr;         /* IPv6 address */
  struct in6_addr netmask;        /* Network mask */
  struct in6_addr router;         /* Default router */
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
 * Name: ipcfg_read
 *
 * Description:
 *   Read and parse the IP configuration file for the specified network
 *   device.
 *
 * Input Parameters:
 *   netdev - The network device.  For examplel "eth0"
 *   ipcfg  - Pointer to a user provided location to receive the IP
 *            configuration.  Refers to either struct ipv4cfg_s or
 *            ipv6cfg_s, depending on the value of af.
 *   af     - Identifies the address family whose IP configuration is
 *            requested.  May be either AF_INET or AF_INET6.
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

int ipcfg_read(FAR const char *netdev, FAR void *ipcfg, sa_family_t af);

/****************************************************************************
 * Name: ipcfg_write
 *
 * Description:
 *   Write the IP configuration file for the specified network device.
 *
 * Input Parameters:
 *   netdev - The network device.  For examplel "eth0"
 *   ipcfg  - The IP configuration to be written.  Refers to either struct
 *            ipv4cfg_s or ipv6cfg_s, depending on the value of af.
 *   af     - Identifies the address family whose IP configuration is
 *            to be written.  May be either AF_INET or AF_INET6.
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

#ifdef CONFIG_IPCFG_WRITABLE
int ipcfg_write(FAR const char *netdev, FAR const void *ipcfg,
                sa_family_t af);
#endif
#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_FSUTILS_IPCFG_H */
