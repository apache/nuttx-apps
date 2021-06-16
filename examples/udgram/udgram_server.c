/****************************************************************************
 * apps/examples/udgram/udgram_server.c
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

int main(int argc, FAR char *argv[])
{
  struct sockaddr_un server;
  struct sockaddr_un client;
  unsigned char inbuf[1024];
  socklen_t addrlen;
  socklen_t recvlen;
  int sockfd;
  int nbytes;
  int offset;

  /* Create a new UDP socket */

  sockfd = socket(PF_LOCAL, SOCK_DGRAM, 0);
  if (sockfd < 0)
    {
      printf("server: socket failure: %d\n", errno);
      return 1;
    }

  /* Bind the socket to a local address */

  addrlen = strlen(CONFIG_EXAMPLES_UDGRAM_ADDR);
  if (addrlen > UNIX_PATH_MAX - 1)
    {
      addrlen = UNIX_PATH_MAX - 1;
    }

  server.sun_family = AF_LOCAL;
  strncpy(server.sun_path, CONFIG_EXAMPLES_UDGRAM_ADDR, addrlen);
  server.sun_path[addrlen] = '\0';

  addrlen += sizeof(sa_family_t) + 1;

  if (bind(sockfd, (struct sockaddr*)&server, addrlen) < 0)
    {
      printf("server: ERROR bind failure: %d\n", errno);
      return 1;
    }

  /* Then receive up to 256 packets of data */

  for (offset = 0; offset < 256; offset++)
    {
      printf("server: %d. Receiving up 1024 bytes\n", offset);
      recvlen = addrlen;
      nbytes = recvfrom(sockfd, inbuf, 1024, 0,
                        (struct sockaddr*)&client, &recvlen);

      if (nbytes < 0)
        {
          printf("server: %d. ERROR recv failed: %d\n", offset, errno);
          close(sockfd);
          return 1;
        }

      if (recvlen < sizeof(sa_family_t) || recvlen > sizeof(struct sockaddr_un))
        {
          printf("server: %d. ERROR Received %d bytes from client with invalid length %d\n",
                 offset, nbytes, recvlen);
        }
      else if (recvlen == sizeof(sa_family_t))
        {
          printf("server: %d. Received %d bytes from a nameless client\n",
                 offset, nbytes);
        }
      else if (recvlen == sizeof(sa_family_t) + 1)
        {
          printf("server: %d. Received %d bytes from an abstract client\n",
                 offset, nbytes);

          if (client.sun_path[0] != '\0')
            {
              printf("server:  ERROR path not NUL terminated\n");
            }
        }
      else /* if (recvlen > sizeof(sa_family_t)+1 &&
                  recvlen <= sizeof(struct sockaddr_un)) */
        {
          int pathlen = recvlen - sizeof(sa_family_t) - 1;

          printf("server: %d. Received %d bytes from a pathname client %s\n",
                 offset, nbytes, client.sun_path);

          if (client.sun_path[pathlen] != '\0')
            {
              printf("server:  ERROR path not NUL terminated\n");
            }
        }

      if (nbytes != SENDSIZE)
        {
          printf("server: %d. ERROR recv size incorrect: %d vs %d\n",
                 offset, nbytes, SENDSIZE);
          close(sockfd);
          return 1;
        }

      if (offset < inbuf[0])
        {
          printf("server: %d. ERROR %d packets lost, resetting offset\n",
                 offset, inbuf[0] - offset);
          offset = inbuf[0];
        }
      else if (offset > inbuf[0])
        {
          printf("server: %d. ERROR Bad offset in buffer: %d\n",
                 offset, inbuf[0]);
          close(sockfd);
          return 1;
        }

      if (!check_buffer(inbuf))
        {
          printf("server: %d. ERROR Bad buffer contents\n", offset);
          close(sockfd);
          return 1;
        }
    }

  close(sockfd);
  return 0;
}
