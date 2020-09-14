/****************************************************************************
 * netutils/ntpclient/ntpclient.c
 *
 *   Copyright (C) 2014, 2016, 2020 Gregory Nutt. All rights reserved.
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

#include <sys/socket.h>
#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sched.h>
#include <errno.h>
#include <debug.h>

#include <netinet/in.h>

#ifdef CONFIG_LIBC_NETDB
#  include <netdb.h>
#  include <arpa/inet.h>
#endif

#include "netutils/ntpclient.h"

#include "ntpv3.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

#if defined(CONFIG_LIBC_NETDB) && !defined(CONFIG_NETUTILS_NTPCLIENT_SERVER)
#  error CONFIG_NETUTILS_NTPCLIENT_SERVER my be provided
#endif

#if !defined(CONFIG_LIBC_NETDB) && !defined(CONFIG_NETUTILS_NTPCLIENT_SERVERIP)
#  error CONFIG_NETUTILS_NTPCLIENT_SERVERIP my be provided
#endif

/* NTP Time is seconds since 1900. Convert to Unix time which is seconds
 * since 1970
 */

#define NTP2UNIX_TRANLSLATION 2208988800u
#define NTP_VERSION          3

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* This enumeration describes the state of the NTP daemon */

enum ntpc_daemon_e
{
  NTP_NOT_RUNNING = 0,
  NTP_STARTED,
  NTP_RUNNING,
  NTP_STOP_REQUESTED,
  NTP_STOPPED
};

/* This type describes the state of the NTP client daemon.  Only one
 * instance of the NTP daemon is permitted in this implementation.
 */

