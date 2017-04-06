/****************************************************************************
 * examples/udpblaster/udpblaster_host.c
 *
 *   Copyright (C) 2015 Gregory Nutt. All rights reserved.
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
 * 3. Neither the name Gregory Nutt nor the names of its contributors may be
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

#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include "udpblaster.h"

/****************************************************************************
 * main
 ****************************************************************************/

int main(int argc, char **argv, char **envp)
{
#ifdef CONFIG_EXAMPLES_UDPBLASTER_IPv4
  struct sockaddr_in target;
#else
  struct sockaddr_in6 target;
#endif
  socklen_t addrlen;
  size_t sendsize;
  unsigned long delay;
  int npackets;
  int ndots;
  int sockfd;
  int ret;

#ifdef CONFIG_EXAMPLES_UDPBLASTER_IPv4
  target.sin_family             = AF_INET;
  target.sin_port               = HTONS(UDPBLASTER_TARGET_PORTNO);
  target.sin_addr.s_addr        = HTONL(CONFIG_EXAMPLES_UDPBLASTER_TARGETIP);

  addrlen                       = sizeof(struct sockaddr_in);
  sockfd                        = socket(PF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    {
      fprintf(stderr, "ERROR: socket() failed: %d\n", errno);
      return 1;
    }

#else
  target.sin6_family            = AF_INET6;
  target.sin6_port              = HTONS(UDPBLASTER_TARGET_PORTNO);
  target.sin6_addr.s6_addr16[0] = HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_1);
  target.sin6_addr.s6_addr16[1] = HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_2);
  target.sin6_addr.s6_addr16[2] = HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_3);
  target.sin6_addr.s6_addr16[3] = HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_4);
  target.sin6_addr.s6_addr16[4] = HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_5);
  target.sin6_addr.s6_addr16[5] = HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_6);
  target.sin6_addr.s6_addr16[6] = HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_7);
  target.sin6_addr.s6_addr16[7] = HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_8);

  addrlen                       = sizeof(struct sockaddr_in6);
  sockfd                        = socket(PF_INET6, SOCK_DGRAM, 0);
  if (sockfd < 0)
    {
      fprintf(stderr, "ERROR: socket() failed: %d\n", errno);
      return 1;
    }
#endif

  /* bytes/packet = UDPBLASTER_SENDSIZE
   * bits/sec     = CONFIG_EXAMPLES_UDPBLASTER_HOSTRATE
   * bytes/sec    = CONFIG_EXAMPLES_UDPBLASTER_HOSTRATE / 8
   * packets/sec  = (bytes/sec) / (bytes/packet)
   *              = CONFIG_EXAMPLES_UDPBLASTER_HOSTRATE / UDPBLASTER_SENDSIZE / 8
   * delay        = microseconds/packet
   *              = (1000000 * UDPBLASTER_SENDSIZE) / CONFIG_EXAMPLES_UDPBLASTER_HOSTRATE / 8
   *              = (125000 * UDPBLASTER_SENDSIZE) / CONFIG_EXAMPLES_UDPBLASTER_HOSTRATE
   */

  sendsize = UDPBLASTER_SENDSIZE;
  delay    = (125000 * sendsize) / CONFIG_EXAMPLES_UDPBLASTER_HOSTRATE;

  npackets = 0;
  ndots    = 0;

  for (;;)
    {
      ret = sendto(sockfd, g_udpblaster_text, sendsize, 0,
                   (struct sockaddr *)&target, addrlen);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: sendto() failed: %d\n", errno);
          return 1;
        }

      if (++npackets >= 10)
        {
          putchar('.');
          npackets = 0;

          if (++ndots >= 50)
          {
            putchar('\n');
            ndots = 0;
          }
        }

      usleep(delay);
    }

  return 0; /* Won't get here */
}
