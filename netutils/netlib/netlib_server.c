/****************************************************************************
 * apps/netutils/netlib/netlib_server.c
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

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <debug.h>

#include <netinet/in.h>

#include "netutils/netlib.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_server
 *
 * Description:
 *   Implement basic server logic
 *
 * Parameters:
 *   portno    The port to listen on (in network byte order)
 *   handler   The entrypoint of the task to spawn when a new connection is
 *             accepted.
 *   stacksize The stack size needed by the spawned task
 *
 * Return:
 *   Does not return unless an error occurs.
 *
 ****************************************************************************/

void netlib_server(uint16_t portno,
                   pthread_startroutine_t handler, int stacksize)
{
  struct sockaddr_in myaddr;
#ifdef CONFIG_NET_SOLINGER
  struct linger ling;
#endif
  pthread_t child;
  pthread_attr_t attr;
  socklen_t addrlen;
  int listensd;
  int acceptsd;
  int ret;

  /* Create a new TCP socket to use to listen for connections */

  listensd = netlib_listenon(portno);
  if (listensd < 0)
    {
      return;
    }

  /* Begin serving connections */

  for (; ; )
    {
      /* Accept the next connectin */

      addrlen = sizeof(struct sockaddr_in);
      acceptsd = accept(listensd, (struct sockaddr *)&myaddr, &addrlen);
      if (acceptsd < 0)
        {
          nerr("ERROR: accept failure: %d\n", errno);
          break;
        }

      ninfo("Connection accepted -- spawning sd=%d\n", acceptsd);

      /* Configure to "linger" until all data is sent when the socket is
       * closed.
       */

#ifdef CONFIG_NET_SOLINGER
      ling.l_onoff  = 1;
      ling.l_linger = 30;     /* timeout is seconds */

      ret = setsockopt(acceptsd, SOL_SOCKET,
                       SO_LINGER, &ling, sizeof(struct linger));
      if (ret < 0)
        {
          close(acceptsd);
          nerr("ERROR: setsockopt SO_LINGER failure: %d\n", errno);
          break;
        }
#endif

      /* Create a thread to handle the connection.  The socket descriptor is
       * provided in as the single argument to the new thread.
       */

      pthread_attr_init(&attr);
      pthread_attr_setstacksize(&attr, stacksize);

      ret = pthread_create(&child, &attr,
                           handler, (pthread_addr_t)((uintptr_t)acceptsd));
      if (ret != 0)
        {
          /* Close the connection */

          close(acceptsd);
          nerr("ERROR: pthread_create failed\n");

          if (ret == EAGAIN)
            {
              /* Lacked resources to create a new thread. This is a temporary
               * condition, so we close this peer, but keep serving for
               * other connections.
               */

              continue;
            }

          /* Something is very wrong... Break out and stop serving */

          break;
        }

      /* We don't care when/how the child thread exits so detach from it now
       * in order to avoid memory leaks.
       */

      pthread_detach(child);
    }

  /* Close the listerner socket */

  close(listensd);
}
