/****************************************************************************
 * apps/netutils/netlib/netlib_setessid.c
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
 * Name: netlib_setessid
 *
 * Description:
 *   Set the wireless access point ESSID
 *
 * Parameters:
 *   ifname   The name of the interface to use
 *   essid    Wireless ESSD address to set, size must be less then or equal
 *            to IW_ESSID_MAX_SIZE + 1 (including the NUL string terminator).
 *
 * Return:
 *   0 on success; -1 on failure (errno may not be set)
 *
 ****************************************************************************/

int netlib_setessid(FAR const char *ifname, FAR const char *essid)
{
  int ret = ERROR;

  if (ifname != NULL && essid != NULL)
    {
      /* Get a socket (only so that we get access to the INET subsystem) */

      int sockfd = socket(NETLIB_SOCK_FAMILY,
                          NETLIB_SOCK_TYPE, NETLIB_SOCK_PROTOCOL);
      if (sockfd >= 0)
        {
          struct iwreq req;

          /* Put the driver name into the request */

          strncpy(req.ifr_name, ifname, IFNAMSIZ);

          /* Put the new ESSID into the request */

          req.u.essid.pointer = (FAR void *)essid;
          req.u.essid.length  = strlen(essid) + 1;
          req.u.essid.flags   = 1;

          /* Perform the ioctl to set the ESSID */

          ret = ioctl(sockfd, SIOCSIWESSID, (unsigned long)&req);
          close(sockfd);
        }
    }

  return ret;
}

#endif /* CONFIG_NET */
