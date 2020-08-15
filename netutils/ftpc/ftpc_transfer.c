/****************************************************************************
 * apps/netutils/ftpc/ftpc_transfer.c
 *
 *   Copyright (C) 2011 Gregory Nutt. All rights reserved.
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

#include <sys/stat.h>
#include <sys/time.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <poll.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <debug.h>
#include <arpa/inet.h>

#include "netutils/ftpc.h"
#include "netutils/netlib.h"

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
 * Name: ftp_cmd_epsv
 *
 * Description:
 *   Enter passive mode using EPSV command.
 *
 *   In active mode FTP the client connects from a random port (N>1023) to
 *   the FTP server's command port, port 21. Then the client starts listening
 *   to port N+1 and sends the FTP command PORT N+1 to the FTP server. The
 *   server will then connect back to the client's specified data port from
 *   its local data port, which is port 20. In passive mode FTP the client
 *   initiates both connections to the server, solving the problem of
 *   firewalls filtering the incoming data port connection to the client from
 *   the server. When opening an FTP connection, the client opens two random
 *   ports locally (N>1023 and N+1). The first port contacts the server on
 *   port 21, but instead of then issuing a PORT command and allowing the
 *   server to connect back to its data port, the client will issue the PASV
 *   command. The result of this is that the server then opens a random
 *   unprivileged port (P > 1023) and sends the PORT P command back to the
 *   client. The client then initiates the connection from port N+1 to port
 *   P on the server to transfer data.
 *
 ****************************************************************************/

#ifndef CONFIG_FTPC_DISABLE_EPSV
static int ftp_cmd_epsv(FAR struct ftpc_session_s *session,
                        FAR union ftpc_sockaddr_u *addr)
{
  char *ptr;
  int nscan;
  int ret;
  uint16_t tmp;

  /* Request passive mode.  The server normally accepts EPSV with code 227.
   * Its response is a single line showing the IP address of the server and
   * the TCP port number where the server is accepting connections.
   */

  ret = ftpc_cmd(session, "EPSV");
  if (ret < 0 || !ftpc_connected(session))
    {
      return ERROR;
    }

  /* Skip over any leading stuff before important data begins */

  ptr = session->reply + 4;
  while (*ptr != '(')
    {
      ptr++;

      if (ptr > (session->reply + sizeof(session->reply) - 1))
        {
          nwarn("WARNING: Error parsing EPSV reply: '%s'\n", session->reply);
          return ERROR;
        }
    }

  ptr++;

  /* The response is then just the port number. None of the other fields
   * are supplied.
   */

  nscan = sscanf(ptr, "|||%hu|", &tmp);
  if (nscan != 1)
    {
      nwarn("WARNING: Error parsing EPSV reply: '%s'\n", session->reply);
      return ERROR;
    }

#ifdef CONFIG_NET_IPv4
  if (addr->sa.sa_family == AF_INET)
    {
      addr->in4.sin_port = HTONS(tmp);
    }
#endif

#ifdef CONFIG_NET_IPv6
  if (addr->sa.sa_family == AF_INET6)
    {
      addr->in6.sin6_port = HTONS(tmp);
    }
#endif

  return OK;
}
#endif

/****************************************************************************
 * Name: ftp_cmd_pasv
 *
 * Description:
 *   Enter passive mode using PASV command.
 *
 *   In active mode FTP the client connects from a random port (N>1023) to
 *   the FTP server's command port, port 21. Then the client starts listening
 *   to port N+1 and sends the FTP command PORT N+1 to the FTP server. The
 *   server will then connect back to the client's specified data port from
 *   its local data port, which is port 20. In passive mode FTP the client
 *   initiates both connections to the server, solving the problem of
 *   firewalls filtering the incoming data port connection to the client from
 *   the server. When opening an FTP connection, the client opens two random
 *   ports locally (N>1023 and N+1). The first port contacts the server on
 *   port 21, but instead of then issuing a PORT command and allowing the
 *   server to connect back to its data port, the client will issue the PASV
 *   of this is that the server then opens a random unprivileged port (P >
 *   command. The result 1023) and sends the PORT P command back to the
 *   client. The client then initiates the connection from port N+1 to port P
 *   on the server to transfer data.
 *
 ****************************************************************************/

