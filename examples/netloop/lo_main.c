/****************************************************************************
 * apps/examples/netloop/lo_main.c
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
#include <sys/stat.h>
#include <sys/time.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <debug.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "netloop.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define IOBUFFER_SIZE 80

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lo_client
 ****************************************************************************/

static int lo_client(void)
{
  struct sockaddr_in myaddr;
  char outbuf[IOBUFFER_SIZE];
  char inbuf[IOBUFFER_SIZE];
  int sockfd;
  int len;
  int nbytessent;
  int nbytesrecvd;
  int ret;
  int i;

#ifdef CONFIG_EXAMPLES_NETLOOP_KEEPALIVE
  struct timeval tv;
  int value;
#endif

  /* Create a new TCP socket */

  sockfd = socket(PF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    {
      ret = -errno;
      printf("lo_client: socket failure %d\n", ret);
      return ret;
    }

#ifdef CONFIG_EXAMPLES_NETLOOP_KEEPALIVE
  /* Enable TCP KeepAlive */

  value = TRUE;
  ret = setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &value, sizeof(int));
  if (ret < 0)
    {
      ret = -errno;
      fprintf(stderr, "setsockopt(SO_KEEPALIVE) failed: %d\n", ret);
      return ret;
    }

  tv.tv_sec  = 5;
  tv.tv_usec = 0;

  ret = setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPIDLE,
                   &tv, sizeof(struct timeval));
  if (ret < 0)
    {
      ret = -errno;
      fprintf(stderr, "setsockopt(TCP_KEEPIDLE) failed: %d\n", ret);
      return ret;
    }

  tv.tv_sec  = 1;
  tv.tv_usec = 0;

  ret = setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPINTVL,
                   &tv, sizeof(struct timeval));
  if (ret < 0)
    {
      ret = -errno;
      fprintf(stderr, "setsockopt(TCP_KEEPIDLE) failed: %d\n", ret);
      return ret;
    }

  value = 3;
  ret = setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPCNT, &value, sizeof(int));
  if (ret < 0)
    {
      ret = -errno;
      fprintf(stderr, "setsockopt(SO_KEEPALIVE) failed: %d\n", ret);
      return ret;
    }
#endif

  /* Connect the socket to the server */

  myaddr.sin_family      = AF_INET;
  myaddr.sin_port        = htons(LISTENER_PORT);
  myaddr.sin_addr.s_addr = htonl(LO_ADDRESS);

  printf("lo_client: Connecting to %08x:%d...\n",
         LO_ADDRESS, LISTENER_PORT);
  if (connect(sockfd,
              (struct sockaddr *)&myaddr, sizeof(struct sockaddr_in)) < 0)
    {
      ret = -errno;
      printf("lo_client: connect failure: %d\n", ret);
      goto errout_with_socket;
    }

  printf("lo_client: Connected\n");

  /* Then send and receive messages */

  for (i = 0; ; i++)
    {
      sprintf(outbuf, "Loopback message %d", i);
      len = strlen(outbuf);

      printf("lo_client: Sending '%s' (%d bytes)\n", outbuf, len);
      nbytessent = send(sockfd, outbuf, len, 0);
      printf("lo_client: Sent %d bytes\n", nbytessent);

      if (nbytessent < 0)
        {
          ret = -errno;
          printf("lo_client: send failed: %d\n", ret);
          goto errout_with_socket;
        }
      else if (nbytessent != len)
        {
          ret = -EINVAL;
          printf("lo_client: Bad send length: %d Expected: %d\n",
                 nbytessent, len);
          goto errout_with_socket;
        }

      printf("lo_client: Receiving...\n");
      nbytesrecvd = recv(sockfd, inbuf, IOBUFFER_SIZE, 0);

      if (nbytesrecvd < 0)
        {
          ret = -errno;
          printf("lo_client: recv failed: %d\n", ret);
          goto errout_with_socket;
        }
      else if (nbytesrecvd == 0)
        {
          ret = -ENOTCONN;
          printf("lo_client: The server broke the connections\n");
          goto errout_with_socket;
        }

      inbuf[nbytesrecvd] = '\0';
      printf("lo_client: Received '%s' (%d bytes)\n", inbuf, nbytesrecvd);

      if (nbytesrecvd != len)
        {
          ret = -EINVAL;
          printf("lo_client: Bad recv length: %d Expected: %d\n",
                 nbytesrecvd, len);
          goto errout_with_socket;
        }
      else if (memcmp(inbuf, outbuf, len) != 0)
        {
          ret = -EINVAL;
          printf("lo_client: Received outbuf does not match sent outbuf\n");
          goto errout_with_socket;
        }

#ifdef CONFIG_EXAMPLES_NETLOOP_KEEPALIVE
      /* Send four messages, then wait a longer amount of time ...
       * longer than:
       * TCP_KEEPIDLE + TCP_KEEPINTVL * TCP_KEEPCNT = 5 + 3 * 1 = 8 seconds.
       */

      if (i != 0 && (i & 3) == 0)
        {
          printf("lo_client: Long sleep\n");
          sleep(12);
        }
      else
#endif
        {
          printf("lo_client: Sleeping\n");
          sleep(6);
        }
    }

  close(sockfd);
  return 0;

errout_with_socket:
  close(sockfd);
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netloop_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  pthread_t tid;
  int ret;

  /* Start the listeners */

  printf("netloop_main: Starting lo_listener thread\n");

  ret = pthread_create(&tid, NULL, lo_listener, NULL);
  if (ret != 0)
    {
      printf("netloop_main: Failed to create lo_listener thread: %d\n", ret);
    }

  /* Run the client */

  ret = lo_client();
  return 0;
}
