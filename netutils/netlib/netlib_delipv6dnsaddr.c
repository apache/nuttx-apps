/****************************************************************************
 * apps/netutils/netlib/netlib_delipv6dnsaddr.c
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
#include <string.h>
#include <errno.h>

#include <netinet/in.h>
#include <nuttx/net/dns.h>

#include "netutils/netlib.h"

#if defined(CONFIG_NET_IPv6) && defined(CONFIG_NETDB_DNSCLIENT)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_del_ipv6dnsaddr
 *
 * Description:
 *   Remove a DNS server IPv6 address from the list by address
 *
 * Parameters:
 *   inaddr   The IPv6 address to remove
 *
 * Return:
 *   Zero (OK) is returned on success; A negated errno value is returned
 *   on failure.
 *
 ****************************************************************************/

int netlib_del_ipv6dnsaddr(FAR const struct in6_addr *inaddr)
{
  struct sockaddr_in6 addr;
  int ret = -EINVAL;

  if (inaddr)
    {
      /* Del the IPv6 DNS server address */

      addr.sin6_family = AF_INET6;
      addr.sin6_port   = 0;
      memcpy(&addr.sin6_addr, inaddr, sizeof(struct in6_addr));

      ret = dns_del_nameserver((FAR const struct sockaddr *)&addr,
                               sizeof(struct sockaddr_in6));
    }

  return ret;
}

/****************************************************************************
 * Name: netlib_del_ipv6dnsaddr_by_index
 *
 * Description:
 *   Remove a DNS server IPv6 address from the list by index (0-based)
 *
 * Parameters:
 *   index:The index of the DNS server to remove (0=first, 1=second, etc.)
 *
 * Return:
 *   Zero (OK) is returned on success; A negated errno value is returned
 *   on failure.
 *
 ****************************************************************************/

int netlib_del_ipv6dnsaddr_by_index(int index)
{
  return dns_del_nameserver_by_index(index);
}

#endif /* CONFIG_NET_IPv6 && CONFIG_NETDB_DNSCLIENT */