struct ntpc_daemon_s
{
  uint8_t state; /* See enum ntpc_daemon_e */
  sem_t lock;    /* Used to protect the whole structure */
  sem_t sync;    /* Used to synchronize start and stop events */
  pid_t pid;     /* Task ID of the NTP daemon */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* This type describes the state of the NTP client daemon.  Only one
 * instance of the NTP daemon is permitted in this implementation.  This
 * limitation is due only to this global data structure.
 */

static struct ntpc_daemon_s g_ntpc_daemon =
{
  NTP_NOT_RUNNING,
  SEM_INITIALIZER(1),
  SEM_INITIALIZER(0),
  -1
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ntpc_getuint32
 *
 * Description:
 *   Return the big-endian, 4-byte value in network (big-endian) order.
 *
 ****************************************************************************/

static inline uint32_t ntpc_getuint32(FAR uint8_t *ptr)
{
  /* Network order is big-endian; host order is irrelevant */

  return (uint32_t)ptr[3] |          /* MS byte appears first in data stream */
         ((uint32_t)ptr[2] << 8) |
         ((uint32_t)ptr[1] << 16) |
         ((uint32_t)ptr[0] << 24);
}

/****************************************************************************
 * Name: ntpc_settime
 *
 * Description:
 *   Given the NTP time in seconds, set the system time
 *
 ****************************************************************************/

static void ntpc_settime(FAR uint8_t *timestamp)
{
  struct timespec tp;
  time_t seconds;
  uint32_t frac;
  uint32_t nsec;
#ifdef CONFIG_HAVE_LONG_LONG
  uint64_t tmp;
#else
  uint32_t a16;
  uint32_t b0;
  uint32_t t32;
  uint32_t t16;
  uint32_t t0;
#endif

  /* NTP timestamps are represented as a 64-bit fixed-point number, in
   * seconds relative to 0000 UT on 1 January 1900.  The integer part is
   * in the first 32 bits and the fraction part in the last 32 bits, as
   * shown in the following diagram.
   *
   *    0                   1                   2                   3
   *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   *   |                         Integer Part                          |
   *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   *   |                         Fraction Part                         |
   *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */

  seconds = ntpc_getuint32(timestamp);

  /* Translate seconds to account for the difference in the origin time */

  if (seconds > NTP2UNIX_TRANLSLATION)
    {
      seconds -= NTP2UNIX_TRANLSLATION;
    }

  /* Conversion of the fractional part to nanoseconds:
   *
   *  NSec = (f * 1,000,000,000) / 4,294,967,296
   *       = (f * (5**9 * 2**9) / (2**32)
   *       = (f * 5**9) / (2**23)
   *       = (f * 1,953,125) / 8,388,608
   */

  frac = ntpc_getuint32(timestamp + 4);
#ifdef CONFIG_HAVE_LONG_LONG
  /* if we have 64-bit long long values, then the computation is easy */

  tmp  = ((uint64_t)frac * 1953125) >> 23;
  nsec = (uint32_t)tmp;

#else
  /* If we don't have 64 bit integer types, then the calculation is a little
   * more complex:
   *
   * Let f         = a    << 16 + b
   *     1,953,125 = 0x1d << 16 + 0xcd65
   * NSec << 23 =  ((a << 16) + b) * ((0x1d << 16) + 0xcd65)
   *            = (a << 16) * 0x1d << 16) +
   *              (a << 16) * 0xcd65 +
   *              b         * 0x1d << 16) +
   *              b         * 0xcd65;
   */

  /* Break the fractional part up into two values */

  a16  = frac >> 16;
  b0   = frac & 0xffff;

  /* Get the b32 and b0 terms
   *
   * t32 = (a << 16) * 0x1d << 16)
   * t0  = b * 0xcd65
   */

  t32  = 0x001d * a16;
  t0   = 0xcd65 * b0;

  /* Get the first b16 term
   *
   * (a << 16) * 0xcd65
   */

  t16  = 0xcd65 * a16;

  /* Add the upper 16-bits to the b32 accumulator */

  t32 += (t16 >> 16);

  /* Add the lower 16-bits to the b0 accumulator, handling carry to the b32
   * accumulator
   */

  t16  <<= 16;
  if (t0 > (0xffffffff - t16))
    {
      t32++;
    }

  t0 += t16;

  /* Get the second b16 term
   *
   * b * (0x1d << 16)
   */

  t16  = 0x001d * b0;

  /* Add the upper 16-bits to the b32 accumulator */

  t32 += (t16 >> 16);

  /* Add the lower 16-bits to the b0 accumulator, handling carry to the b32
   * accumulator
   */

  t16  <<= 16;
  if (t0 > (0xffffffff - t16))
    {
      t32++;
    }

  t0 += t16;

  /* t32 and t0 represent the 64 bit product.  Now shift right by 23 bits to
   * accomplish the divide by by 2**23.
   */

  nsec = (t32 << (32 - 23)) + (t0 >> 23);
#endif

  /* Set the system time */

  tp.tv_sec  = seconds;
  tp.tv_nsec = nsec;
  clock_settime(CLOCK_REALTIME, &tp);

  sinfo("Set time to %lu seconds: %d\n", (unsigned long)tp.tv_sec, ret);
}

/****************************************************************************
 * Name: ntpc_daemon
 *
 * Description:
 *   This the NTP client daemon.  This is a *very* minimal
 *   implementation! An NTP request is and the system clock is set when the
 *   response is received
 *
 ****************************************************************************/

static int ntpc_daemon(int argc, char **argv)
{
  struct sockaddr_in server;
  struct ntp_datagram_s pkt;
  struct timeval tv;

#ifdef CONFIG_LIBC_NETDB
  struct hostent *he;
  struct in_addr **addr_list;
#endif

  socklen_t socklen;
  ssize_t nbytes;
  int exitcode = EXIT_SUCCESS;
  int retry = 0;
  int sd;
  int ret;

  /* Indicate that we have started */

  g_ntpc_daemon.state = NTP_RUNNING;
  sem_post(&g_ntpc_daemon.sync);

  /* Create a datagram socket  */

  sd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sd < 0)
    {
      nerr("ERROR: socket failed: %d\n", errno);

      g_ntpc_daemon.state = NTP_STOPPED;
      sem_post(&g_ntpc_daemon.sync);
      return EXIT_FAILURE;
    }

  /* Setup a receive timeout on the socket */

  tv.tv_sec  = 5;
  tv.tv_usec = 0;

