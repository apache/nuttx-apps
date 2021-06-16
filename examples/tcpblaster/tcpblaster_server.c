/****************************************************************************
 * apps/examples/tcpblaster/tcpblaster-server.c
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
#include <poll.h>
#include <netinet/in.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#include <arpa/inet.h>

#include "tcpblaster.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void tcpblaster_server(void)
{
#ifdef CONFIG_EXAMPLES_TCPBLASTER_IPv6
  struct sockaddr_in6 myaddr;
#else
  struct sockaddr_in myaddr;
#endif
#ifdef TCPBLASTER_HAVE_SOLINGER
  struct linger ling;
#endif
  struct timespec start;
  unsigned long recvtotal;
  socklen_t addrlen;
  char *buffer;
  int groupcount;
  int recvcount;
  int listensd;
  int acceptsd;
  int nbytesread;
  int optval;
  char timebuff[100];

  setbuf(stdout, NULL);

  /* Allocate a BIG buffer */

  buffer = (FAR char *)malloc(SENDSIZE);
  if (!buffer)
    {
      printf("server: failed to allocate buffer\n");
      exit(1);
    }

  /* Create a new TCP socket */

  listensd = socket(PF_INETX, SOCK_STREAM, 0);
  if (listensd < 0)
    {
      printf("server: socket failure: %d\n", errno);
      goto errout_with_buffer;
    }

  /* Set socket to reuse address */

  optval = 1;
  if (setsockopt(listensd, SOL_SOCKET, SO_REUSEADDR, (FAR void *)&optval,
                 sizeof(int)) < 0)
    {
      printf("server: setsockopt SO_REUSEADDR failure: %d\n", errno);
      goto errout_with_listensd;
    }

  /* Bind the socket to a local address */

#ifdef CONFIG_EXAMPLES_TCPBLASTER_IPv6

  myaddr.sin6_family = AF_INET6;
  myaddr.sin6_port   = HTONS(CONFIG_EXAMPLES_TCPBLASTER_SERVER_PORTNO);
#if defined(CONFIG_EXAMPLES_TCPBLASTER_LOOPBACK) && !defined(CONFIG_NET_LOOPBACK)
  memcpy(myaddr.sin6_addr.s6_addr, g_tcpblasterserver_ipv6, 16);
#else
  memset(myaddr.sin6_addr.s6_addr, 0, 16);
#endif
  addrlen = sizeof(struct sockaddr_in6);

  printf("Binding to IPv6 Address: "
         "%02x%02x:%02x%02x:%02x%02x:%02x%02x:"
         "%02x%02x:%02x%02x:%02x%02x:%02x%02x\n",
         myaddr.sin6_addr.s6_addr[0], myaddr.sin6_addr.s6_addr[1],
         myaddr.sin6_addr.s6_addr[2], myaddr.sin6_addr.s6_addr[3],
         myaddr.sin6_addr.s6_addr[4], myaddr.sin6_addr.s6_addr[5],
         myaddr.sin6_addr.s6_addr[6], myaddr.sin6_addr.s6_addr[7],
         myaddr.sin6_addr.s6_addr[8], myaddr.sin6_addr.s6_addr[9],
         myaddr.sin6_addr.s6_addr[10], myaddr.sin6_addr.s6_addr[11],
         myaddr.sin6_addr.s6_addr[12], myaddr.sin6_addr.s6_addr[13],
         myaddr.sin6_addr.s6_addr[14], myaddr.sin6_addr.s6_addr[15]);
#else
  myaddr.sin_family  = AF_INET;
  myaddr.sin_port    = HTONS(CONFIG_EXAMPLES_TCPBLASTER_SERVER_PORTNO);

#if defined(CONFIG_EXAMPLES_TCPBLASTER_LOOPBACK) && !defined(CONFIG_NET_LOOPBACK)
  myaddr.sin_addr.s_addr        = (in_addr_t)g_tcpblasterserver_ipv4;
#else
  myaddr.sin_addr.s_addr        = INADDR_ANY;
#endif
  addrlen = sizeof(struct sockaddr_in);

  printf("Binding to IPv4 Address: %08lx\n",
         (unsigned long)myaddr.sin_addr.s_addr);
