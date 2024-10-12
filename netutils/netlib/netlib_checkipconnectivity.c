/****************************************************************************
 * apps/netutils/netlib/netlib_checkipconnectivity.c
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
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <debug.h>

#include <arpa/inet.h>
#include <nuttx/net/ip.h>

#include "netutils/icmp_ping.h"
#include "netutils/netlib.h"

#ifdef CONFIG_NETUTILS_PING

/****************************************************************************
 * Private Data
 ****************************************************************************/

#define PING_IPADDR_DATALEN  80

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ping_ipaddr_callback
 *
 * Description:
 *   ping ipaddr callback
 *
 * Parameters:
 *   result  The result of ping
 *
 ****************************************************************************/

static void ping_ipaddr_callback(FAR const struct ping_result_s *result)
{
  FAR int *count = result->info->priv;

  if (result->code == ICMP_I_FINISH)
    {
      *count = result->nreplies;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_check_ipconnectivity
 *
 * Description:
 *   Get the network dns status
 *
 * Parameters:
 *   ip       The ipv4 address to check
 *   timeout  The max timeout of each ping
 *   retry    The retry times of ping
 *
 * Return:
 *   nums of remote reply of ping; a nagtive on failure
 *
 ****************************************************************************/

int netlib_check_ipconnectivity(FAR const char *ip, int timeout, int retry)
{
  int replies = 0;
#ifdef CONFIG_NETDB_DNSSERVER_IPv4ADDR
  char ip_str[INET_ADDRSTRLEN];
  struct in_addr addr;
#endif
  struct ping_info_s info;

  info.count    = retry;
  info.datalen  = PING_IPADDR_DATALEN;
  info.delay    = 0;
  info.timeout  = timeout;
  info.callback = ping_ipaddr_callback;
  info.priv     = &replies;

  /* if ip is NULL we use default DNS server */

  if (ip == NULL)
    {
#ifdef CONFIG_NETDB_DNSSERVER_IPv4ADDR
      addr.s_addr = HTONL(CONFIG_NETDB_DNSSERVER_IPv4ADDR);
      inet_ntop(AF_INET, &addr, ip_str, INET_ADDRSTRLEN);
      info.hostname = ip_str;
#else
      nerr("ERROR: IP is NULL and no DNS server configured\n");
      return -EINVAL;
#endif
    }
  else
    {
      info.hostname = ip;
    }

  ninfo("ping ipaddr test %s \n", info.hostname);
  icmp_ping(&info);

  return replies;
}

#endif /* CONFIG_NETUTILS_PING */