#ifdef CONFIG_FTPC_DISABLE_EPSV
static int ftp_cmd_pasv(FAR struct ftpc_session_s *session,
                        FAR union ftpc_sockaddr_u *addr)
{
  int tmpap[6];
  char *ptr;
  int nscan;
  int ret;

  /* Request passive mode.  The server normally accepts PASV with code 227.
   * Its response is a single line showing the IP address of the server and
   * the TCP port number where the server is accepting connections.
   */

  ret = ftpc_cmd(session, "PASV");
  if (ret < 0 || !ftpc_connected(session))
    {
      return ERROR;
    }

  /* Skip over any leading stuff before important data begins */

  ptr = session->reply + 4;
  while (*ptr != '\0' && !isdigit((int)*ptr))
    {
      ptr++;
    }

  /* The response is then 6 integer values:  four representing the
   * IP address and two representing the port number.
   */

  nscan = sscanf(ptr, "%d,%d,%d,%d,%d,%d",
                 &tmpap[0], &tmpap[1], &tmpap[2],
                 &tmpap[3], &tmpap[4], &tmpap[5]);
  if (nscan != 6)
    {
      nwarn("WARNING: Error parsing PASV reply: '%s'\n", session->reply);
      return ERROR;
    }

  /* Then copy the sscanf'ed values as bytes */

  memcpy(&addr->in4.sin_addr, tmpap, sizeof(addr->in4.sin_addr));
  memcpy(&addr->in4.sin_port, &tmpap[4], sizeof(addr->in4.sin_port));

  return OK;
}
#endif

/****************************************************************************
 * Name: ftpc_abspath
 *
 * Description:
 *   Get the absolute path to a file, handling tilde expansion.
 *
 ****************************************************************************/

