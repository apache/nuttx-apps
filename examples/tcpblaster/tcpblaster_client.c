/****************************************************************************
 * apps/examples/tcpblaster/tcpblaster_client.c
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

#include <sys/socket.h>
#include <netinet/in.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <poll.h>
#include <string.h>
#include <errno.h>

#include <arpa/inet.h>

#include "tcpblaster.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void tcpblaster_client(void)
{
#ifdef CONFIG_EXAMPLES_TCPBLASTER_IPv6
  struct sockaddr_in6 server;
#else
  struct sockaddr_in server;
#endif
  struct timespec start;
  socklen_t addrlen;
  FAR char *outbuf;
  unsigned long sendtotal;
  unsigned long totallost;
  int groupcount;
  int sendcount;
  int partials;
  int sockfd;
  int nbytessent;
  int ch;
  int i;
  char timebuff[100];

  setbuf(stdout, NULL);

  /* Allocate buffers */

  outbuf = (FAR char *)malloc(SENDSIZE);
  if (!outbuf)
    {
      printf("client: failed to allocate buffers\n");
      exit(1);
    }

  /* Create a new TCP socket */

  sockfd = socket(PF_INETX, SOCK_STREAM, 0);
  if (sockfd < 0)
    {
      printf("client socket failure %d\n", errno);
      goto errout_with_buffers;
    }

  /* Set up the server address */

#ifdef CONFIG_EXAMPLES_TCPBLASTER_IPv6
  server.sin6_family     = AF_INET6;
  server.sin6_port       = HTONS(CONFIG_EXAMPLES_TCPBLASTER_SERVER_PORTNO);
  memcpy(server.sin6_addr.s6_addr, g_tcpblasterserver_ipv6, 16);
  addrlen                = sizeof(struct sockaddr_in6);

  printf("Connecting to IPv6 Address: "
         "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\n",
         g_tcpblasterserver_ipv6[0], g_tcpblasterserver_ipv6[1],
         g_tcpblasterserver_ipv6[2], g_tcpblasterserver_ipv6[3],
         g_tcpblasterserver_ipv6[4], g_tcpblasterserver_ipv6[5],
         g_tcpblasterserver_ipv6[6], g_tcpblasterserver_ipv6[7]);
#else
  server.sin_family      = AF_INET;
  server.sin_port        = HTONS(CONFIG_EXAMPLES_TCPBLASTER_SERVER_PORTNO);
  server.sin_addr.s_addr = (in_addr_t)g_tcpblasterserver_ipv4;
  addrlen                = sizeof(struct sockaddr_in);

  printf("Connecting to IPv4 Address: %08lx\n",
         (unsigned long)g_tcpblasterserver_ipv4);
#endif

  /* Connect the socket to the server */

  if (connect(sockfd, (FAR struct sockaddr *)&server, addrlen) < 0)
    {
      printf("client: connect failure: %d\n", errno);
      goto errout_with_socket;
    }

  printf("client: Connected\n");

  /* Initialize the buffer */

  ch = 0x20;
  for (i = 0; i < SENDSIZE; i++ )
    {
      outbuf[i] = ch;
      if (++ch > 0x7e)
        {
          ch = 0x20;
        }
    }

  /* Then send messages forever */

  groupcount = 0;
  sendcount = 0;
  sendtotal = 0;
  partials  = 0;
  totallost = 0;

  clock_gettime(CLOCK_REALTIME, &start);

  for (; ; )
    {
#ifdef CONFIG_EXAMPLES_TCPBLASTER_POLLOUT
      struct pollfd fds[1];
      int ret;

      memset(fds, 0, 1 * sizeof(struct pollfd));
      fds[0].fd     = sockfd;
      fds[0].events = POLLOUT | POLLHUP;

      /* Wait until we can send data or until the connection is lost */

      ret = poll(fds, 1, -1);
      if (ret < 0)
        {
          printf("client: ERROR poll failed: %d\n", errno);
          goto errout_with_socket;
        }

      if ((fds[0].revents & POLLHUP) != 0)
        {
          printf("client: WARNING poll returned POLLHUP\n");
          goto errout_with_socket;
        }
#endif

      nbytessent = send(sockfd, outbuf, SENDSIZE, 0);
      if (nbytessent < 0)
        {
          printf("client: send failed: %d\n", errno);
          goto errout_with_socket;
        }
      else if (nbytessent > 0 && nbytessent < SENDSIZE)
        {
          /* Partial buffers can be sent if there is insufficient buffering
           * space to buffer the whole SENDSIZE request.  This is not an
           * error, but is an interesting thing to keep track of.
           */

          partials++;
          totallost += (SENDSIZE - nbytessent);
        }
      else if (nbytessent != SENDSIZE)
        {
          printf("client: Bad send length=%d: %d of \n",
                  nbytessent, SENDSIZE);
          goto errout_with_socket;
        }

      sendtotal += nbytessent;

      if (++sendcount >= GROUPSIZE)
        {
          struct timespec elapsed;
          struct timespec curr;
          float fkbrecvd;
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

          fkbrecvd = sendtotal / 1024.0f;
          felapsed = elapsed.tv_sec + elapsed.tv_nsec / 1000000000.0f;
          printf("[%s] %d: Sent %d %d-byte buffers: %7.1fKB "
                 "(avg %5.1f KB) in %6.2f seconds (%7.1f KB/second)\n",
                 timebuff, groupcount, sendcount, SENDSIZE, fkbrecvd,
                 fkbrecvd / sendcount, felapsed, fkbrecvd / felapsed);

          if (partials > 0)
            {
              float fkblost;

              fkblost = (float)totallost / 1024.0;
              printf("Partial buffers sent: %d Data not sent: %7.1f Kb\n",
                     partials, fkblost);
            }

          sendcount  = 0;
          sendtotal  = 0;
          partials   = 0;
          totallost  = 0;
          groupcount++;

          clock_gettime(CLOCK_REALTIME, &start);
        }
    }

  free(outbuf);
  return;

errout_with_socket:
  close(sockfd);

errout_with_buffers:
  free(outbuf);
  exit(1);
}
