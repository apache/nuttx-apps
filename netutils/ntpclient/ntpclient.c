/****************************************************************************
 * netutils/ntpclient/ntpclient.c
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sched.h>
#include <errno.h>
#include <debug.h>

#include <apps/netutils/ntpclient.h>

#include "ntpv3.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* NTP Time is seconds since 1900. Convert to Unix time which is seconds
 * since 1970
 */

#define NTP2UNIX_TRANLSLATION 2208988800u
#define NTP_VERSION          3

/****************************************************************************
 * Private Types
 ****************************************************************************/
/* This enumeration describes the state of the NTP daemon */

enum ntpclient_daemon_e
{
  NTP_NOT_RUNNING = 0,
  NTP_STARTED,
  NTP_RUNNING,
  NTP_STOP_REQUESTED,
  NTP_STOPPED
};

/* This type describes the state of the NTP client daemon.  Only once
 * instance of the NTP daemon is permitted in this implementation.
 */

struct ntpclient_daemon_s
{
  volatile uint8_t state; /* See enum ntpclient_daemon_e */
  sem_t interlock;        /* Used to synchronize start and stop events */
  pid_t pid;              /* Task ID of the NTP daemon */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* This type describes the state of the NTP client daemon.  Only once
 * instance of the NTP daemon is permitted in this implementation.  This
 * limitation is due only to this global data structure.
 */

static struct ntpclient_daemon_s g_ntpclient_daemon;

/****************************************************************************
 * Private Functions
 ****************************************************************************/
/****************************************************************************
 * Name: ntpclient_getuint32
 *
 * Description:
 *   Return the big-endian, 4-byte value in network (big-endian) order.
 *
 ****************************************************************************/

static inline uint32_t ntpclient_getuint32(FAR uint8_t *ptr)
{
  /* Network order is big-endian; host order is irrelevant */

  return (uint32_t)ptr[0] |          /* LS byte appears first in data stream */
         ((uint32_t)ptr[1] << 8) |
         ((uint32_t)ptr[2] << 16) |
         ((uint32_t)ptr[3] << 24);
}

/****************************************************************************
 * Name: ntpclient_settime
 *
 * Description:
 *   Given the NTP time in seconds, set the system time
 *
 ****************************************************************************/

static void ntpclient_settime(FAR uint8_t *timestamp)
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

  seconds = ntpclient_getuint32(timestamp);

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

  frac = ntpclient_getuint32(timestamp + 4);
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
  t0   = 0xcd65 * b0

  /* Get the first b16 term
   *
   * (a << 16) * 0xcd65
   */

  t16  = 0xcd65 * a16

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
  else
    {
      t0 += t16;
    }

  /* Get the second b16 term
   *
   * b * 0x1d << 16)
   */

  t16  = 0x001d * b

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
  else
    {
      t0 += t16;
    }

  /* t32 and t0 represent the 64 bit product.  Now shift right by 23 bits to
   * accomplish the divide by by 2**23.
   */

  nsec = (t32 << (32 - 23)) + (t0 >> 23)
#endif

  /* Set the system time */

  tp.tv_sec  = seconds;
  tp.tv_nsec = nsec;
  clock_settime(CLOCK_REALTIME, &tp);

  svdbg("Set time to %ld seconds: %d\n", tp.tv_sec, ret);
}

/****************************************************************************
 * Name: ntpclient_daemon
 *
 * Description:
 *   This the the NTP client daemon.  This is a *very* minimal
 *   implementation! An NTP request is and the system clock is set when the
 *   response is received
 *
 ****************************************************************************/

