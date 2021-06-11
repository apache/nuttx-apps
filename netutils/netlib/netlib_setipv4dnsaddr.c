/****************************************************************************
 * apps/netutils/netlib/netlib_setipv4dnsaddr.c
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

#if defined(CONFIG_NET_IPv4) && defined(CONFIG_NETDB_DNSCLIENT)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_set_ipv4dnsaddr
 *
 * Description:
 *   Set the DNS server IPv4 address
 *
 * Parameters:
 *   inaddr   The address to set
 *
 * Return:
 *   Zero (OK) is returned on success; A negated errno value is returned
 *   on failure.
 *
 ****************************************************************************/

int netlib_set_ipv4dnsaddr(FAR const struct in_addr *inaddr)
{
  struct sockaddr_in addr;
  int ret = -EINVAL;

  if (inaddr)
    {
      /* Set the IPv4 DNS server address */

      addr.sin_family = AF_INET;
      addr.sin_port   = 0;
      memcpy(&addr.sin_addr, inaddr, sizeof(struct in_addr));

      ret = dns_add_nameserver((FAR const struct sockaddr *)&addr,
                               sizeof(struct sockaddr_in));
    }

  return ret;
}

#endif /* CONFIG_NET_IPv4 && CONFIG_NETDB_DNSCLIENT */
