/****************************************************************************
 * examples/pf_ieee802154/pf_client.c
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

#include <sys/types.h>
#include <sys/socket.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "pfieee802154.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int create_socket(void)
{
  struct sockaddr_ieee802154_s addr;
  socklen_t addrlen;
  int sockfd;

  /* Create a new DGRAM socket */

  sockfd = socket(AF_IEEE802154, SOCK_DGRAM, 0);
  if (sockfd < 0)
    {
      printf("client ERROR: client socket failure %d\n", errno);
      return -1;
    }

  /* Bind the DGRAM socket to the radio address */

  addr.sa_family = AF_IEEE802154;
  memcpy(&addr.sa_addr, &g_server_addr, sizeof(struct ieee802154_saddr_s));
  addrlen = sizeof(struct sockaddr_ieee802154_s);

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

int main(int argc, FAR char *argv[])
{
  struct sockaddr_ieee802154_s server;
  unsigned char outbuf[SENDSIZE];
  socklen_t addrlen;
  int sockfd;
  int nbytes;
  int offset;

  /* Parse any command line options */

  pf_cmdline(argc, argv);

  /* Create a new DGRAM socket */

  sockfd = create_socket();
  if (sockfd < 0)
    {
      printf("client ERROR: create_socket failed %d\n");
      exit(1);
    }

  /* Then send and receive 256 messages */

  for (offset = 0; offset < 256; offset++)
    {
      /* Set up the output buffer */

      fill_buffer(outbuf, offset);

      /* Set up the server address */

      server.sa_family = AF_IEEE802154;
      memcpy(&server.sa_addr, &g_server_addr, sizeof(struct ieee802154_saddr_s));
      addrlen          = sizeof(struct sockaddr_ieee802154_s);

      /* Send the message */

      printf("client: %d. Sending %d bytes\n", offset, SENDSIZE);
      nbytes = sendto(sockfd, outbuf, SENDSIZE, 0,
                      (struct sockaddr*)&server, addrlen);
      printf("client: %d. Sent %d bytes\n", offset, nbytes);

      if (nbytes < 0)
        {
          printf("client: %d. sendto failed: %d\n", offset, errno);
          close(sockfd);
          exit(1);
        }
      else if (nbytes != SENDSIZE)
        {
          printf("client: %d. Bad send length: %d Expected: %d\n",
                 offset, nbytes, SENDSIZE);
          close(sockfd);
          exit(1);
        }

      /* Now, sleep a bit.  No packets should be dropped due to overrunning
       * the server.
       */

      sleep(2);
    }

  close(sockfd);
  return 0;
}
