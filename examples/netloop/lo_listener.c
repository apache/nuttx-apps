/****************************************************************************
 * apps/examples/netloop/lo_listener.c
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

#include <sys/stat.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <debug.h>

#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "netutils/netlib.h"

#include "netloop.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define IOBUFFER_SIZE 80

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct net_listener_s
{
  struct sockaddr_in addr;
  fd_set          master;
  fd_set          working;
  char            buffer[IOBUFFER_SIZE];
  int             listensd;
  int             mxsd;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: net_closeclient
 ****************************************************************************/

static bool net_closeclient(struct net_listener_s *nls, int sd)
{
  printf("lo_listener: Closing host side connection sd=%d\n", sd);
  close(sd);
  FD_CLR(sd, &nls->master);

  /* If we just closed the max SD, then search downward for the next biggest SD. */

  while (FD_ISSET(nls->mxsd, &nls->master) == false)
    {
      nls->mxsd -= 1;
    }

  return true;
}

/****************************************************************************
 * Name: net_incomingdata
 ****************************************************************************/

static inline bool net_incomingdata(struct net_listener_s *nls, int sd)
{
  char *ptr;
  int nbytes;
  int ret;

  /* Read data from the socket */

#ifdef FIONBIO
  for (;;)
#endif
    {
      printf("lo_listener: Read data from sd=%d\n", sd);
      ret = recv(sd, nls->buffer, IOBUFFER_SIZE, 0);
      if (ret < 0)
        {
          if (errno != EINTR)
            {
              printf("lo_listener: recv failed sd=%d: %d\n", sd, errno);
              if (errno != EAGAIN)
                {
                  net_closeclient(nls, sd);
                  return false;
                }
            }
        }
      else if (ret == 0)
        {
          printf("lo_listener: Client connection lost sd=%d\n", sd);
          net_closeclient(nls, sd);
          return false;
        }
      else
        {
          nls->buffer[ret]='\0';
          printf("lo_listener: Read '%s' (%d bytes)\n", nls->buffer, ret);

          /* Echo the data back to the client */

          for (nbytes = ret, ptr = nls->buffer; nbytes > 0; )
            {
              ret = send(sd, ptr, nbytes, 0);
              if (ret < 0)
                {
                  if (errno != EINTR)
                    {
                       printf("lo_listener: Send failed sd=%d: %d\n", sd, errno);
                       net_closeclient(nls, sd);
                       return false;
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

  return 0;
}

/****************************************************************************
 * Name: net_connection
 ****************************************************************************/

static inline bool net_connection(struct net_listener_s *nls)
{
  int sd;

  /* Loop until all connections have been processed */

#ifdef FIONBIO
  for (;;)
#endif
    {
      printf("lo_listener: Accepting new connection on sd=%d\n", nls->listensd);

      sd = accept(nls->listensd, NULL, NULL);
      if (sd < 0)
        {
          printf("lo_listener: accept failed: %d\n", errno);

          if (errno != EINTR)
            {
              return false;
            }
        }
      else
        {
          /* Add the new connection to the master set */

          printf("lo_listener: Connection accepted for sd=%d\n", sd);

          FD_SET(sd, &nls->master);
          if (sd > nls->mxsd)
            {
               nls->mxsd = sd;
            }

          return true;
        }
    }

  return false;
}

/****************************************************************************
 * Name: net_mksocket
 ****************************************************************************/

static inline bool net_mksocket(struct net_listener_s *nls)
{
  int value;
  int ret;

  /* Create a listening socket */

  printf("lo_listener: Initializing listener socket\n");
  nls->listensd = socket(AF_INET, SOCK_STREAM, 0);
  if (nls->listensd < 0)
    {
      printf("lo_listener: socket failed: %d\n", errno);
      return false;
    }

  /* Configure the socket */

  value = 1;
  ret = setsockopt(nls->listensd, SOL_SOCKET,  SO_REUSEADDR, (char*)&value, sizeof(int));
  if (ret < 0)
    {
      printf("lo_listener: setsockopt failed: %d\n", errno);
      close(nls->listensd);
      return false;
    }

  /* Set the socket to non-blocking */

#ifdef FIONBIO
  ret = ioctl(nls->listensd, FIONBIO, (char *)&value);
  if (ret < 0)
    {
      printf("lo_listener: ioctl failed: %d\n", errno);
      close(nls->listensd);
      return false;
    }
#endif

  /* Bind the socket */

  memset(&nls->addr, 0, sizeof(struct sockaddr_in));
  nls->addr.sin_family      = AF_INET;
  nls->addr.sin_port        = htons(LISTENER_PORT);
  nls->addr.sin_addr.s_addr = htonl(LO_ADDRESS);
  ret = bind(nls->listensd, (struct sockaddr *)&nls->addr, sizeof(struct sockaddr_in));
  if (ret < 0)
    {
      printf("lo_listener: bind failed: %d\n", errno);
      close(nls->listensd);
      return false;
    }

  /* Mark the socket as a listener */

  ret = listen(nls->listensd, 32);
  if (ret < 0)
    {
      printf("lo_listener: bind failed: %d\n", errno);
      close(nls->listensd);
      return false;
    }

  return true;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lo_listener
 ****************************************************************************/

void *lo_listener(pthread_addr_t pvarg)
{
  struct net_listener_s nls;
  struct timeval timeout;
  int nsds;
  int ret;
  int i;

  /* Set up a listening socket */

  memset(&nls, 0, sizeof(struct net_listener_s));
  if (!net_mksocket(&nls))
    {
       return (void*)1;
    }

  /* Initialize the 'master' file descriptor set */

  FD_ZERO(&nls.master);
  nls.mxsd = nls.listensd;
  FD_SET(nls.listensd, &nls.master);

  /* Set up a 3 second timeout */

  timeout.tv_sec  = LISTENER_DELAY;
  timeout.tv_usec = 0;

  /* Loop waiting for incoming connections or for incoming data
   * on any of the connect sockets.
   */

  for (;;)
    {
      /* Wait on select */

      printf("lo_listener: Calling select(), listener sd=%d\n", nls.listensd);
      memcpy(&nls.working, &nls.master, sizeof(fd_set));
      ret = select(nls.mxsd + 1, (FAR fd_set*)&nls.working, (FAR fd_set*)NULL, (FAR fd_set*)NULL, &timeout);
      if (ret < 0)
        {
          printf("lo_listener: select failed: %d\n", errno);
          break;
        }

      /* Check for timeout */

      if (ret == 0)
        {
          printf("lo_listener: Timeout\n");
          continue;
        }

      /* Find which descriptors caused the wakeup */

      nsds = ret;
      for (i = 0; i <= nls.mxsd && nsds > 0; i++)
        {
          /* Is this descriptor ready? */

          if (FD_ISSET(i, &nls.working))
            {
              /* Yes, is it our listener? */

              printf("lo_listener: Activity on sd=%d\n", i);

              nsds--;
              if (i == nls.listensd)
                {
                  net_connection(&nls);
                }
              else
                {
                  net_incomingdata(&nls, i);
                }
            }
        }
    }

  /* Cleanup */

#if 0 /* Don't get here */
   for (i = 0; i <= nls.mxsd; +i++)
    {
      if (FD_ISSET(i, &nls.master))
        {
          close(i);
        }
    }
#endif
  return NULL;  /* Keeps some compilers from complaining */
}
