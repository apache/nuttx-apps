/****************************************************************************
 * netutils/netlib/netlib_setdripv4addr.c
 *
 *   Copyright (C) 2007-2009, 2011, 2015 Gregory Nutt. All rights reserved.
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
#ifdef CONFIG_NET_IPv4

#include <sys/socket.h>
#include <sys/ioctl.h>

#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <netinet/in.h>
#include <net/if.h>

#ifdef CONFIG_NET_ROUTE
#  include <net/route.h>
#endif

#include "netutils/netlib.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_set_dripv4addr
 *
 * Description:
 *   Set the default router IPv4 address
 *
 * Parameters:
 *   ifname   The name of the interface to use
 *   ipaddr   The address to set
 *
 * Return:
 *   0 on success; -1 on failure
 *
 ****************************************************************************/

int netlib_set_dripv4addr(FAR const char *ifname,
                          FAR const struct in_addr *addr)
{
  int ret = ERROR;

#ifdef CONFIG_NET_ROUTE
  struct sockaddr_in target;
  struct sockaddr_in netmask;
  struct sockaddr_in router;

  memset(&target, 0, sizeof(target));
  target.sin_family  = AF_INET;

  memset(&netmask, 0, sizeof(netmask));
  netmask.sin_family  = AF_INET;

  router.sin_addr    = *addr;
  router.sin_family  = AF_INET;
#endif

  if (ifname && addr)
    {
      int sockfd = socket(PF_INET, NETLIB_SOCK_TYPE, 0);
      if (sockfd >= 0)
        {
          FAR struct sockaddr_in *inaddr;
          struct ifreq req;

          /* Add the device name to the request */

          strncpy(req.ifr_name, ifname, IFNAMSIZ);

          /* Add the INET address to the request */

          inaddr             = (FAR struct sockaddr_in *)&req.ifr_addr;
          inaddr->sin_family = AF_INET;
          inaddr->sin_port   = 0;
          memcpy(&inaddr->sin_addr, addr, sizeof(struct in_addr));

          ret = ioctl(sockfd, SIOCSIFDSTADDR, (unsigned long)&req);

#ifdef CONFIG_NET_ROUTE
          if (OK == ret)
            {
              /* Delete the default route first */
              /* This call fails if no default route exists, but it's OK */

              delroute(sockfd,
                       (FAR struct sockaddr_storage *)&target,
                       (FAR struct sockaddr_storage *)&netmask);

              /* Then add the new default route */

              ret = addroute(sockfd,
                             (FAR struct sockaddr_storage *)&target,
                             (FAR struct sockaddr_storage *)&netmask,
                             (FAR struct sockaddr_storage *)&router);
            }
#endif

          close(sockfd);
        }
    }

  return ret;
}

#endif /* CONFIG_NET_IPv4 */
