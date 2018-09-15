/****************************************************************************
 * examples/netloop/lo_main.c
 *
 *   Copyright (C) 2015 Gregory Nutt. All rights reserved.
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
 * 3. Neither the name NuttX nor the names of its contributors may be
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

  ret = setsockopt(sockfd, SOL_TCP, TCP_KEEPIDLE, &tv, sizeof(struct timeval));
  if (ret < 0)
    {
      ret = -errno;
      fprintf(stderr, "setsockopt(TCP_KEEPIDLE) failed: %d\n", ret);
      return ret;
    }

  tv.tv_sec  = 1;
  tv.tv_usec = 0;

  ret = setsockopt(sockfd, SOL_TCP, TCP_KEEPINTVL, &tv, sizeof(struct timeval));
  if (ret < 0)
    {
      ret = -errno;
      fprintf(stderr, "setsockopt(TCP_KEEPIDLE) failed: %d\n", ret);
      return ret;
    }

  value = 3;
  ret = setsockopt(sockfd, SOL_TCP, TCP_KEEPCNT, &value, sizeof(int));
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

  printf("lo_client: Connecting to %08x:%d...\n", LO_ADDRESS, LISTENER_PORT);
  if (connect( sockfd, (struct sockaddr*)&myaddr, sizeof(struct sockaddr_in)) < 0)
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
          printf("lo_client: Bad send length: %d Expected: %d\n", nbytessent, len);
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
          printf("lo_client: Bad recv length: %d Expected: %d\n", nbytesrecvd, len);
          goto errout_with_socket;
        }
      else if (memcmp(inbuf, outbuf, len) != 0)
        {
          ret = -EINVAL;
          printf("lo_client: Received outbuf does not match sent outbuf\n");
          goto errout_with_socket;
        }

#ifdef CONFIG_EXAMPLES_NETLOOP_KEEPALIVE
      /* Send four messages, then wait a longer amount of time ... longer than
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

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int netloop_main(int argc, char *argv[])
#endif
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