#endif

  if (bind(listensd, (FAR struct sockaddr *)&myaddr, addrlen) < 0)
    {
      printf("server: bind failure: %d\n", errno);
      goto errout_with_listensd;
    }

  /* Listen for connections on the bound TCP socket */

  if (listen(listensd, 5) < 0)
    {
      printf("server: listen failure %d\n", errno);
      goto errout_with_listensd;
    }

  /* Accept only one connection */

  printf("server: Accepting connections on port %d\n",
         CONFIG_EXAMPLES_TCPBLASTER_SERVER_PORTNO);
  acceptsd = accept(listensd, (FAR struct sockaddr *)&myaddr, &addrlen);
  if (acceptsd < 0)
    {
      printf("server: accept failure: %d\n", errno);
      goto errout_with_listensd;
    }

  printf("server: Connection accepted -- receiving\n");

  /* Configure to "linger" until all data is sent when the socket is closed */

#ifdef TCPBLASTER_HAVE_SOLINGER
  ling.l_onoff  = 1;
  ling.l_linger = 30;     /* timeout is seconds */

  if (setsockopt(acceptsd, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling)) < 0)
    {
      printf("server: setsockopt SO_LINGER failure: %d\n", errno);
      goto errout_with_acceptsd;
    }
#endif

  /* Then receive data forever */

  recvcount = 0;
  recvtotal = 0;
  groupcount = 0;

  clock_gettime(CLOCK_REALTIME, &start);

  for (; ; )
    {
#ifdef CONFIG_EXAMPLES_TCPBLASTER_POLLIN
      struct pollfd fds[1];
      int ret;

      memset(fds, 0, 1 * sizeof(struct pollfd));
      fds[0].fd     = acceptsd;
      fds[0].events = POLLIN | POLLHUP;

      /* Wait until we can receive data or until the connection is lost */

      ret = poll(fds, 1, -1);
      if (ret < 0)
        {
          printf("server: ERROR poll failed: %d\n", errno);
          goto errout_with_acceptsd;
        }

      if ((fds[0].revents & POLLHUP) != 0)
        {
          printf("server: WARNING poll returned POLLHUP\n");
          goto errout_with_acceptsd;
        }
#endif

      nbytesread = recv(acceptsd, buffer, SENDSIZE, 0);
      if (nbytesread < 0)
        {
          printf("server: recv failed: %d\n", errno);
          goto errout_with_acceptsd;
        }
      else if (nbytesread == 0)
        {
          printf("server: The client broke the connection\n");
          goto errout_with_acceptsd;
        }

      recvtotal += nbytesread;

      if (++recvcount >= GROUPSIZE)
        {
          struct timespec elapsed;
          struct timespec curr;
          float fkbsent;
          float felapsed;

          clock_gettime(CLOCK_REALTIME, &curr);

          elapsed.tv_sec  = curr.tv_sec - start.tv_sec;
          if (curr.tv_nsec >= start.tv_nsec)
            {
              elapsed.tv_nsec = curr.tv_nsec - start.tv_nsec;
            }
          else
            {
              unsigned long borrow = 1000000000 - start.tv_nsec;
              elapsed.tv_sec--;
              elapsed.tv_nsec = curr.tv_nsec + borrow;
            }

          strftime(timebuff, 100,
                   "%Y-%m-%d %H:%M:%S.000", localtime(&curr.tv_sec));

          fkbsent  = recvtotal / 1024.0f;
          felapsed = elapsed.tv_sec + elapsed.tv_nsec / 1000000000.0f;
          printf("[%s] %d: Received %d buffers: %7.1f KB (buffer average"
                 "size: %5.1f KB) in %6.2f seconds (%7.1f KB/second)\n",
                 timebuff, groupcount, recvcount, fkbsent,
                 fkbsent / recvcount, felapsed, fkbsent / felapsed);

          recvcount       = 0;
          recvtotal       = 0;
          groupcount++;

          clock_gettime(CLOCK_REALTIME, &start);
        }
    }

errout_with_acceptsd:
  close(acceptsd);

errout_with_listensd:
  close(listensd);

errout_with_buffer:
  free(buffer);
  exit(1);
}
