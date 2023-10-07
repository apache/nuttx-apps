/****************************************************************************
 * apps/system/dhcp6c/renew6_main.c
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
#include <debug.h>

#include <net/if.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <nuttx/net/net.h>
#include <nuttx/net/netdev.h>
#include <nuttx/net/dns.h>
#include "netutils/netlib.h"
#include "netutils/dhcp6c.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void icmpv6_linkipaddr_6(FAR const uint8_t *mac, uint16_t *ipaddr)
{
  ipaddr[0]  = HTONS(0xfe80);
  ipaddr[1]  = 0;
  ipaddr[2]  = 0;
  ipaddr[3]  = 0;
  ipaddr[4]  = HTONS(mac[0] << 8 | mac[1]);
  ipaddr[5]  = HTONS(mac[2] << 8 | 0x00ff);
  ipaddr[6]  = HTONS(0x00fe << 8 | mac[3]);
  ipaddr[7]  = HTONS(mac[4] << 8 | mac[5]);
  ipaddr[4] ^= HTONS(0x0200);
}

static void dhcp6c_set_lladdr(FAR const char *ifname)
{
  struct in6_addr addr6;
  uint8_t mac[IFHWADDRLEN];

  /* Get the MAC address of the NIC */

  netlib_getmacaddr(ifname, mac);

  /* Set the Link Local Address of the NIC */

  icmpv6_linkipaddr_6(mac, (uint16_t *)&addr6);
  netlib_set_ipv6addr(ifname, &addr6);
  netlib_prefix2ipv6netmask(64, &addr6);
}

static int setup_result(FAR const char *ifname,
                        FAR struct dhcp6c_state *presult)
{
  int ret;

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

  ret = netlib_set_ipv6dnsaddr(&presult->dns);
  if (ret != OK)
    {
      nerr("netlib_set_ipv6dnsaddr fail\n");
      return ret;
    }

  return ret;
}

int main(int argc, FAR char * const argv[])
{
  char ifname[IFNAMSIZ];
  void *handle = NULL;
  struct dhcp6c_state result;
  int ret = -EINVAL;

  memset(ifname, 0, sizeof(ifname));
  memset(&result, 0, sizeof(result));
  if (argc < 2)
    {
      nerr("Input parameters are invalid!\n");
      return ret;
    }

  strlcpy(ifname, argv[1], sizeof(ifname));

  if (argc == 2)
    {
      ret = netlib_icmpv6_autoconfiguration(ifname);
      if (ret)
        {
          nerr("netlib_icmpv6_autoconfiguration fail\n");
          return ret;
        }
    }
  else
    {
      dhcp6c_set_lladdr(ifname);
      handle = dhcp6c_open(ifname);
      ret = dhcp6c_request(handle, &result);
      if (ret != OK)
        {
          nerr("dhcp6c_request fail\n");
        }

      setup_result(ifname, &result);
      dhcp6c_close(handle);
    }

  return ret;
}
