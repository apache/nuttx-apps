/****************************************************************************
 * apps/netutils/netlib/netlib_listenon.c
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
#include <errno.h>
#include <debug.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "netutils/netlib.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_listenon
 *
 * Description:
 *   Implement basic server listening
 *
 * Parameters:
 *   portno    The port to listen on (in network byte order)
 *
 * Return:
 *   A valid listening socket or -1 on error.
 *
 ****************************************************************************/

int netlib_listenon(uint16_t portno)
{
  struct sockaddr_in myaddr;
  int listensd;
  int optval;

  /* Create a new TCP socket to use to listen for connections */

  listensd = socket(PF_INET, SOCK_STREAM, 0);
  if (listensd < 0)
    {
      nerr("ERROR: socket failure: %d\n", errno);
      return ERROR;
    }

  /* Set socket to reuse address */

  optval = 1;
  if (setsockopt(listensd, SOL_SOCKET,
                 SO_REUSEADDR, (void *)&optval, sizeof(int)) < 0)
    {
      nwarn("WARNING: setsockopt SO_REUSEADDR failure: %d\n", errno);
    }

  /* Bind the socket to a local address */

  myaddr.sin_family      = AF_INET;
  myaddr.sin_port        = portno;
  myaddr.sin_addr.s_addr = INADDR_ANY;

  if (bind(listensd, (struct sockaddr *)&myaddr,
           sizeof(struct sockaddr_in)) < 0)
    {
      nerr("ERROR: bind failure: %d\n", errno);
      goto errout_with_socket;
    }

  /* Listen for connections on the bound TCP socket */

  if (listen(listensd, 5) < 0)
    {
      nerr("ERROR: listen failure %d\n", errno);
      goto errout_with_socket;
    }

  /* Begin accepting connections */

  ninfo("Accepting connections on port %d\n", ntohs(portno));
  return listensd;

errout_with_socket:
  close(listensd);
  return ERROR;
}
