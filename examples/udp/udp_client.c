/****************************************************************************
 * apps/examples/udp/udp_client.c
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

#include <sys/types.h>
#include <sys/socket.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "udp.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int create_socket(void)
{
  socklen_t addrlen;
  int sockfd;

#ifdef CONFIG_EXAMPLES_UDP_IPv4
  struct sockaddr_in addr;

  /* Create a new IPv4 UDP socket */

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    {
      printf("client ERROR: client socket failure %d\n", errno);
      return -1;
    }

  /* Bind the UDP socket to a IPv4 port */

  addr.sin_family      = AF_INET;
  addr.sin_port        = HTONS(CONFIG_EXAMPLES_UDP_CLIENT_PORTNO);
  addr.sin_addr.s_addr = HTONL(INADDR_ANY);
  addrlen              = sizeof(struct sockaddr_in);

#else
  struct sockaddr_in6 addr;

  /* Create a new IPv6 UDP socket */

  sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
  if (sockfd < 0)
    {
      printf("client ERROR: client socket failure %d\n", errno);
      return -1;
    }

  /* Bind the UDP socket to a IPv6 port */

  addr.sin6_family     = AF_INET6;
  addr.sin6_port       = HTONS(CONFIG_EXAMPLES_UDP_CLIENT_PORTNO);
  memset(addr.sin6_addr.s6_addr, 0, sizeof(struct in6_addr));
  addrlen              = sizeof(struct sockaddr_in6);
#endif

  if (bind(sockfd, (FAR struct sockaddr *)&addr, addrlen) < 0)
    {
      printf("client ERROR: Bind failure: %d\n", errno);
      return -1;
    }

  return sockfd;
}

static inline void fill_buffer(unsigned char *buf, int offset)
{
  int ch;
  int j;

  buf[0] = offset;
  for (ch = 0x20, j = offset + 1; ch < 0x7f; ch++, j++)
    {
      if (j >= SENDSIZE)
        {
          j = 1;
        }

      buf[j] = ch;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void udp_client(void)
{
#ifdef CONFIG_EXAMPLES_UDP_IPv6
  struct sockaddr_in6 server;
#else
  struct sockaddr_in server;
#endif
  unsigned char outbuf[SENDSIZE];
  socklen_t addrlen;
  int sockfd;
  int nbytes;
  int offset;
#ifdef CONFIG_EXAMPLES_UDP_BROADCAST
  int optval;
  int ret;
#endif

  /* Create a new UDP socket */

  sockfd = create_socket();
  if (sockfd < 0)
    {
      printf("client ERROR: create_socket failed\n");
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

  /* Then send and receive 256 messages */

  for (offset = 0; offset < 256; offset++)
    {
      /* Set up the output buffer */

      fill_buffer(outbuf, offset);

      /* Set up the server address */

#ifdef CONFIG_EXAMPLES_UDP_IPv6
      server.sin6_family     = AF_INET6;
      server.sin6_port       = HTONS(CONFIG_EXAMPLES_UDP_SERVER_PORTNO);
      memcpy(server.sin6_addr.s6_addr16,
             g_udpserver_ipv6, 8 * sizeof(uint16_t));
      addrlen                = sizeof(struct sockaddr_in6);
#else
      server.sin_family      = AF_INET;
      server.sin_port        = HTONS(CONFIG_EXAMPLES_UDP_SERVER_PORTNO);
      server.sin_addr.s_addr = (in_addr_t)g_udpserver_ipv4;
      addrlen                = sizeof(struct sockaddr_in);
#endif

      /* Send the message */

      printf("client: %d. Sending %d bytes\n", offset, SENDSIZE);
      nbytes = sendto(sockfd, outbuf, SENDSIZE, 0,
                      (struct sockaddr *)&server, addrlen);
      printf("client: %d. Sent %d bytes\n", offset, nbytes);

      if (nbytes < 0)
        {
          printf("client: %d. sendto failed: %d\n", offset, errno);
          close(sockfd);
          exit(-1);
        }
      else if (nbytes != SENDSIZE)
        {
          printf("client: %d. Bad send length: %d Expected: %d\n",
                 offset, nbytes, SENDSIZE);
          close(sockfd);
          exit(-1);
        }

      /* Now, sleep a bit.  No packets should be dropped due to overrunning
       * the server.
       */

      sleep(2);
    }

  close(sockfd);
}
