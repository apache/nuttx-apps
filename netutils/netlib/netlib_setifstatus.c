/****************************************************************************
 * apps/netutils/netlib/netlib_setifstatus.c
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
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <netinet/in.h>
#include <net/if.h>

#include "netutils/netlib.h"

#ifdef CONFIG_NET

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_ifup
 *
 * Description:
 *   Set the network interface UP
 *
 * Parameters:
 *   ifname   The name of the interface to use
 *
 * Return:
 *   0 on success; -1 on failure
 *
 ****************************************************************************/

int netlib_ifup(const char *ifname)
{
  int ret = ERROR;
  if (ifname)
    {
      /* Get a socket (only so that we get access to the INET subsystem) */

      int sockfd = socket(NETLIB_SOCK_FAMILY,
                          NETLIB_SOCK_TYPE, NETLIB_SOCK_PROTOCOL);
      if (sockfd >= 0)
        {
          struct ifreq req;
          memset (&req, 0, sizeof(struct ifreq));

          /* Put the driver name into the request */

          strncpy(req.ifr_name, ifname, IFNAMSIZ);

          /* Perform the ioctl to ifup flag */

          req.ifr_flags |= IFF_UP;

          ret = ioctl(sockfd, SIOCSIFFLAGS, (unsigned long)&req);
          close(sockfd);
        }
    }

  return ret;
}

/****************************************************************************
 * Name: netlib_ifdown
 *
 * Description:
 *   Set the network interface DOWN
 *
 * Parameters:
 *   ifname   The name of the interface to use
 *
 * Return:
 *   0 on success; -1 on failure
 *
 ****************************************************************************/

int netlib_ifdown(const char *ifname)
{
  int ret = ERROR;
  if (ifname)
    {
      /* Get a socket (only so that we get access to the INET subsystem) */

      int sockfd = socket(NETLIB_SOCK_FAMILY,
                          NETLIB_SOCK_TYPE, NETLIB_SOCK_PROTOCOL);
      if (sockfd >= 0)
        {
          struct ifreq req;
          memset (&req, 0, sizeof(struct ifreq));

          /* Put the driver name into the request */

          strncpy(req.ifr_name, ifname, IFNAMSIZ);

          /* Perform the ioctl to ifup flag */

          req.ifr_flags |= IFF_DOWN;

          ret = ioctl(sockfd, SIOCSIFFLAGS, (unsigned long)&req);
          close(sockfd);
        }
    }

  return ret;
}

#endif /* CONFIG_NET */