static int ntpclient_daemon(int argc, char **argv)
{
  struct sockaddr_in server;
  struct ntp_datagram_s xmit;
  struct ntp_datagram_s recv;
  struct timeval tv;
  socklen_t socklen;
  ssize_t nbytes;
  int exitcode = EXIT_SUCCESS;
  int ret;
  int sd;

  /* Indicate that we have started */

  g_ntpclient_daemon.state = NTP_RUNNING;
  sem_post(&g_ntpclient_daemon.interlock);

  /* Create a datagram socket  */

  sd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sd < 0)
    {
      ndbg("ERROR: socket failed: %d\n", errno);

      g_ntpclient_daemon.state = NTP_STOPPED;
      sem_post(&g_ntpclient_daemon.interlock);
      return EXIT_FAILURE;
    }

  /* Setup a receive timeout on the socket */

  tv.tv_sec  = 5;
  tv.tv_usec = 0;

  ret = setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval));
  if (ret < 0)
    {
      ndbg("ERROR: setsockopt failed: %d\n", errno);

      g_ntpclient_daemon.state = NTP_STOPPED;
      sem_post(&g_ntpclient_daemon.interlock);
      return EXIT_FAILURE;
    }

  /* Setup or sockaddr_in struct with information about the server we are
   * going to ask the the time from.
   */

  memset(&server, 0, sizeof(struct sockaddr_in));
  server.sin_family      = AF_INET;
  server.sin_port        = htons(CONFIG_NETUTILS_NTPCLIENT_PORTNO);
  server.sin_addr.s_addr = htonl(CONFIG_NETUTILS_NTPCLIENT_SERVERIP);

  /* Here we do the communication with the NTP server.  This is a very simple
   * client architecture.  A request is sent and then a NTP packet is received.
   * The NTP packet received is decoded to the recv structure for easy
   * access.
   */

  while (g_ntpclient_daemon.state == NTP_STOP_REQUESTED)
    {
      memset(&xmit, 0, sizeof(xmit));
      xmit.lvm = MKLVM(0, 3, NTP_VERSION);

      svdbg("Sending a NTP packet\n");

      ret = sendto(sd, &xmit, sizeof(struct ntp_datagram_s),
                   0, (FAR struct sockaddr *)&server,
                   sizeof(struct sockaddr_in));
      if (ret < 0)
        {
          ndbg("ERROR: sendto() failed: %d\n", errno);
          exitcode = EXIT_FAILURE;
          break;
        }

      /* Attempt to receive a packet (with a timeout) */

      socklen = sizeof(struct sockaddr_in);
      nbytes = recvfrom(sd, (void *)&recv, sizeof(struct ntp_datagram_s),
                        0, (FAR struct sockaddr *)&server, &socklen);
      if (nbytes >= NTP_DATAGRAM_MINSIZE)
        {
          svdbg("Setting time\n");
          ntpclient_settime(recv.recvtimestamp);
        }

      /* A full implementation of an NTP client would requireq much more.  I
       * think we we can skip that here.
       */

      svdbg("Waiting for %d seconds\n", CONFIG_NETUTILS_NTPCLIENT_POLLDELAYSEC);
      (void)sleep(CONFIG_NETUTILS_NTPCLIENT_POLLDELAYSEC);
    }

  /* The NTP client is terminating */

  g_ntpclient_daemon.state = NTP_STOPPED;
  sem_post(&g_ntpclient_daemon.interlock);
  return exitcode;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/****************************************************************************
 * Name: ntpclient_start
 *
 * Description:
 *   Start the NTP daemon
 *
 ****************************************************************************/

int ntpclient_start(void)
{
  /* Is the NTP in a non-running state? */

  sched_lock();
  if (g_ntpclient_daemon.state == NTP_NOT_RUNNING ||
      g_ntpclient_daemon.state == NTP_STOPPED)
    {
      /* Is this the first time that the NTP daemon has been started? */

      if (g_ntpclient_daemon.state == NTP_NOT_RUNNING)
        {
          /* Yes... then we will need to initialize the state structure */

          sem_init(&g_ntpclient_daemon.interlock, 0, 0);
        }

      /* Start the NTP daemon */

      g_ntpclient_daemon.state = NTP_STARTED;
      g_ntpclient_daemon.pid =
        TASK_CREATE("NTP daemon", CONFIG_NETUTILS_NTPCLIENT_SERVERPRIO,
                    CONFIG_NETUTILS_NTPCLIENT_STACKSIZE, ntpclient_daemon,
                    NULL);

      /* Handle failures to start the NTP daemon */

      if (g_ntpclient_daemon.pid < 0)
        {
          int errval = errno;
          DEBUGASSERT(errval > 0);

          g_ntpclient_daemon.state = NTP_STOPPED;
          ndbg("ERROR: Failed to start the NTP daemon\n", errval);
          return -errval;
        }

      /* Wait for any daemon state change */

      do
        {
          (void)sem_wait(&g_ntpclient_daemon.interlock);
        }
      while (g_ntpclient_daemon.state == NTP_STARTED);
    }

  sched_unlock();
  return OK;
}

/****************************************************************************
 * Name: ntpclient_stop
 *
 * Description:
 *   Stop the NTP daemon
 *
 ****************************************************************************/

int ntpclient_stop(void)
{
  /* Is the NTP in a running state? */

  sched_lock();
  if (g_ntpclient_daemon.state == NTP_STARTED ||
      g_ntpclient_daemon.state == NTP_RUNNING)
    {
      /* Yes.. request that the daemon stop. */

      g_ntpclient_daemon.state = NTP_STOP_REQUESTED;

      /* Wait for any daemon state change */

      do
        {
          (void)sem_wait(&g_ntpclient_daemon.interlock);
        }
      while (g_ntpclient_daemon.state == NTP_STOP_REQUESTED);
    }

  sched_unlock();
  return OK;
}