  ret = setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval));
  if (ret < 0)
    {
      nerr("ERROR: setsockopt failed: %d\n", errno);

      g_ntpc_daemon.state = NTP_STOPPED;
      sem_post(&g_ntpc_daemon.sync);
      return EXIT_FAILURE;
    }

  /* Setup or sockaddr_in struct with information about the server we are
   * going to ask the time from.
   */

  memset(&server, 0, sizeof(struct sockaddr_in));
  server.sin_family      = AF_INET;
  server.sin_port        = htons(CONFIG_NETUTILS_NTPCLIENT_PORTNO);

#ifndef CONFIG_LIBC_NETDB
  server.sin_addr.s_addr = htonl(CONFIG_NETUTILS_NTPCLIENT_SERVERIP);
#else
  he = gethostbyname(CONFIG_NETUTILS_NTPCLIENT_SERVER);
  if (he != NULL && he->h_addrtype == AF_INET)
    {
      addr_list = (struct in_addr **)he->h_addr_list;
      server.sin_addr.s_addr = addr_list[0]->s_addr;
      ninfo("INFO: '%s' resolved to: %s\n",
            CONFIG_NETUTILS_NTPCLIENT_SERVER,
            inet_ntoa(server.sin_addr));
    }
  else
    {
      nerr("ERROR: Failed to resolve '%s'\n",
           CONFIG_NETUTILS_NTPCLIENT_SERVER);

      g_ntpc_daemon.state = NTP_STOPPED;
      sem_post(&g_ntpc_daemon.sync);
      return EXIT_FAILURE;
    }
#endif

  /* Here we do the communication with the NTP server.  This is a very simple
   * client architecture.  A request is sent and then a NTP packet is
   * received and used to set the current time.
   *
   * NOTE that the scheduler is locked whenever this loop runs.  That
   * assures both:  (1) that there are no asynchronous stop requests and
   * (2) that we are not suspended while in critical moments when we about
   * to set the new time.  This sounds harsh, but this function is suspended
   * most of the time either: (1) sending a datagram, (2) receiving a
   * datagram, or (3) waiting for the next poll cycle.
   *
   * TODO: The first datagram that is sent is usually lost.  That is because
   * the MAC address of the NTP server is not in the ARP table.  This is
   * particularly bad here because the request will not be sent again until
   * the long delay expires leaving the system with bad time for a long time
   * initially.  Solutions:
   *
   * 1. Fix send logic so that it assures that the ARP request has been
   *    sent and the entry is in the ARP table before sending the packet
   *    (best).
   * 2. Add some ad hoc logic here so that there is no delay until at least
   *    one good time is received.
   */

  sched_lock();
  while (g_ntpc_daemon.state != NTP_STOP_REQUESTED)
    {
      /* Format the transmit datagram */

      memset(&pkt, 0, sizeof(pkt));
      pkt.lvm = MKLVM(0, NTP_VERSION, 3);

      sinfo("Sending a NTP packet\n");

      ret = sendto(sd, &pkt, sizeof(struct ntp_datagram_s),
                   0, (FAR struct sockaddr *)&server,
                   sizeof(struct sockaddr_in));

      if (ret < 0)
        {
          /* Check if we received a signal.  That is not an error but
           * other error events will terminate the client.
           */

          int errval = errno;
          if (errval != EINTR)
            {
              nerr("ERROR: sendto() failed: %d\n", errval);
              exitcode = EXIT_FAILURE;
              break;
            }

          /* Go back to the top of the loop if we were interrupted
           * by a signal.  The signal might mean that we were
           * requested to stop(?)
           */

          continue;
        }

      /* Attempt to receive a packet (with a timeout that was set up via
       * setsockopt() above)
       */

      socklen = sizeof(struct sockaddr_in);
      nbytes = recvfrom(sd, (void *)&pkt, sizeof(struct ntp_datagram_s),
                        0, (FAR struct sockaddr *)&server, &socklen);

      /* Check if the received message was long enough to be a valid NTP
       * datagram.
       */

      if (nbytes >= (ssize_t)NTP_DATAGRAM_MINSIZE)
        {
          sinfo("Setting time\n");
          ntpc_settime(pkt.recvtimestamp);
          retry = 0;
        }

      /* Check for errors.  Note that properly received, short datagrams
       * are simply ignored.
       */

      else if (nbytes < 0)
        {
          /* Check if we received a signal.  That is not an error but
           * other error events will terminate the client.
           */

          int errval = errno;
          if (errval != EINTR)
            {
              /* Allow up to three retries */

              if (++retry < 3)
                {
                  continue;
                }

              /* Then declare the failure */

              nerr("ERROR: recvfrom() failed: %d\n", errval);
              exitcode = EXIT_FAILURE;
              break;
            }
        }

      /* A full implementation of an NTP client would require much more.  I
       * think we can skip most of that here.
       */

      if (g_ntpc_daemon.state == NTP_RUNNING)
        {
          sinfo("Waiting for %d seconds\n",
                CONFIG_NETUTILS_NTPCLIENT_POLLDELAYSEC);

          sleep(CONFIG_NETUTILS_NTPCLIENT_POLLDELAYSEC);
        }
    }

  /* The NTP client is terminating */

  sched_unlock();

  g_ntpc_daemon.state = NTP_STOPPED;
  sem_post(&g_ntpc_daemon.sync);
  return exitcode;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ntpc_start
 *
 * Description:
 *   Start the NTP daemon
 *
 * Returned Value:
 *   On success, the non-negative task ID of the NTPC daemon is returned;
 *   On failure, a negated errno value is returned.
 *
 ****************************************************************************/

