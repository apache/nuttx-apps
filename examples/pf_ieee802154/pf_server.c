/****************************************************************************
 * apps/examples/pf_ieee802154/pf_server.c
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
#include <netinet/in.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <arpa/inet.h>

#include "pfieee802154.h"

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

int main(int argc, FAR char *argv[])
{
  struct sockaddr_ieee802154_s server;
  struct sockaddr_ieee802154_s client;
  unsigned char inbuf[1024];
  socklen_t addrlen;
  socklen_t recvlen;
  int sockfd;
  int nbytes;
  int optval;
  int offset;
  int ret;

  /* Parse any command line options */

  pf_cmdline(argc, argv);

  /* Create a new DGRAM socket */

  sockfd = socket(PF_IEEE802154, SOCK_DGRAM, 0);
  if (sockfd < 0)
    {
      printf("server: socket failure: %d\n", errno);
      exit(1);
    }

  /* Set socket to reuse address */

  optval = 1;
  ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (FAR void *)&optval,
                   sizeof(int));
  if (ret < 0)
    {
      printf("server: setsockopt SO_REUSEADDR failure: %d\n", errno);
      exit(1);
    }

  /* Bind the socket to a local address */

  server.sa_family = AF_IEEE802154;
  memcpy(&server.sa_addr, &g_server_addr, sizeof(struct ieee802154_saddr_s));
  addrlen = sizeof(struct sockaddr_ieee802154_s);

  if (bind(sockfd, (FAR struct sockaddr *)&server, addrlen) < 0)
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

      if (client.sa_addr.s_mode == IEEE802154_ADDRMODE_SHORT)
        {
          printf("server: %d. Received %d bytes from %02x:%02x\n",
                 offset, nbytes,
                 client.sa_addr.s_saddr[0], client.sa_addr.s_saddr[1]);
        }
      else if (client.sa_addr.s_mode == IEEE802154_ADDRMODE_EXTENDED)
        {
          printf("server: %d. Received %d bytes from "
                 "%02x:%02x:%02x:%02x:%02x:%02x:%02x: %02x\n",
                 offset, nbytes,
                 client.sa_addr.s_eaddr[0], client.sa_addr.s_eaddr[1],
                 client.sa_addr.s_eaddr[2], client.sa_addr.s_eaddr[3],
                 client.sa_addr.s_eaddr[4], client.sa_addr.s_eaddr[5],
                 client.sa_addr.s_eaddr[6], client.sa_addr.s_eaddr[7]);
        }
      else
        {
          printf("server: %d. Received %d bytes (no address)\n",
                 offset, nbytes);
        }

      if (nbytes < 0)
        {
          printf("server: %d. recv failed: %d\n", offset, errno);
          close(sockfd);
          exit(1);
        }

      if (nbytes != SENDSIZE)
        {
          printf("server: %d. recv size incorrect: %d vs %d\n",
                 offset, nbytes, SENDSIZE);
          close(sockfd);
          exit(1);
        }

      if (offset < inbuf[0])
        {
          printf("server: %d. %d packets lost, resetting offset\n",
                 offset, inbuf[0] - offset);
          offset = inbuf[0];
        }
      else if (offset > inbuf[0])
        {
          printf("server: %d. Bad offset in buffer: %d\n", offset, inbuf[0]);
          close(sockfd);
          exit(1);
        }

      if (!check_buffer(inbuf))
        {
          printf("server: %d. Bad buffer contents\n", offset);
          close(sockfd);
          exit(1);
        }
    }

  close(sockfd);
  return 0;
}