static FAR char *ftpc_abspath(FAR struct ftpc_session_s *session,
                              FAR const char *relpath,
                              FAR const char *homedir,
                              FAR const char *curdir)
{
  FAR char *ptr = NULL;

  /* If no relative path was provide,
   * then use the current working directory
   */

  if (!relpath)
    {
      return strdup(curdir);
    }

  /* Handle tilde expansion */

  if (relpath[0] == '~')
    {
      /* Is the relative path only '~' */

      if (relpath[1] == '\0')
        {
          return strdup(homedir);
        }

      /* No... then a '/' better follow the tilde */

      else if (relpath[1] == '/')
        {
          asprintf(&ptr, "%s%s", homedir, &relpath[1]);
        }

      /* Hmmm... this pretty much guaranteed to fail */

      else
        {
          ptr = strdup(relpath);
        }
    }

  /* No tilde expansion.  Check for a path relative to the current
   * directory.
   */

  else if (strncmp(relpath, "./", 2) == 0)
    {
      asprintf(&ptr, "%s%s", curdir, relpath + 1);
    }

  /* Check for an absolute path */

  else if (relpath[0] == '/')
    {
      ptr = strdup(relpath);
    }

  /* Assume it a relative path */

  else
    {
      asprintf(&ptr, "%s/%s", curdir, relpath);
    }

  return ptr;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ftpc_xfrinit
 *
 * Description:
 *   Perform common transfer setup logic.
 *
 ****************************************************************************/

int ftpc_xfrinit(FAR struct ftpc_session_s *session)
{
  union ftpc_sockaddr_u addr;
  int ret;
#ifdef CONFIG_FTPC_DISABLE_EPRT
  uint8_t *paddr;
  uint8_t *pport;
#else
  char ipstr[48];
#endif

  /* We must be connected to initiate a transfer */

  if (!ftpc_connected(session))
    {
      nerr("ERROR: Not connected\n");
      goto errout;
    }

  /* Should we enter passive mode? */

  if (FTPC_IS_PASSIVE(session))
    {
      /* Initialize the data channel */

      ret = ftpc_sockinit(&session->data, session->server.sa.sa_family);
      if (ret != OK)
        {
          nerr("ERROR: ftpc_sockinit() failed: %d\n", errno);
          goto errout;
        }

      /* Does this host support the PASV command */

      if (!FTPC_HAS_PASV(session))
        {
          nerr("ERROR: Host doesn't support passive mode\n");
          goto errout_with_data;
        }

      /* Configure the address to be the server address. If EPSV is used, the
       * port will be populated by parsing the reply of the EPSV command. If
       * the PASV command is used, the address and port will be overwritten.
       */

      memcpy(&addr, &session->server, sizeof(union ftpc_sockaddr_u));

      /* Yes.. going passive. */

#ifdef CONFIG_FTPC_DISABLE_EPSV
      ret = ftp_cmd_pasv(session, &addr);
      if (ret < 0)
        {
          nerr("ERROR: ftp_cmd_pasv() failed: %d\n", errno);
          goto errout_with_data;
        }
#else
      ret = ftp_cmd_epsv(session, &addr);
      if (ret < 0)
        {
          nerr("ERROR: ftp_cmd_epsv() failed: %d\n", errno);
          goto errout_with_data;
        }
#endif

      /* Connect the data socket */

      ret = ftpc_sockconnect(&session->data, (FAR struct sockaddr *)&addr);
      if (ret < 0)
        {
          nerr("ERROR: ftpc_sockconnect() failed: %d\n", errno);
          goto errout_with_data;
        }
    }
  else
    {
      /* Initialize the data listener socket that allows us to accept new
       * data connections from the server
       */

      ret = ftpc_sockinit(&session->dacceptor, session->server.sa.sa_family);
      if (ret != OK)
        {
          nerr("ERROR: ftpc_sockinit() failed: %d\n", errno);
          goto errout;
        }

      /* Use the server IP address to find the network interface, and
       * subsequent local IP address used to establish the active
       * connection. We must send the IP and port to the server so that
       * it knows how to connect.
       */

#ifdef CONFIG_NET_IPv6
      if (session->server.sa.sa_family == AF_INET6)
        {
          ret = netlib_ipv6adaptor(&session->server.in6.sin6_addr,
                                   &session->dacceptor.laddr.in6.sin6_addr);
          if (ret < 0)
            {
              nerr("ERROR: netlib_ipv6adaptor() failed: %d\n", ret);
              goto errout_with_data;
            }
        }
      else
#endif
#ifdef CONFIG_NET_IPv4
      if (session->server.sa.sa_family == AF_INET)
        {
          ret = netlib_ipv4adaptor(session->server.in4.sin_addr.s_addr,
                  &session->dacceptor.laddr.in4.sin_addr.s_addr);
          if (ret < 0)
            {
              nerr("ERROR: netlib_ipv4adaptor() failed: %d\n", ret);
              goto errout_with_data;
            }
        }
      else
#endif
        {
          nerr("ERROR: unsupported address family\n");
          goto errout_with_data;
        }

      /* Wait for the connection to be established */

      ftpc_socklisten(&session->dacceptor);

#ifdef CONFIG_FTPC_DISABLE_EPRT
      /* Then send our local data channel address to the server */

      paddr = (uint8_t *)&session->dacceptor.laddr.in4.sin_addr;
      pport = (uint8_t *)&session->dacceptor.laddr.in4.sin_port;

      ret = ftpc_cmd(session, "PORT %d,%d,%d,%d,%d,%d",
                     paddr[0], paddr[1], paddr[2],
                     paddr[3], pport[0], pport[1]);
#else
#ifdef CONFIG_NET_IPv6
      if (session->dacceptor.laddr.sa.sa_family == AF_INET6)
        {
          if (!inet_ntop(AF_INET6, &session->dacceptor.laddr.in6.sin6_addr,
                         ipstr, 48))
            {
              nerr("ERROR: inet_ntop failed: %d\n", errno);
              goto errout_with_data;
            }

          ret = ftpc_cmd(session, "EPRT |2|%s|%d|", ipstr,
                         session->dacceptor.laddr.in6.sin6_port);
        }
      else
#endif /* CONFIG_NET_IPv6 */
#ifdef CONFIG_NET_IPv4
      if (session->dacceptor.laddr.sa.sa_family == AF_INET)
        {
          if (!inet_ntop(AF_INET, &session->dacceptor.laddr.in4.sin_addr,
                         ipstr, 48))
            {
              nerr("ERROR: inet_ntop failed: %d\n", errno);
              goto errout_with_data;
            }

          ret = ftpc_cmd(session, "EPRT |1|%s|%d|", ipstr,
                         session->dacceptor.laddr.in4.sin_port);
        }
      else
#endif /* CONFIG_NET_IPv4 */
#endif /* CONFIG_FTPC_DISABLE_EPRT */

      if (ret < 0)
        {
          nerr("ERROR: ftpc_cmd() failed: %d\n", errno);
          goto errout_with_data;
        }
    }

  return OK;

errout_with_data:
  ftpc_sockclose(&session->data);
  ftpc_sockclose(&session->dacceptor);
errout:
  return ERROR;
}

/****************************************************************************
 * Name: ftpc_xfrreset
 *
 * Description:
 *   Reset transfer variables
 *
 ****************************************************************************/

void ftpc_xfrreset(struct ftpc_session_s *session)
{
  session->size     = 0;
  session->flags   &= ~FTPC_XFER_FLAGS;
}

/****************************************************************************
 * Name: ftpc_xfrmode
 *
 * Description:
 *   Select ASCII or Binary transfer mode
 *
 ****************************************************************************/

int ftpc_xfrmode(struct ftpc_session_s *session, uint8_t xfrmode)
{
  int ret;

  /* Check if we have already selected the requested mode */

  DEBUGASSERT(xfrmode != FTPC_XFRMODE_UNKNOWN);
  if (session->xfrmode != xfrmode)
    {
      /* Send the TYPE request to control the binary flag.
       * Parameters for the TYPE request include:
       *
       *  A: Turn the binary flag off.
       *  A N: Turn the binary flag off.
       *  I: Turn the binary flag on.
       *  L 8: Turn the binary flag on.
       *
       *  The server accepts the TYPE request with code 200.
       */

      ret = ftpc_cmd(session, "TYPE %c",
                     xfrmode == FTPC_XFRMODE_ASCII ? 'A' : 'I');
      UNUSED(ret);
      session->xfrmode = xfrmode;
    }

  return OK;
}

/****************************************************************************
 * Name: ftpc_xfrabort
 *
 * Description:
 *   Abort a transfer in progress
 *
 ****************************************************************************/

int ftpc_xfrabort(FAR struct ftpc_session_s *session, FAR FILE *stream)
{
  FAR struct pollfd fds;
  int ret;

  /* Make sure that we are still connected */

  if (!ftpc_connected(session))
    {
      return ERROR;
    }

  /* Check if there is data waiting to be read from the cmd channel */

  fds.fd     = session->cmd.sd;
  fds.events = POLLIN;
  ret        = poll(&fds, 1, 0);
  if (ret > 0)
    {
      /* Read data from command channel */

      ninfo("Flush cmd channel data\n");
      while (stream &&
             fread(session->buffer, 1, CONFIG_FTP_BUFSIZE, stream) > 0);
      return OK;
    }

  FTPC_SET_INTERRUPT(session);

  /* Send the Telnet interrupt sequence to abort the transfer:
   * <IAC IP><IAC DM>ABORT<CR><LF>
   */

  ninfo("Telnet ABORt sequence\n");
  ftpc_sockprintf(&session->cmd, "%c%c", TELNET_IAC, TELNET_IP); /* Interrupt process */
  ftpc_sockprintf(&session->cmd, "%c%c", TELNET_IAC, TELNET_DM); /* Telnet synch signal */
  ftpc_sockprintf(&session->cmd, "ABOR\r\n");                    /* Abort */
  ftpc_sockflush(&session->cmd);

  /* Read remaining bytes from connection */

  while (stream &&
         fread(session->buffer, 1, CONFIG_FTP_BUFSIZE, stream) > 0);

  /* Get the ABORt reply */

  fptc_getreply(session);

  /* Expected replies are: "226 Closing data connection" or
   * "426 Connection closed; transfer aborted"
   */

  if (session->code != 226 && session->code != 426)
    {
      ninfo("Expected 226 or 426 reply\n");
    }
  else
    {
      /* Get the next reply */

      fptc_getreply(session);

      /* Expected replies are:  "226 Closing data connection" or
       * "225 Data connection open; no transfer in progress"
       */

      if (session->code != 226 && session->code != 225)
        {
          ninfo("Expected 225 or 226 reply\n");
        }
    }

  return ERROR;
}

/****************************************************************************
 * Name: ftpc_timeout
 *
 * Description:
 *   A timeout occurred -- either on a specific command or while waiting
 *   for a reply.
 *
 * NOTE:
 *   This function executes in the context of a timer interrupt handler.
 *
 ****************************************************************************/

void ftpc_timeout(wdparm_t arg)
{
  FAR struct ftpc_session_s *session = (FAR struct ftpc_session_s *)arg;

  nerr("ERROR: Timeout!\n");
  DEBUGASSERT(session);
  kill(session->pid, CONFIG_FTP_SIGNAL);
}

/****************************************************************************
 * Name: ftpc_absrpath
 *
 * Description:
 *   Get the absolute path to a remote file, handling tilde expansion.
 *
 ****************************************************************************/

FAR char *ftpc_absrpath(FAR struct ftpc_session_s *session,
                        FAR const char *relpath)
{
  FAR char *absrpath = ftpc_abspath(session, relpath,
                                    session->homerdir, session->currdir);
  ninfo("%s -> %s\n", relpath, absrpath);
  return absrpath;
}

/****************************************************************************
 * Name: ftpc_abslpath
 *
 * Description:
 *   Get the absolute path to a local file, handling tilde expansion.
 *
 ****************************************************************************/

FAR char *ftpc_abslpath(FAR struct ftpc_session_s *session,
                        FAR const char *relpath)
{
  FAR char *abslpath = ftpc_abspath(session, relpath,
                                    session->homeldir, session->curldir);
  ninfo("%s -> %s\n", relpath, abslpath);
  return abslpath;
}
