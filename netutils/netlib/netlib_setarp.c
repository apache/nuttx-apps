/****************************************************************************
 * netutils/netlib/netlib_setarp.c
 *
 *   Copyright (C) 2016 Gregory Nutt. All rights reserved.
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
#ifdef CONFIG_NET_ARP

#include <sys/socket.h>
#include <sys/ioctl.h>

#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <netinet/in.h>
#include <net/if.h>

#include <nuttx/net/arp.h>
#include "netutils/netlib.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_set_arpmapping
 *
 * Description:
 *   Add the current hardware address mapping for the provided protocol
 *   address to the ARP table.
 *
 * Parameters:
 *   inaddr   The IPv4 address to use in the mapping
 *   macaddr  The Ethernet MAC address to use in the mapping
 *
 * Return:
 *   0 on success; a negated errno value on failure.
 *
 ****************************************************************************/

int netlib_set_arpmapping(FAR const struct sockaddr_in *inaddr,
                          FAR const uint8_t *macaddr)
{
  int ret = -EINVAL;

  if (inaddr != NULL && macaddr != NULL)
    {
      int sockfd = socket(PF_INET, NETLIB_SOCK_TYPE, 0);
      if (sockfd >= 0)
        {
          struct arpreq req;

          memcpy(&req.arp_pa, inaddr, sizeof(struct sockaddr_in));

          req.arp_ha.sa_family = ARPHRD_ETHER;
          memcpy(&req.arp_ha.sa_data, macaddr, ETHER_ADDR_LEN);

          ret = ioctl(sockfd, SIOCSARP, (unsigned long)((uintptr_t)&req));
          if (ret < 0)
            {
              ret = -errno;
              DEBUGASSERT(ret < 0);
            }

          close(sockfd);
        }
    }

  return ret;
}

#endif /* CONFIG_NET_ARP */
