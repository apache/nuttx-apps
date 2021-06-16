/****************************************************************************
 * apps/examples/udpblaster/udpblaster_host.c
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

#include "config.h"

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include "udpblaster.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

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

  *(uint16_t *)&target.sin6_addr.s6_addr[0] =
      HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_1);
  *(uint16_t *)&target.sin6_addr.s6_addr[2] =
      HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_2);
  *(uint16_t *)&target.sin6_addr.s6_addr[4] =
      HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_3);
  *(uint16_t *)&target.sin6_addr.s6_addr[6] =
      HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_4);
  *(uint16_t *)&target.sin6_addr.s6_addr[8] =
      HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_5);
  *(uint16_t *)&target.sin6_addr.s6_addr[10] =
      HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_6);
  *(uint16_t *)&target.sin6_addr.s6_addr[12] =
      HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_7);
  *(uint16_t *)&target.sin6_addr.s6_addr[14] =
      HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_8);

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
   *              = CONFIG_EXAMPLES_UDPBLASTER_HOSTRATE /
   *                UDPBLASTER_SENDSIZE / 8
   * delay        = microseconds/packet
   *              = (1000000 * UDPBLASTER_SENDSIZE) /
   *                CONFIG_EXAMPLES_UDPBLASTER_HOSTRATE / 8
   *              = (125000 * UDPBLASTER_SENDSIZE) /
   *                CONFIG_EXAMPLES_UDPBLASTER_HOSTRATE
   */

  sendsize = UDPBLASTER_SENDSIZE;
  delay    = (125000 * sendsize) / CONFIG_EXAMPLES_UDPBLASTER_HOSTRATE;

  npackets = 0;
  ndots    = 0;

  for (; ; )
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
