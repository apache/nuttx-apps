/****************************************************************************
 * apps/examples/pf_ieee802154/pf_client.c
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
      printf("client ERROR: create_socket failed\n");
      exit(1);
    }

  /* Then send and receive 256 messages */

  for (offset = 0; offset < 256; offset++)
    {
      /* Set up the output buffer */

      fill_buffer(outbuf, offset);

      /* Set up the server address */

      server.sa_family = AF_IEEE802154;
      memcpy(&server.sa_addr, &g_server_addr,
             sizeof(struct ieee802154_saddr_s));
      addrlen          = sizeof(struct sockaddr_ieee802154_s);

      /* Send the message */

      printf("client: %d. Sending %d bytes\n", offset, SENDSIZE);
      nbytes = sendto(sockfd, outbuf, SENDSIZE, 0,
                      (struct sockaddr *)&server, addrlen);
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
