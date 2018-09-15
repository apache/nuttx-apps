/****************************************************************************
 * examples/udp/udp_server.c
 *
 *   Copyright (C) 2007, 2009, 2012, 2015 Gregory Nutt. All rights reserved.
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

#include <sys/socket.h>
#include <netinet/in.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <arpa/inet.h>

#include "udp.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static inline int check_buffer(unsigned char *buf)
{
  int ret = 1;
  int offset;
  int ch;
  int j;

  offset = buf[0];
  for (ch = 0x20, j = offset + 1; ch < 0x7f; ch++, j++)
    {
      if (j >= SENDSIZE)
        {
          j = 1;
        }
      if (buf[j] != ch)
        {
          printf("server: Buffer content error for offset=%d, index=%d\n", offset, j);
          ret = 0;
        }
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void udp_server(void)
{
#ifdef CONFIG_EXAMPLES_UDP_IPv6
  struct sockaddr_in6 server;
  struct sockaddr_in6 client;
#else
  struct sockaddr_in server;
  struct sockaddr_in client;
  in_addr_t tmpaddr;
#endif
  unsigned char inbuf[1024];
  socklen_t addrlen;
  socklen_t recvlen;
  int sockfd;
  int nbytes;
  int optval;
  int offset;
#ifdef CONFIG_EXAMPLES_UDP_BROADCAST
  int ret;
#endif

  /* Create a new UDP socket */

  sockfd = socket(PF_INETX, SOCK_DGRAM, 0);
  if (sockfd < 0)
    {
      printf("server: socket failure: %d\n", errno);
      exit(1);
    }

  /* Set socket to reuse address */

  optval = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void*)&optval, sizeof(int)) < 0)
    {
      printf("server: setsockopt SO_REUSEADDR failure: %d\n", errno);
      exit(1);
    }

#ifdef CONFIG_EXAMPLES_UDP_BROADCAST
  optval = 1;
  ret = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(int));
  if (ret < 0)
    {
      printf("Failed to set SO_BROADCAST\n");
      exit(1);
    }
#endif

  /* Bind the socket to a local address */

#ifdef CONFIG_EXAMPLES_UDP_IPv6
  server.sin6_family     = AF_INET6;
  server.sin6_port       = HTONS(CONFIG_EXAMPLES_UDP_SERVER_PORTNO);
  memset(&server.sin6_addr, 0, sizeof(struct in6_addr));

  addrlen                = sizeof(struct sockaddr_in6);
#else
  server.sin_family      = AF_INET;
  server.sin_port        = HTONS(CONFIG_EXAMPLES_UDP_SERVER_PORTNO);
  server.sin_addr.s_addr = HTONL(INADDR_ANY);

  addrlen                = sizeof(struct sockaddr_in);
#endif

  if (bind(sockfd, (struct sockaddr*)&server, addrlen) < 0)
    {
      printf("server: bind failure: %d\n", errno);
      exit(1);
    }

  /* Then receive up to 256 packets of data */

  for (offset = 0; offset < 256; offset++)
    {
      printf("server: %d. Receiving up 1024 bytes\n", offset);
      recvlen = addrlen;
      nbytes = recvfrom(sockfd, inbuf, 1024, 0,
                        (struct sockaddr*)&client, &recvlen);

#ifdef CONFIG_EXAMPLES_UDP_IPv6
      printf("server: %d. Received %d bytes from %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x port %d\n",
             offset, nbytes,
             client.sin6_addr.s6_addr16[0], client.sin6_addr.s6_addr16[1],
             client.sin6_addr.s6_addr16[2], client.sin6_addr.s6_addr16[3],
             client.sin6_addr.s6_addr16[4], client.sin6_addr.s6_addr16[5],
             client.sin6_addr.s6_addr16[6], client.sin6_addr.s6_addr16[7],
             ntohs(client.sin6_port));
#else
      tmpaddr = ntohl(client.sin_addr.s_addr);
      printf("server: %d. Received %d bytes from %d.%d.%d.%d:%d\n",
             offset, nbytes,
             tmpaddr >> 24, (tmpaddr >> 16) & 0xff,
             (tmpaddr >> 8) & 0xff, tmpaddr & 0xff,
             ntohs(client.sin_port));
#endif
      if (nbytes < 0)
        {
          printf("server: %d. recv failed: %d\n", offset, errno);
          close(sockfd);
          exit(-1);
        }

      if (nbytes != SENDSIZE)
        {
          printf("server: %d. recv size incorrect: %d vs %d\n", offset, nbytes, SENDSIZE);
          close(sockfd);
          exit(-1);
        }

      if (offset < inbuf[0])
        {
          printf("server: %d. %d packets lost, resetting offset\n", offset, inbuf[0] - offset);
          offset = inbuf[0];
        }
      else if (offset > inbuf[0])
        {
          printf("server: %d. Bad offset in buffer: %d\n", offset, inbuf[0]);
          close(sockfd);
          exit(-1);
        }

      if (!check_buffer(inbuf))
        {
          printf("server: %d. Bad buffer contents\n", offset);
          close(sockfd);
          exit(-1);
        }
    }
  close(sockfd);
}
