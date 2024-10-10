/****************************************************************************
 * apps/netutils/netlib/netlib_checkifconnectivity.c
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

#include "netutils/netlib.h"

#ifdef CONFIG_NETUTILS_PING

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_check_ifconnectivity
 *
 * Description:
 *   Check network interface connectivity by pinging the gateway
 *
 * Parameters:
 *   ifname   The name of the interface to use
 *   timeout  The timeout of ping
 *   retry    The retry times of ping
 *
 * Return:
 *   nums of gateway reply of ping; a nagtive on failure.
 *
 ****************************************************************************/

int netlib_check_ifconnectivity(FAR const char *ifname,
                                int timeout, int retry)
{
  int ret;
  char destip[INET_ADDRSTRLEN];
  struct in_addr addr;

  ret = netlib_get_dripv4addr(ifname, &addr);
  if (ret < 0)
    {
      nerr("ERROR: failed to get gateway ip %s\n", ifname);
      return ret;
    }

  if (net_ipv4addr_cmp(&addr, INADDR_ANY))
    {
      nerr("ERROR: failed to get gateway ip %s\n", ifname);
      return -EINVAL;
    }

  inet_ntop(AF_INET, &addr, destip, sizeof(destip));
  ninfo("ping gateway %s \n", destip);

  return netlib_check_ipconnectivity(destip, timeout, retry);
}

#endif /* CONFIG_NETUTILS_PING */