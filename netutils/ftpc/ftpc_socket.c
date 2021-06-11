/****************************************************************************
 * apps/netutils/ftpc/ftpc_socket.c
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
#include <sys/socket.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <debug.h>
#include <errno.h>

#include "ftpc_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ftpc_sockinit
 *
 * Description:
 *   Initialize a socket.  Create the socket and "wrap" it as C standard
 *   incoming and outgoing streams.
 *
 ****************************************************************************/

int ftpc_sockinit(FAR struct ftpc_socket_s *sock, sa_family_t family)
{
  /* Initialize the socket structure */

  memset(sock, 0, sizeof(struct ftpc_socket_s));

  DEBUGASSERT(family == AF_INET || family == AF_INET6);

  sock->laddr.sa.sa_family = family;

  /* Create a socket descriptor */

  sock->sd = socket(family, SOCK_STREAM, IPPROTO_TCP);
  if (sock->sd < 0)
    {
      nerr("ERROR: socket() failed: %d\n", errno);
      goto errout;
    }

  /* Call fdopen to "wrap" the socket descriptor as an input stream using C
   * buffered I/O.
   */

  sock->instream = fdopen(sock->sd, "r");
  if (!sock->instream)
    {
      nerr("ERROR: fdopen() failed: %d\n", errno);
      goto errout_with_sd;
    }

  /* Call fdopen to "wrap" the socket descriptor as an output stream using C
   * buffered I/O.
   */

  sock->outstream = fdopen(sock->sd, "w");
  if (!sock->outstream)
    {
      nerr("ERROR: fdopen() failed: %d\n", errno);
      goto errout_with_instream;
    }

  return OK;

/* Close the instream.  NOTE:  Since the underlying socket descriptor is
 * *not* dup'ed, the following close should fail harmlessly.
 */

errout_with_instream:
  fclose(sock->instream);
  sock->instream = NULL;
errout_with_sd:
  close(sock->sd);
  sock->sd = -1;
errout:
  return ERROR;
}

/****************************************************************************
 * Name: ftpc_sockclose
 *
 * Description:
 *   Close a socket
 *
 ****************************************************************************/

void ftpc_sockclose(FAR struct ftpc_socket_s *sock)
{
  /* Note that the same underlying socket descriptor is used for both
   * streams. There should be harmless failures on the second fclose
   * and the close.
   */

  fclose(sock->instream);
  fclose(sock->outstream);
  close(sock->sd);
  memset(sock, 0, sizeof(struct ftpc_socket_s));
  sock->sd = -1;
}

/****************************************************************************
 * Name: ftpc_sockconnect
 *
 * Description:
 *   Connect the socket to the host.  On a failure, the caller should call.
 *   ftpc_sockclose() to clean up.
 *
 ****************************************************************************/

int ftpc_sockconnect(FAR struct ftpc_socket_s *sock,
                     FAR struct sockaddr *addr)
{
  int ret;

  /* Connect to the server */

#ifdef CONFIG_NET_IPv6
  if (addr->sa_family == AF_INET6)
    {
      ret = connect(sock->sd, (struct sockaddr *)addr,
                    sizeof(struct sockaddr_in6));
    }
  else
#endif
#ifdef CONFIG_NET_IPv4
  if (addr->sa_family == AF_INET)
    {
      ret = connect(sock->sd, (struct sockaddr *)addr,
                    sizeof(struct sockaddr_in));
    }
  else
#endif
    {
      nerr("ERROR: Unsupported address family\n");
      return ERROR;
    }

  if (ret < 0)
    {
      nerr("ERROR: connect() failed: %d\n", errno);
      return ERROR;
    }

  /* Get the local address of the socket */

  ret = ftpc_sockgetsockname(sock, &sock->laddr);
  if (ret < 0)
    {
      nerr("ERROR: ftpc_sockgetsockname() failed: %d\n", errno);
      return ERROR;
    }

  sock->connected = true;
  return OK;
}

/****************************************************************************
 * Name: ftpc_sockcopy
 *
 * Description:
 *   Copy the socket state from one location to another.
 *
 ****************************************************************************/

void ftpc_sockcopy(FAR struct ftpc_socket_s *dest,
                   FAR const struct ftpc_socket_s *src)
{
  memcpy(&dest->laddr, &src->laddr, sizeof(dest->laddr));
  dest->connected = ftpc_sockconnected(src);
}

