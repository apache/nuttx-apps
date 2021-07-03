/****************************************************************************
 * apps/netutils/ftpc/ftpc_connect.c
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

#include "ftpc_config.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <debug.h>

#include <arpa/inet.h>

#include "netutils/ftpc.h"

#include "ftpc_internal.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ftpc_connect
 *
 * Description:
 *   Create a session handle and connect to the server.
 *
 ****************************************************************************/

SESSION ftpc_connect(FAR union ftpc_sockaddr_u *server)
{
  FAR struct ftpc_session_s *session;
  int ret;

  /* Allocate a session structure */

  session = (struct ftpc_session_s *)zalloc(sizeof(struct ftpc_session_s));
  if (!session)
    {
      nerr("ERROR: Failed to allocate a session\n");
      errno = ENOMEM;
      goto errout;
    }

  /* Initialize the session structure with all non-zero and variable values */

  memcpy(&session->server, server, sizeof(union ftpc_sockaddr_u));

  session->flags      &= ~FTPC_FLAGS_CLEAR;
  session->flags      |= FTPC_FLAGS_SET;
  session->replytimeo  = CONFIG_FTP_DEFTIMEO * CLOCKS_PER_SEC;
  session->conntimeo   = CONFIG_FTP_DEFTIMEO * CLOCKS_PER_SEC;
  session->pid         = getpid();

  session->cmd.sd       = -1;
  session->data.sd      = -1;
  session->dacceptor.sd = -1;

  /* Use the default port if the user specified port number zero */

#ifdef CONFIG_NET_IPv6
  if (session->server.sa.sa_family == AF_INET6)
    {
      if (!session->server.in6.sin6_port)
        {
          session->server.in6.sin6_port = HTONS(CONFIG_FTP_DEFPORT);
        }
    }
#endif

#ifdef CONFIG_NET_IPv4
  if (session->server.sa.sa_family == AF_INET)
    {
      if (!session->server.in4.sin_port)
        {
          session->server.in4.sin_port = HTONS(CONFIG_FTP_DEFPORT);
        }
    }
#endif

  /* Get the local home directory, i.e., the value of the PWD environment
   * variable at the time of the connection.  We keep a local copy so that
   * we can change the current working directory without effecting any other
   * logic that may be in same context.
   */

  session->homeldir = strdup(ftpc_lpwd());

  /* And (Re-)connect to the server */

  ret = ftpc_reconnect(session);
  if (ret != OK)
    {
      nerr("ERROR: ftpc_reconnect() failed: %d\n", errno);
      goto errout_with_alloc;
    }

  return (SESSION)session;

errout_with_alloc:
  if (session->homeldir != NULL)
    {
      free(session->homeldir);
    }

  free(session);
errout:
  return NULL;
}

/****************************************************************************
 * Name: ftpc_reconnect
 *
 * Description:
 *   re-connect to the server either initially, or after loss of connection.
 *
 ****************************************************************************/

int ftpc_reconnect(FAR struct ftpc_session_s *session)
{
#ifdef CONFIG_DEBUG_NET_ERROR
  char buffer[48];
#endif
  int ret;

  /* Re-initialize the session structure */

  session->replytimeo = CONFIG_FTP_DEFTIMEO * CLOCKS_PER_SEC;
  session->conntimeo  = CONFIG_FTP_DEFTIMEO * CLOCKS_PER_SEC;
  session->xfrmode    = FTPC_XFRMODE_UNKNOWN;

  /* Set up a timer to prevent hangs */

  ret = wd_start(&session->wdog, session->conntimeo,
                 ftpc_timeout, (wdparm_t)session);
  if (ret != OK)
    {
      nerr("ERROR: wd_start() failed\n");
      goto errout;
    }

  /* Initialize a socket */

  ret = ftpc_sockinit(&session->cmd, session->server.sa.sa_family);
  if (ret != OK)
    {
      nerr("ERROR: ftpc_sockinit() failed: %d\n", errno);
      goto errout;
    }

  /* Connect the socket to the server */

#ifdef CONFIG_DEBUG_NET_ERROR
#ifdef CONFIG_NET_IPv6
  if (session->server.sa.sa_family == AF_INET6)
    {
      if (inet_ntop(AF_INET6, &session->server.in6.sin6_addr, buffer, 48))
        {
          ninfo("Connecting to server address %s:%d\n", buffer,
                ntohs(session->server.in6.sin6_port));
        }
    }
#endif /* CONFIG_NET_IPv6 */

#ifdef CONFIG_NET_IPv4
  if (session->server.sa.sa_family == AF_INET)
    {
      if (inet_ntop(AF_INET, &session->server.in4.sin_addr, buffer, 48))
        {
          ninfo("Connecting to server address %s:%d\n", buffer,
                ntohs(session->server.in4.sin_port));
        }
    }
#endif /* CONFIG_NET_IPv4 */
#endif /* CONFIG_DEBUG_NET_ERROR */

  ret = ftpc_sockconnect(&session->cmd,
                         (FAR struct sockaddr *)&session->server);
  if (ret != OK)
    {
      nerr("ERROR: ftpc_sockconnect() failed: %d\n", errno);
      goto errout_with_socket;
    }

  /* Read startup message from server */

  fptc_getreply(session);

  /* Check for "120 Service ready in nnn minutes" */

  if (session->code == 120)
    {
      fptc_getreply(session);
    }

  wd_cancel(&session->wdog);

  if (!ftpc_sockconnected(&session->cmd))
    {
      ftpc_reset(session);
      goto errout;
    }

  /* Check for "220 Service ready for new user" */

  if (session->code == 220)
    {
      FTPC_SET_CONNECTED(session);
    }

  if (!FTPC_IS_CONNECTED(session))
    {
      goto errout_with_socket;
    }

#ifdef CONFIG_DEBUG_NET_ERROR
  ninfo("Connected\n");
#ifdef CONFIG_NET_IPv6
  if (session->server.sa.sa_family == AF_INET6)
    {
      if (inet_ntop(AF_INET6, &session->server.in6.sin6_addr, buffer, 48))
        {
          ninfo("  Remote address: %s:%d\n", buffer,
                ntohs(session->server.in6.sin6_port));
        }

      if (inet_ntop(AF_INET6, &session->cmd.laddr.in6.sin6_addr, buffer, 48))
        {
          ninfo("  Local address:  %s:%d\n", buffer,
                ntohs(session->cmd.laddr.in6.sin6_port));
        }
    }
#endif /* CONFIG_NET_IPv6 */

#ifdef CONFIG_NET_IPv4
  if (session->server.sa.sa_family == AF_INET)
    {
      if (inet_ntop(AF_INET, &session->server.in4.sin_addr, buffer, 48))
        {
          ninfo("  Remote address: %s:%d\n", buffer,
                ntohs(session->server.in4.sin_port));
        }

      if (inet_ntop(AF_INET, &session->cmd.laddr.in4.sin_addr, buffer, 48))
        {
          ninfo("  Local address:  %s:%d\n", buffer,
                ntohs(session->cmd.laddr.in4.sin_port));
        }
    }
#endif /* CONFIG_NET_IPv4 */
#endif /* CONFIG_DEBUG_NET_ERROR */

  return OK;

errout_with_socket:
  ftpc_sockclose(&session->cmd);
errout:
  return ERROR;
}
