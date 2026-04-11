/****************************************************************************
 * apps/netutils/netlib/netlib_obtainipv4addr.c
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

#include <nuttx/debug.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "netutils/dhcpc.h"
#include "netutils/netlib.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: dhcp_setup_result
 *
 * Description:
 *   Set network connection parameters
 *
 * Parameters:
 *   ifname   The name of the interface to use
 *   ds       The configure network parameter values
 *
 * Return:
 *   0 on success
 *   Negative integer on failure
 *
 ****************************************************************************/

static int dhcp_setup_result(FAR const char *ifname,
                             FAR struct dhcpc_state *ds)
{
  int ret = netlib_set_ipv4addr(ifname, &ds->ipaddr);
  if (ret < 0)
    {
      nerr("ERROR: set IPv4 address failed: %d\n", ret);
      return ret;
    }

  if (ds->netmask.s_addr != 0)
    {
      ret = netlib_set_ipv4netmask(ifname, &ds->netmask);
      if (ret < 0)
        {
          nerr("ERROR: set IPv4 netmask failed: %d\n", ret);
          return ret;
        }
    }

  if (ds->default_router.s_addr != 0)
    {
      ret = netlib_set_dripv4addr(ifname, &ds->default_router);
      if (ret < 0)
        {
          nerr("ERROR: set default router address failed: %d\n", ret);
          return ret;
        }
    }

#ifdef CONFIG_NETDB_DNSCLIENT
  /* Set all received DNS server addresses */

  if (ds->num_dnsaddr > 0)
    {
      uint8_t i;
      for (i = 0; i < ds->num_dnsaddr; i++)
        {
          if (ds->dnsaddr[i].s_addr != 0)
            {
              ret = netlib_set_ipv4dnsaddr(&ds->dnsaddr[i]);
              if (ret < 0)
                {
                  nerr("ERROR: set DNS server %d address failed: %d\n",
                        i, ret);
                  return ret;
                }
            }
        }
    }
#endif

  return OK;
}

#ifdef CONFIG_NETUTILS_NTPCLIENT
static int dhcp_set_ntp_servers(FAR const struct dhcpc_state *ds)
{
  char ntp_server_list[CONFIG_NETUTILS_DHCPC_NTP_SERVERS *
                       (INET_ADDRSTRLEN + 1)];
  size_t offset = 0;
  uint8_t i;

  /* Clear the DHCP-provided NTP server list,
   * consider case: that device has joined another dhcp domain,
   * it need refresh related settings.
   */

  if (ds->num_ntpaddr == 0)
    {
      return netlib_set_ntp_servers_from_dhcp(NULL);
    }

  ntp_server_list[0] = '\0';

  for (i = 0; i < ds->num_ntpaddr; i++)
    {
      char addrbuf[INET_ADDRSTRLEN];
      int ret;

      /* Skip empty entries */

      if (ds->ntpaddr[i].s_addr == 0)
        {
          continue;
        }

      if (inet_ntop(AF_INET, &ds->ntpaddr[i], addrbuf, sizeof(addrbuf)) ==
          NULL)
        {
          return -EINVAL;
        }

      /* Append the server to the list */

      ret = snprintf(ntp_server_list + offset,
                     sizeof(ntp_server_list) - offset,
                     "%s%s", offset == 0 ? "" : ";", addrbuf);
      if (ret < 0 || (size_t)ret >= sizeof(ntp_server_list) - offset)
        {
          return -E2BIG;
        }

      offset += (size_t)ret;
    }

  /* Clear the list if all entries were empty */

  if (offset == 0)
    {
      return netlib_set_ntp_servers_from_dhcp(NULL);
    }

  return netlib_set_ntp_servers_from_dhcp(ntp_server_list);
}
#endif

/****************************************************************************
 * Name: dhcp_obtain_statefuladdr
 *
 * Description:
 *   Configure network  by dhcp
 *
 * Parameters:
 *   ifname   The name of the interface to use
 *
 * Return:
 *   0 on success
 *   Negative integer on failure
 *
 ****************************************************************************/

static int dhcp_obtain_statefuladdr(FAR const char *ifname)
{
  struct dhcpc_state ds;
  FAR void *handle;
  int ret;
  uint8_t mac[IFHWADDRLEN];

#ifdef CONFIG_NET_ETHERNET
  ret = netlib_getmacaddr(ifname, mac);
  if (ret < 0)
    {
      nerr("ERROR: get MAC address failed for '%s' : %d\n", ifname, ret);
      return ret;
    }
#else
  bzero(mac, sizeof(mac));
#endif

  /* Set up the DHCPC modules */

  handle = dhcpc_open(ifname, &mac, IFHWADDRLEN);
  if (handle == NULL)
    {
      nerr("ERROR: dhcpc open failed for '%s'\n", ifname);
      return -EINVAL;
    }

  /* Get an IP address.  Note that there is no logic for renewing the
   * IP address in this example. The address should be renewed in
   * (ds.lease_time / 2) seconds.
   */

  ret = dhcpc_request(handle, &ds);
  if (ret == OK)
    {
      ret = dhcp_setup_result(ifname, &ds);
#ifdef CONFIG_NETUTILS_NTPCLIENT
      if (ret == OK)
        {
          ret = dhcp_set_ntp_servers(&ds);
          if (ret < 0)
            {
              nwarn("WARNING: failed to update DHCP NTP server list: %d\n",
                    ret);
              ret = OK;
            }
        }
#endif
    }
  else
    {
      nerr("ERROR: dhcpc request failed: %d\n", ret);
    }

  dhcpc_close(handle);
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_obtain_ipv4addr
 *
 * Description:
 *   Perform DHCP in an attempt to get the IP
 *   address of the specified network device.
 *
 * Parameters:
 *   ifname   The name of the interface to use
 *
 * Return:
 *   0 on success
 *   Negative integer on failure
 *
 ****************************************************************************/

int netlib_obtain_ipv4addr(FAR const char *ifname)
{
  if (ifname == NULL)
    {
      nerr("ERROR: Invalid network card name\n");
      return -EINVAL;
    }

  return dhcp_obtain_statefuladdr(ifname);
}
