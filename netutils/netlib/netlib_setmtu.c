/****************************************************************************
 * apps/netutils/netlib/netlib_setmtu.c
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
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <netinet/in.h>
#include <net/if.h>

#include "netutils/netlib.h"

#ifdef CONFIG_NET

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_set_mtu
 *
 * Description:
 *   Set MTU of the NuttX driver state structure
 *
 * Parameters:
 *   ifname   The name of the interface to use
 *   mtu    Maximum Transmission Unit，MTU
 *
 * Return:
 *   0 on success; -1 on failure (errno may not be set)
 *
 ****************************************************************************/

int netlib_set_mtu(FAR const char *ifname, int mtu)
{
  int ret = ERROR;

  if (ifname != NULL && mtu != 0)
    {
      /* Get a socket (only so that we get access to the INET subsystem) */

      int sockfd = socket(NET_SOCK_FAMILY, NET_SOCK_TYPE, NET_SOCK_PROTOCOL);
      if (sockfd >= 0)
        {
          struct ifreq req;

          /* Put the driver name into the request */

          strlcpy(req.ifr_name, ifname, IFNAMSIZ);

          /* Put the new MTU into the request */

          req.ifr_mtu = mtu;

          /* Perform the ioctl to set the MTU */

          ret = ioctl(sockfd, SIOCSIFMTU, (unsigned long)&req);
          close(sockfd);
        }
    }

  return ret;
}

#endif /* CONFIG_NET */
