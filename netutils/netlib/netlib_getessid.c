/****************************************************************************
 * apps/netutils/netlib/netlib_getessid.c
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

#include <nuttx/wireless/wireless.h>

#include "netutils/netlib.h"

#ifdef CONFIG_NET

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_getessid
 *
 * Description:
 *   Get the wireless access point ESSID
 *
 * Parameters:
 *   ifname   The name of the interface to use
 *   essid    Location to store the returned ESSID, size must be
 *            IW_ESSID_MAX_SIZE + 1 or larger
 *   idlen    size of memory set asdie for the ESSID.
 *
 * Return:
 *   0 on success; -1 on failure (errno may not be set)
 *
 ****************************************************************************/

int netlib_getessid(FAR const char *ifname, FAR char *essid, size_t idlen)
{
  int ret = ERROR;

  if (ifname != NULL && essid != NULL && idlen > IW_ESSID_MAX_SIZE)
    {
      /* Get a socket (only so that we get access to the INET subsystem) */

      int sockfd = socket(NETLIB_SOCK_FAMILY,
                          NETLIB_SOCK_TYPE, NETLIB_SOCK_PROTOCOL);
      if (sockfd >= 0)
        {
          struct iwreq req;

          /* Put the driver name into the request */

          memset(&req, 0, sizeof(struct iwreq));
          strncpy(req.ifr_name, ifname, IFNAMSIZ);

          /* Put pointer to receive the ESSID into the request */

          req.u.essid.pointer = (FAR void *)essid;

          /* Perform the ioctl to get the ESSID */

          ret = ioctl(sockfd, SIOCGIWESSID, (unsigned long)&req);
          close(sockfd);
        }
    }

  return ret;
}

#endif /* CONFIG_NET */