int ntpc_start(void)
{
  /* Is the NTP in a non-running state? */

  sem_wait(&g_ntpc_daemon.lock);
  if (g_ntpc_daemon.state == NTP_NOT_RUNNING ||
      g_ntpc_daemon.state == NTP_STOPPED)
    {
      /* Start the NTP daemon */

      g_ntpc_daemon.state = NTP_STARTED;
      g_ntpc_daemon.pid =
        task_create("NTP daemon", CONFIG_NETUTILS_NTPCLIENT_SERVERPRIO,
                    CONFIG_NETUTILS_NTPCLIENT_STACKSIZE, ntpc_daemon,
                    NULL);

      /* Handle failures to start the NTP daemon */

      if (g_ntpc_daemon.pid < 0)
        {
          int errval = errno;
          DEBUGASSERT(errval > 0);

          g_ntpc_daemon.state = NTP_STOPPED;
          nerr("ERROR: Failed to start the NTP daemon\n", errval);
          sem_post(&g_ntpc_daemon.lock);
          return -errval;
        }

      /* Wait for any daemon state change */

      do
        {
          sem_wait(&g_ntpc_daemon.sync);
        }
      while (g_ntpc_daemon.state == NTP_STARTED);
    }

  sem_post(&g_ntpc_daemon.lock);
  return g_ntpc_daemon.pid;
}

/****************************************************************************
 * Name: ntpc_stop
 *
 * Description:
 *   Stop the NTP daemon
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.  The current
 *   implementation only returns success.
 *
 ****************************************************************************/

int ntpc_stop(void)
{
  int ret;

  /* Is the NTP in a running state? */

  sem_wait(&g_ntpc_daemon.lock);
  if (g_ntpc_daemon.state == NTP_STARTED ||
      g_ntpc_daemon.state == NTP_RUNNING)
    {
      /* Yes.. request that the daemon stop. */

      g_ntpc_daemon.state = NTP_STOP_REQUESTED;

      /* Wait for any daemon state change */

      do
        {
          /* Signal the NTP client */

          ret = kill(g_ntpc_daemon.pid,
                     CONFIG_NETUTILS_NTPCLIENT_SIGWAKEUP);

          if (ret < 0)
            {
              nerr("ERROR: kill pid %d failed: %d\n",
                   g_ntpc_daemon.pid, errno);
              break;
            }

          /* Wait for the NTP client to respond to the stop request */

          sem_wait(&g_ntpc_daemon.sync);
        }
      while (g_ntpc_daemon.state == NTP_STOP_REQUESTED);
    }

  sem_post(&g_ntpc_daemon.lock);
  return OK;
}
