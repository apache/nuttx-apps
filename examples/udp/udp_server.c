/****************************************************************************
 * apps/examples/udp/udp_server.c
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
          printf("server: Buffer content error for offset=%d, index=%d\n",
                 offset, j);
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
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&optval,
                 sizeof(int)) < 0)
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

  if (bind(sockfd, (struct sockaddr *)&server, addrlen) < 0)
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
                        (struct sockaddr *)&client, &recvlen);

#ifdef CONFIG_EXAMPLES_UDP_IPv6
      printf("server: %d. Received %d bytes from "
             "%02x%02x:%02x%02x:%02x%02x:%02x%02x:"
             "%02x%02x:%02x%02x:%02x%02x:%02x%02x port %d\n",
             offset, nbytes,
             client.sin6_addr.s6_addr[0], client.sin6_addr.s6_addr[1],
             client.sin6_addr.s6_addr[2], client.sin6_addr.s6_addr[3],
             client.sin6_addr.s6_addr[4], client.sin6_addr.s6_addr[5],
             client.sin6_addr.s6_addr[6], client.sin6_addr.s6_addr[7],
             client.sin6_addr.s6_addr[8], client.sin6_addr.s6_addr[9],
             client.sin6_addr.s6_addr[10], client.sin6_addr.s6_addr[11],
             client.sin6_addr.s6_addr[12], client.sin6_addr.s6_addr[13],
             client.sin6_addr.s6_addr[14], client.sin6_addr.s6_addr[15],
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
          printf("server: %d. recv size incorrect: %d vs %d\n", offset,
                 nbytes, SENDSIZE);
          close(sockfd);
          exit(-1);
        }

      if (offset < inbuf[0])
        {
          printf("server: %d. %d packets lost, resetting offset\n", offset,
                 inbuf[0] - offset);
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