/****************************************************************************
 * Name: ftpc_sockaccept
 *
 * Description:
 *   Accept a connection on the data socket.  This function is only used
 *   in active mode.
 *
 *   In active mode FTP the client connects from a random port (N>1023) to
 *   the FTP server's command port, port 21. Then, the client starts
 *   listening to port N+1 and sends the FTP command PORT N+1 to the FTP
 *   server. The server will then connect back to the client's specified data
 *   port from its local data port, which is port 20. In passive mode FTP the
 *   client initiates both connections to the server, solving the problem of
 *   firewalls filtering the incoming data port connection to the client from
 *   the server. When opening an FTP connection, the client opens two random
 *   ports locally (N>1023 and N+1). The first port contacts the server on
 *   port 21, but instead of then issuing a PORT command and allowing the
 *   server to connect back to its data port, the client will issue the PASV
 *   command. The result of this is that the server then opens a random
 *   unprivileged port (P > 1023) and sends the PORT P command back to the
 *   client. The client then initiates the connection from port N+1 to port P
 *   on the server to transfer data.
 *
 ****************************************************************************/

int ftpc_sockaccept(FAR struct ftpc_socket_s *acceptor,
                    FAR struct ftpc_socket_s *sock)
{
  union ftpc_sockaddr_u addr;
  socklen_t addrlen;

  /* Any previous socket should have been uninitialized (0) or explicitly
   * closed (-1).  But the path to this function may include a call to
   * ftpc_sockinit().  If so... close that socket and call accept to
   * get a new one.
   */

  if (sock->sd > 0)
    {
      ftpc_sockclose(sock);
    }

  addrlen  = sizeof(addr);
  sock->sd = accept(acceptor->sd, (struct sockaddr *)&addr, &addrlen);
  if (sock->sd == -1)
    {
      nerr("ERROR: accept() failed: %d\n", errno);
      return ERROR;
    }

  memcpy(&sock->laddr, &addr, sizeof(union ftpc_sockaddr_u));

  /* Create in/out C buffer I/O streams on the data channel.  First,
   * create the incoming buffered stream.
   */

  sock->instream = fdopen(sock->sd, "r");
  if (!sock->instream)
    {
      nerr("ERROR: fdopen() failed: %d\n", errno);
      goto errout_with_sd;
    }

  /* Create the outgoing stream */

  sock->outstream = fdopen(sock->sd, "w");
  if (!sock->outstream)
    {
      nerr("ERROR: fdopen() failed: %d\n", errno);
      goto errout_with_instream;
    }

  return OK;

/* Close the instream.  NOTE:  Since the underlying socket descriptor is
 * *not* dup'ed, the following close should fail harmlessly.
 */

errout_with_instream:
  fclose(sock->instream);
  sock->instream = NULL;
errout_with_sd:
  close(sock->sd);
  sock->sd = -1;
  return ERROR;
}

/****************************************************************************
 * Name: ftpc_socklisten
 *
 * Description:
 *   Bind the socket to local address and wait for connection from server.
 *
 ****************************************************************************/

int ftpc_socklisten(FAR struct ftpc_socket_s *sock)
{
  int ret;

  /* Bind the local socket to the local address */

#ifdef CONFIG_NET_IPv6
  if (sock->laddr.sa.sa_family == AF_INET6)
    {
      sock->laddr.in6.sin6_port = 0;
      ret = bind(sock->sd, (struct sockaddr *)&sock->laddr,
                 sizeof(struct sockaddr_in6));
    }
  else
#endif
#ifdef CONFIG_NET_IPv4
  if (sock->laddr.sa.sa_family == AF_INET)
    {
      sock->laddr.in4.sin_port = 0;
      ret = bind(sock->sd, (struct sockaddr *)&sock->laddr,
                 sizeof(struct sockaddr_in));
    }
  else
#endif
    {
      nerr("ERROR: unsupported family\n");
      return ERROR;
    }

  if (ret < 0)
    {
      nerr("ERROR: bind() failed: %d\n", errno);
      return ERROR;
    }

  /* Wait for the connection to the server */

  if (listen(sock->sd, 1) == -1)
    {
      return ERROR;
    }

  /* Then get the local address selected by NuttX */

  ret = ftpc_sockgetsockname(sock, &sock->laddr);
  return ret;
}

/****************************************************************************
 * Name: ftpc_sockprintf
 *
 * Description:
 *   printf to a socket stream
 *
 ****************************************************************************/

int ftpc_sockprintf(FAR struct ftpc_socket_s *sock, FAR const char *fmt, ...)
{
  va_list ap;
  int r;

  va_start(ap, fmt);
  r = vfprintf(sock->outstream, fmt, ap);
  va_end(ap);
  return r;
}

/****************************************************************************
 * Name: ftpc_sockgetsockname
 *
 * Description:
 *   Get the address of the local socket
 *
 ****************************************************************************/

int ftpc_sockgetsockname(FAR struct ftpc_socket_s *sock,
                         FAR union ftpc_sockaddr_u *addr)
{
  socklen_t len;
  int ret;

  len = sizeof(union ftpc_sockaddr_u);

  ret = getsockname(sock->sd, (FAR struct sockaddr *)addr, &len);
  if (ret < 0)
    {
      nerr("ERROR: getsockname failed: %d\n", errno);
      return ERROR;
    }

  return OK;
}
