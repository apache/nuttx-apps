/****************************************************************************
 * netutils/netlib/netlib_ipv6adaptor.c
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
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <net/if.h>
#include <nuttx/net/ip.h>

#include "netutils/netlib.h"

#ifdef CONFIG_NET_IPv6

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_ipv6adaptor
 ****************************************************************************/

int netlib_ipv6adaptor(FAR const struct in6_addr *destipaddr,
                       FAR struct in6_addr *srcipaddr)
{
  FAR struct lifreq *lifr;
  struct lifconf lifc;
  int nintf;
  int ret;
  int sd;
  int i;

  /* REVISIT:  Should check the routing table to see if there is a route
   * for the destipaddr before finding the device that provides the sub-net.
   */

  /* Find the size of the buffer we need to hold the interface descriptions */

  lifc.lifc_req = NULL;
  lifc.lifc_len = 0;

  sd = socket(AF_INET6, NETLIB_SOCK_IOCTL, 0);
  if (sd < 0)
    {
      ret = -errno;
      fprintf(stderr, "ERROR: socket() failed: %d\n", ret);
      return ret;
    }

  ret = ioctl(sd, SIOCGLIFCONF, (unsigned long)((uintptr_t)&lifc));
  if (ret < 0)
    {
      ret = -errno;
      fprintf(stderr, "ERROR: ioctl(SIOCGLIFCONF) failed: %d\n", ret);
      goto errout_with_sd;
    }

  /* Allocate the buffer to hold the interface descriptions */

  lifr = (FAR struct lifreq *)malloc(lifc.lifc_len);
  if (lifr == NULL)
    {
      fprintf(stderr, "ERROR: Failed to allocate LIFC buffer\n");
      ret = -ENOMEM;
      goto errout_with_sd;
    }

  /* Get the interface descriptions */

  lifc.lifc_req = lifr;

  ret = ioctl(sd, SIOCGLIFCONF, (unsigned long)((uintptr_t)&lifc));
  if (ret < 0)
    {
      ret = -errno;
      fprintf(stderr, "ERROR: ioctl(SIOCGLIFCONF) failed: %d\n", ret);
      goto errout_with_ifr;
    }

  /* Find an interface that supports the subnet needed by the provided
   * IPv6 address.
   */

  nintf = lifc.lifc_len / sizeof(struct lifreq);
  ret   = -EHOSTUNREACH;

  for (i = 0; i < nintf; i++)
    {
      FAR struct lifreq *req = &lifr[i];
      FAR struct sockaddr_in6 *addr =
        (FAR struct sockaddr_in6 *)&req->lifr_addr;
      struct lifreq maskreq;
      FAR struct sockaddr_in6 *netmask;
      int status;

      /* Get the network mask */

      strncpy(maskreq.lifr_name, lifr->lifr_name, IFNAMSIZ);

      status = ioctl(sd, SIOCGLIFNETMASK, (unsigned long)((uintptr_t)&maskreq));
      if (status < 0)
        {
          ret = -errno;
          fprintf(stderr, "ERROR: ioctl(SIOCGLIFNETMASK) failed: %d\n",
                  ret);
          goto errout_with_ifr;
        }

      netmask = (FAR struct sockaddr_in6 *)&maskreq.lifr_addr;

      /* Does this device provide the sub-net? */

      if (net_ipv6addr_maskcmp(addr->sin6_addr.s6_addr16,
                               destipaddr->s6_addr16,
                               netmask->sin6_addr.s6_addr16))
        {
          /* Yes.. return the IP address assigned to this adaptor */

          net_ipv6addr_copy(srcipaddr->s6_addr16,
                            addr->sin6_addr.s6_addr16);
          ret = OK;
          break;
        }
    }

errout_with_ifr:
  free(lifr);

errout_with_sd:
  close(sd);
  return ret;
}

#endif /* CONFIG_NET_IPv6 */
