/****************************************************************************
 * apps/examples/udgram/udgram_client.c
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
#include <sys/un.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "udgram.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

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
  struct sockaddr_un server;
  unsigned char outbuf[SENDSIZE];
  socklen_t addrlen;
  int sockfd;
  int nbytes;
  int offset;

  /* Create a new UDP socket */

  sockfd = socket(PF_LOCAL, SOCK_DGRAM, 0);
  if (sockfd < 0)
    {
      printf("client: ERROR socket failure %d\n", errno);
      return 1;
    }

  /* Then send and receive 256 messages */

  for (offset = 0; offset < 256; offset++)
    {
      /* Set up the output buffer */

      fill_buffer(outbuf, offset);

      /* Set up the server address */

      addrlen = strlen(CONFIG_EXAMPLES_UDGRAM_ADDR);
      if (addrlen > UNIX_PATH_MAX - 1)
        {
          addrlen = UNIX_PATH_MAX - 1;
        }

      server.sun_family = AF_LOCAL;
      strncpy(server.sun_path, CONFIG_EXAMPLES_UDGRAM_ADDR, addrlen);
      server.sun_path[addrlen] = '\0';

      addrlen += sizeof(sa_family_t) + 1;

      /* Send the message */

      printf("client: %d. Sending %d bytes\n", offset, SENDSIZE);
      nbytes = sendto(sockfd, outbuf, SENDSIZE, 0,
                      (struct sockaddr *)&server, addrlen);

      if (nbytes < 0)
        {
          printf("client: %d. ERROR sendto failed: %d\n", offset, errno);
          close(sockfd);
          return 1;
        }
      else if (nbytes != SENDSIZE)
        {
          printf("client: %d. ERROR Bad send length: %d Expected: %d\n",
                 offset, nbytes, SENDSIZE);
          close(sockfd);
          return 1;
        }

      printf("client: %d. Sent %d bytes\n", offset, nbytes);
    }

  close(sockfd);
  return 0;
}
