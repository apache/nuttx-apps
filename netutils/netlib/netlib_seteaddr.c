/****************************************************************************
 * apps/netutils/netlib/netlib_seteaddr.c
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
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <net/if.h>
#include <nuttx/wireless/ieee802154/ieee802154_mac.h>

#include "netutils/netlib.h"

#if defined(CONFIG_NET_6LOWPAN) || defined(CONFIG_NET_IEEE802154)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_seteaddr
 *
 * Description:
 *   Set the IEEE802.15.4 extended MAC address
 *
 * Parameters:
 *   ifname The name of the interface to use
 *   eaddr  The new extended address
 *
 * Return:
 *   0 on success; -1 on failure.  errno will be set on failure.
 *
 ****************************************************************************/

int netlib_seteaddr(FAR const char *ifname, FAR const uint8_t *eaddr)
{
  struct ieee802154_netmac_s arg;
  int ret = ERROR;

  if (ifname != NULL)
    {
      /* Get a socket (only so that we get access to the INET subsystem) */

      int sockfd = socket(PF_INET6, NETLIB_SOCK_TYPE, 0);
      if (sockfd >= 0)
        {
          /* Perform the IOCTL */

          strncpy(arg.ifr_name, ifname, IFNAMSIZ);
          arg.u.setreq.attr = IEEE802154_ATTR_MAC_EADDR;
          IEEE802154_EADDRCOPY(arg.u.setreq.attrval.mac.eaddr, eaddr);

          ret = ioctl(sockfd, MAC802154IOC_MLME_SET_REQUEST,
                     (unsigned long)((uintptr_t)&arg));
          if (ret < 0)
            {
              ret = -errno;
              fprintf(stderr, "MAC802154IOC_MLME_SET_REQUEST failed: %d\n",
                     ret);
            }

          close(sockfd);
        }
    }

  return ret;
}

#endif /* CONFIG_NET_6LOWPAN || CONFIG_NET_IEEE802154 */
