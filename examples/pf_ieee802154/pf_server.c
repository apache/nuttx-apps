/****************************************************************************
 * examples/pf_ieee802154/pf_server.c
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
          printf("server: Buffer content error for offset=%d, index=%d\n", offset, j);
          ret = 0;
        }
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int pfserver_main(int argc, char *argv[])
#endif
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
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void*)&optval, sizeof(int)) < 0)
    {
      printf("server: setsockopt SO_REUSEADDR failure: %d\n", errno);
      exit(1);
    }

  /* Bind the socket to a local address */

  server.sa_family = AF_IEEE802154;
  memcpy(&server.sa_addr, &g_server_addr, sizeof(struct ieee802154_saddr_s));
  addrlen = sizeof(struct sockaddr_ieee802154_s);

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
                        (struct sockaddr *)&client, &recvlen);

      if (client.sa_addr.s_mode == IEEE802154_ADDRMODE_SHORT)
        {
          printf("server: %d. Received %d bytes from %02x:%02x\n",
                 offset, nbytes,
                 client.sa_addr.s_saddr[0], client.sa_addr.s_saddr[1]);
        }
      else if (client.sa_addr.s_mode == IEEE802154_ADDRMODE_SHORT)
        {
          printf("server: %d. Received %d bytes from "
                 "%02x:%02x:%02x:%02x:%02x:%02x:%02x: %02x\n",
                 offset, nbytes,
                 client.sa_addr.s_saddr[0], client.sa_addr.s_saddr[1],
                 client.sa_addr.s_saddr[2], client.sa_addr.s_saddr[3],
                 client.sa_addr.s_saddr[4], client.sa_addr.s_saddr[5],
                 client.sa_addr.s_saddr[6], client.sa_addr.s_saddr[7]);
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
          printf("server: %d. recv size incorrect: %d vs %d\n", offset, nbytes, SENDSIZE);
          close(sockfd);
          exit(1);
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
