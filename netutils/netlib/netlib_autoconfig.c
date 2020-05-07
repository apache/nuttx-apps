/****************************************************************************
 * netutils/netlib/netlib_autoconfig.c
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
#ifdef CONFIG_NET_ICMPv6_AUTOCONF

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <netinet/in.h>
#include <net/if.h>

#include "netutils/netlib.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_icmpv6_autoconfiguration
 *
 * Description:
 *   Perform ICMPv6 auto-configuration in an attempt to get the IP address
 *   of the specified network device.
 *
 * Parameters:
 *   ifname   The name of the interface to use
 *
 * Return:
 *   0 on success; -1 on failure (with the errno variable set appropriately)
 *
 ****************************************************************************/

int netlib_icmpv6_autoconfiguration(FAR const char *ifname)
{
  int ret = ERROR;

  /* Verify that an interface name was provided */

  if (ifname)
    {
      /* Get an IPv6 socket */

      int sockfd = socket(PF_INET6, NETLIB_SOCK_TYPE, 0);
      if (sockfd >= 0)
        {
          /* Create a request consisting only of the interface name */

          struct lifreq req;
          strncpy(req.lifr_name, ifname, IFNAMSIZ);

          /* Perform the ICMPv6 auto-configuration and close the socket */

          ret = ioctl(sockfd, SIOCIFAUTOCONF,
                      (unsigned long)((uintptr_t)&req));
          close(sockfd);
        }
    }
  else
    {
      /* No interface name */

      errno = EINVAL;
    }

  return ret;
}

#endif /* CONFIG_NET_ICMPv6_AUTOCONF */
