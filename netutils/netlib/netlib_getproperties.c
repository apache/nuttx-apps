/****************************************************************************
 * apps/netutils/netlib/netlib_getproperties.c
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
#include <assert.h>
#include <errno.h>

#include "nuttx/wireless/pktradio.h"
#include "netutils/netlib.h"

#if defined(CONFIG_NET_6LOWPAN) || defined(CONFIG_NET_IEEE802154)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_getproperties
 *
 * Description:
 *   Get the non-IEEE802.15.4 packet radio node address
 *
 * Parameters:
 *   ifname  - The name of the interface to use
 *   nodeadd - Location to return the node address
 *
 * Return:
 *   0 on success; -1 on failure.  errno will be set on failure.
 *
 ****************************************************************************/

int netlib_getproperties(FAR const char *ifname,
                         FAR struct pktradio_properties_s *properties)
{
  struct pktradio_ifreq_s req;
  int ret = ERROR;

  DEBUGASSERT(ifname != NULL && properties != NULL);
  if (ifname != NULL)
    {
      /* Get a socket (only so that we get access to the INET subsystem) */

      int sockfd = socket(PF_INET6, NETLIB_SOCK_TYPE, 0);
      if (sockfd >= 0)
        {
          /* Copy the network interface name */

          strncpy(req.pifr_name, ifname, IFNAMSIZ);

          /* And perform the IOCTL command */

          ret = ioctl(sockfd, SIOCPKTRADIOGGPROPS,
                      (unsigned long)((uintptr_t)&req));
          if (ret >= 0)
            {
              /* Copy the radio properties from the request */

              memcpy(properties, &req.pifr_props,
                     sizeof(struct pktradio_properties_s));
            }

          close(sockfd);
        }
    }

  return ret;
}

#endif /* CONFIG_NET_6LOWPAN || CONFIG_NET_IEEE802154 */
