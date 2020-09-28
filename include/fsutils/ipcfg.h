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
 * Public Types
 ****************************************************************************/

/* Values for the BOOTPROTO setting */

enum ipcfg_bootproto_e
{
  BOOTPROTO_NONE     = 0, /* No protocol assigned */
  BOOTPROTO_STATIC   = 1, /* Use static IP */
  BOOTPROTO_DHCP     = 2, /* Use DHCP */
  BOOTPROTO_FALLBACK = 3  /* Use DHCP with fall back static IP */
};

/* The structure contains the parsed content of the ipcfg-<dev> file.
 * Summary of file content:
 *
 * DEVICE=name
 *   where name is the name of the physical device.
 *
 * BOOTPROTO=protocol
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
 * IPADDR=address
 *    where address is the IPv4 address.  Used only with static or fallback
 *    protocols.
 *
 * NETMASK=address
 *    where address is the netmask.  Used only with static or fallback
 *    protocols.
 *
 * ROUTER=address
 *    where address is the IPv4 default router address.  Used only with
 *    static or fallback protocols.
 *
 * DNS=address
 *   where address is a (optional) name server address.
 */

struct ipcfg_s
{
  enum ipcfg_bootproto_e proto; /* Configure for static and/or DHCP */

  /* The following fields are required for static/fallback configurations */

  in_addr_t ipaddr;             /* IPv4 address */
  in_addr_t netmask;            /* Network mask */
  in_addr_t router;             /* Default router */

  /* The following fields are optional for dhcp and fallback configurations */

  in_addr_t dnsaddr;            /* Name server address */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

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
 *            configuration.
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

int ipcfg_read(FAR const char *netdev, FAR struct ipcfg_s *ipcfg);

/****************************************************************************
 * Name: ipcfg_write
 *
 * Description:
 *   Write the IP configuration file for the specified network device.
 *
 * Input Parameters:
 *   netdev - The network device.  For examplel "eth0"
 *   ipcfg  - The IP configuration to be written.
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

#ifdef CONFIG_IPCFG_WRITABLE
int ipcfg_write(FAR const char *netdev, FAR const struct ipcfg_s *ipcfg);
#endif

#endif /* __APPS_INCLUDE_FSUTILS_IPCFG_H */
