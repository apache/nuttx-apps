/****************************************************************************
 * netutils/netlib/netlib_getproperties.c
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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

#if (defined(CONFIG_NET_6LOWPAN) || defined(CONFIG_NET_IEEE802154)) && \
    CONFIG_NSOCKET_DESCRIPTORS > 0

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

#endif /* (CONFIG_NET_6LOWPAN || CONFIG_NET_IEEE802154) && CONFIG_NSOCKET_DESCRIPTORS */
