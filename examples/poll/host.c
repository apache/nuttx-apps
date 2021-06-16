/****************************************************************************
 * apps/examples/poll/host.c
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

#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#define pthread_addr_t void *
#include "poll_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef TARGETIP
#  error TARGETIP not defined
#endif

#define IOBUFFER_SIZE 80

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * main
 ****************************************************************************/

int main(int argc, char **argv, char **envp)
{
  struct sockaddr_in myaddr;
  char outbuf[IOBUFFER_SIZE];
  char inbuf[IOBUFFER_SIZE];
  int sockfd;
  int len;
  int nbytessent;
  int nbytesrecvd;
  int i;

  /* Create a new TCP socket */

  sockfd = socket(PF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    {
      printf("client socket failure %d\n", errno);
      goto errout_with_outbufs;
    }

  /* Connect the socket to the server */

  myaddr.sin_family      = AF_INET;
  myaddr.sin_port        = htons(LISTENER_PORT);
  myaddr.sin_addr.s_addr = inet_addr(TARGETIP);

  printf("client: Connecting to %s...\n", TARGETIP);
  if (connect( sockfd, (struct sockaddr*)&myaddr, sizeof(struct sockaddr_in)) < 0)
    {
      printf("client: connect failure: %d\n", errno);
      goto errout_with_socket;
    }
  printf("client: Connected\n");

  /* Then send and receive messages */

  for (i = 0; ; i++)
    {
      sprintf(outbuf, "Remote message %d", i);
      len = strlen(outbuf);

      printf("client: Sending '%s' (%d bytes)\n", outbuf, len);
      nbytessent = send(sockfd, outbuf, len, 0);
      printf("client: Sent %d bytes\n", nbytessent);

      if (nbytessent < 0)
        {
          printf("client: send failed: %d\n", errno);
          goto errout_with_socket;
        }
      else if (nbytessent != len)
        {
          printf("client: Bad send length: %d Expected: %d\n", nbytessent, len);
          goto errout_with_socket;
        }

      printf("client: Receiving...\n");
      nbytesrecvd = recv(sockfd, inbuf, IOBUFFER_SIZE, 0);

      if (nbytesrecvd < 0)
        {
          printf("client: recv failed: %d\n", errno);
          goto errout_with_socket;
        }
      else if (nbytesrecvd == 0)
        {
          printf("client: The server broke the connections\n");
          goto errout_with_socket;
        }

      inbuf[nbytesrecvd] = '\0';
      printf("client: Received '%s' (%d bytes)\n", inbuf, nbytesrecvd);

      if (nbytesrecvd != len)
        {
          printf("client: Bad recv length: %d Expected: %d\n", nbytesrecvd, len);
          goto errout_with_socket;
        }
      else if (memcmp(inbuf, outbuf, len) != 0)
        {
          printf("client: Received outbuf does not match sent outbuf\n");
          goto errout_with_socket;
        }

      printf("client: Sleeping\n");
      sleep(8);
    }

  close(sockfd);
  return 0;

errout_with_socket:
  close(sockfd);
errout_with_outbufs:
  exit(1);
}
