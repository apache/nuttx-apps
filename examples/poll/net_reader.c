/****************************************************************************
 * apps/examples/poll/net_reader.c
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
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>

#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "netutils/netlib.h"

#include "poll_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define IOBUFFER_SIZE 80

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: net_configure
 ****************************************************************************/

static void net_configure(void)
{
  struct in_addr addr;
#if defined(CONFIG_EXAMPLES_POLL_NOMAC)
  uint8_t mac[IFHWADDRLEN];
#endif

  /* Configure the network
   * Many embedded network interfaces must have a software assigned MAC
   */

#ifdef CONFIG_EXAMPLES_POLL_NOMAC
  mac[0] = 0x00;
  mac[1] = 0xe0;
  mac[2] = 0xde;
  mac[3] = 0xad;
  mac[4] = 0xbe;
  mac[5] = 0xef;
  netlib_setmacaddr("eth0", mac);
#endif

  /* Set up our host address */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_POLL_IPADDR);
  netlib_set_ipv4addr("eth0", &addr);

  /* Set up the default router address */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_POLL_DRIPADDR);
  netlib_set_dripv4addr("eth0", &addr);

  /* Setup the subnet mask */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_POLL_NETMASK);
  netlib_set_ipv4netmask("eth0", &addr);

  /* New versions of netlib_set_ipvXaddr will not bring the network up,
   * So ensure the network is really up at this point.
   */

  netlib_ifup("eth0");
}

/****************************************************************************
 * Name: net_receive
 ****************************************************************************/

static void net_receive(int sd)
{
  struct timeval timeout;
  char buffer[IOBUFFER_SIZE];
  char *ptr;
  fd_set readset;
  int nbytes;
  int ret;

  /* Set up the timeout */

  timeout.tv_sec  = NET_LISTENER_DELAY;
  timeout.tv_usec = 0;

  /* Loop while we have the connection */

  for (; ; )
    {
      /* Wait for incoming message */

      do
        {
          FD_ZERO(&readset);
          FD_SET(sd, &readset);
          ret = select(sd + 1, &readset, NULL, NULL, &timeout);
        }
      while (ret < 0 && errno == EINTR);

      /* Something has happened */

      if (ret < 0)
        {
          printf("net_reader: select failed: %d\n", errno);
          return;
        }
      else if (ret == 0)
        {
          printf("net_reader: Timeout\n");
        }
      else
        {
          printf("net_reader: Read data from sd=%d\n", sd);
          memset(buffer, '?', IOBUFFER_SIZE); /* Just to make sure we really receive something */
          ret = recv(sd, buffer, IOBUFFER_SIZE, 0);
          if (ret < 0)
            {
              if (errno != EINTR)
                {
                  printf("net_reader: recv failed sd=%d: %d\n", sd, errno);
                  if (errno != EAGAIN)
                    {
                       return;
                    }
                }
            }
          else if (ret == 0)
            {
              printf("net_reader: Client connection lost sd=%d\n", sd);
              return;
            }
          else
            {
              buffer[ret] = '\0';
              printf("net_reader: Read '%s' (%d bytes)\n", buffer, ret);

              /* Echo the data back to the client */

              for (nbytes = ret, ptr = buffer; nbytes > 0; )
                {
                  ret = send(sd, ptr, nbytes, 0);
                  if (ret < 0)
                    {
                      if (errno != EINTR)
                        {
                           printf("net_reader: Send failed sd=%d: %d\n",
                                  sd, errno);
                           return;
                        }
                    }
                  else
                    {
                      nbytes -= ret;
                      ptr    += ret;
                    }
                }
            }
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: net_reader
 ****************************************************************************/

void *net_reader(pthread_addr_t pvarg)
{
  struct sockaddr_in addr;
#ifdef POLL_HAVE_SOLINGER
  struct linger ling;
#endif
  int listensd;
  int acceptsd;
  socklen_t addrlen;
  int optval;

  /* Configure the network */

  net_configure();

  /* Create a new TCP socket */

  listensd = socket(PF_INET, SOCK_STREAM, 0);
  if (listensd < 0)
    {
      printf("net_reader: socket failure: %d\n", errno);
      goto errout;
    }

  /* Set socket to reuse address */

  optval = 1;
  if (setsockopt(listensd, SOL_SOCKET, SO_REUSEADDR,
                 &optval, sizeof(int)) < 0)
    {
      printf("net_reader: setsockopt SO_REUSEADDR failure: %d\n", errno);
      goto errout_with_listensd;
    }

  /* Bind the socket to a local address */

  addr.sin_family      = AF_INET;
  addr.sin_port        = HTONS(LISTENER_PORT);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(listensd, (struct sockaddr *)&addr,
           sizeof(struct sockaddr_in)) < 0)
    {
      printf("net_reader: bind failure: %d\n", errno);
      goto errout_with_listensd;
    }

  /* Listen for connections on the bound TCP socket */

  if (listen(listensd, 5) < 0)
    {
      printf("net_reader: listen failure %d\n", errno);
      goto errout_with_listensd;
    }

  /* Connection loop */

  for (; ; )
    {
      /* Accept only one connection */

      printf("net_reader: Accepting new connections on port %d\n",
             LISTENER_PORT);
      addrlen = sizeof(struct sockaddr_in);
      acceptsd = accept(listensd, (struct sockaddr *)&addr, &addrlen);
      if (acceptsd < 0)
        {
          printf("net_reader: accept failure: %d\n", errno);
          continue;
        }

      printf("net_reader: Connection accepted on sd=%d\n", acceptsd);

      /* Configure to "linger" until all data is sent when the socket is
       * closed
       */

#ifdef POLL_HAVE_SOLINGER
      ling.l_onoff  = 1;
      ling.l_linger = 30;     /* timeout is seconds */

      if (setsockopt(acceptsd, SOL_SOCKET, SO_LINGER,
                     &ling, sizeof(struct linger)) < 0)
        {
          printf("net_reader: setsockopt SO_LINGER failure: %d\n", errno);
          goto errout_with_acceptsd;
        }
#endif

      /* Handle incoming messages on the connection. */

      net_receive(acceptsd);

      printf("net_reader: Closing sd=%d\n", acceptsd);
      close(acceptsd);
    }

#ifdef POLL_HAVE_SOLINGER
errout_with_acceptsd:
  close(acceptsd);
#endif
errout_with_listensd:
  close(listensd);
errout:
  return NULL;
}
