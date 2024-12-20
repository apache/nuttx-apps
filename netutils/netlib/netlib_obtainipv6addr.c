/****************************************************************************
 * apps/netutils/netlib/netlib_obtainipv6addr.c
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
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <debug.h>

#include <netinet/in.h>
#include <net/if.h>
#include <nuttx/net/dns.h>

#include "netutils/netlib.h"
#include "netutils/dhcp6c.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int dhcpv6_setup_result(FAR const char *ifname,
                               FAR struct dhcp6c_state *presult)
{
  int ret;

#ifdef CONFIG_NETDEV_MULTIPLE_IPv6
  ret = netlib_add_ipv6addr(ifname, &presult->addr, presult->pl);
  if (ret != OK)
    {
      nerr("netlib_add_ipv6addr fail\n");
      return ret;
    }
#else

  ret = netlib_set_ipv6addr(ifname, &presult->addr);
  if (ret != OK)
    {
      nerr("netlib_set_ipv6addr fail\n");
      return ret;
    }

  ret = netlib_set_ipv6netmask(ifname, &presult->netmask);
  if (ret != OK)
    {
      nerr("netlib_set_ipv6netmask fail\n");
      return ret;
    }
#endif

#if defined(CONFIG_NET_IPv6) && defined(CONFIG_NETDB_DNSCLIENT)
  ret = netlib_set_ipv6dnsaddr(&presult->dns);
  if (ret != OK)
    {
      nerr("netlib_set_ipv6dnsaddr fail\n");
      return ret;
    }
#endif

  return ret;
}

static int dhcpv6_obtain_statefuladdr(FAR const char *ifname)
{
  FAR void *handle = NULL;
  struct dhcp6c_state result;
  int ret;

  memset(&result, 0, sizeof(result));
  handle = dhcp6c_open(ifname);

  ret = dhcp6c_request(handle, &result);
  if (ret == OK)
    {
      dhcpv6_setup_result(ifname, &result);
    }
  else
    {
      nerr("dhcp6c_request fail\n");
    }

  dhcp6c_close(handle);
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_obtain_ipv6addr
 *
 * Description:
 *   Perform ICMPv6 auto-configuration and DHCPv6 in an attempt to
 *   get the IP address of the specified network device.
 *
 * Parameters:
 *   ifname   The name of the interface
 *
 * Return:
 *   0 on success; negative integer on failure
 *
 ****************************************************************************/

int netlib_obtain_ipv6addr(FAR const char *ifname)
{
  int ret;

  if (ifname == NULL)
    {
      /* No interface name */

      errno = EINVAL;
      return ERROR;
    }

  ret = netlib_icmpv6_autoconfiguration(ifname);
  if (ret)
    {
      if (errno == EADDRNOTAVAIL)
        {
          ninfo("obtain IPv6 address by dhcpv6\n");
          ret = dhcpv6_obtain_statefuladdr(ifname);
        }
      else
        {
          nerr("netlib_icmpv6_autoconfiguration fail\n");
        }
    }

  return ret;
}
