/****************************************************************************
 * netutils/ping/icmp_ping.c
 *
 *   Copyright (C) 2015 Gregory Nutt. All rights reserved.
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

#include <sys/socket.h>
#include <signal.h>
#include <time.h>

#include <arpa/inet.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ICMPBUF ((struct icmp_iphdr_s *)&dev->d_buf[NET_LL_HDRLEN(dev)])
#define ICMPDAT (&dev->d_buf[NET_LL_HDRLEN(dev) + sizeof(struct icmp_iphdr_s)])

/* Use the monotonic clock if it is available */

#ifdef CONFIG_CLOCK_MONOTONIC
#  define PING_CLOCK  CLOCK_MONOTONIC
#else
#  define PING_CLOCK  CLOCK_REALTIME
#endif

#ifndef CONFIG_NETUTILS_PING_SIGNO
#  define CONFIG_NETUTILS_PING_SIGNO 13
#endif

#define ICMP_DATALEN      56
#define IPv4_PAYLOAD_SIZE (ICMP_HDRLEN + ICMP_DATALEN)
#define PING_BUFFER_SIZE  (IPv4_HDRLEN + IPv4_PAYLOAD_SIZE)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: icmpv6_echo_request
 *
 * Description:
 *   Setup to send an ICMP packet
 *
 * Input Parameters:
 *
 *
 * Return:
 *   None
 *
 ****************************************************************************/

static void icmp_echo_request(FAR struct net_driver_s *dev,
                              FAR in_addr_t *destaddr)
{
  FAR struct icmp_iphdr_s *picmp = ICMPBUF;
  FAR uint8_t *ptr;
  size_t sendlen;
  size_t icmplen;
  size_t pktlen;
  int i;

  /* The total length to send is the size of the application data plus the
   * IPv4 and ICMP headers (and, eventually, the Ethernet header)
   */

  pktlen = IPv4_PAYLOAD_SIZE + IPICMP_HDRLEN;

  /* The total size of the data (for ICMP checksum calculation) includes the
   * size of the ICMP header
   */

  icmplen = IPv4_PAYLOAD_SIZE + ICMP_HDRLEN;

  /* Initialize the IP header. */

  picmp->vhl         = 0x45;
  picmp->tos         = 0;
  picmp->len[0]      = (pktlen >> 8);
  picmp->len[1]      = (pktlen & 0xff);
  ++g_ipid;
  picmp->ipid[0]     = g_ipid >> 8;
  picmp->ipid[1]     = g_ipid & 0xff;
  picmp->ipoffset[0] = IP_FLAG_DONTFRAG >> 8;
  picmp->ipoffset[1] = IP_FLAG_DONTFRAG & 0xff;
  picmp->ttl         = IP_TTL;
  picmp->proto       = IP_PROTO_ICMP;

  net_ipv4addr_hdrcopy(picmp->srcipaddr, &dev->d_ipaddr);
  net_ipv4addr_hdrcopy(picmp->destipaddr, destaddr);

  /* Format the ICMP ECHO request packet */

  picmp->type  = ICMP_ECHO_REQUEST;
  picmp->icode = 0;
  picmp->id    = htons(pstate->png_id);
  picmp->seqno = htons(pstate->png_seqno);

  /* Insert some recognizable data into the packet */

  for (i = 0, ptr = ICMPDAT; i < ICMP_DATALEN; i++)
    {
      *ptr++ = i;
    }

  /* Calculate IP checksum. */

  picmp->ipchksum    = 0;
  picmp->ipchksum    = ~(ipv4_chksum(dev));

  /* Calculate the ICMP checksum. */

  picmp->icmpchksum  = 0;
  picmp->icmpchksum  = ~(icmp_chksum(dev, icmplen));
  if (picmp->icmpchksum == 0)
    {
      picmp->icmpchksum = 0xffff;
    }
}

static void ping_timeout(int signo, FAR siginfo_t *info, FAR void *arg)
{
  FAR bool *timeout = (FAR bool *)arg;
  *timeout = true;
}

int ipv4_ping(FAR struct sockaddr_in *raddr,
              FAR const struct timespec *timeout,
              FAR struct timespec *roundtrip)
{
  FAR struct icmp *icmpv4;
  struct sigevent notify;
  struct sigaction act;
  struct sigaction oact;
  struct itimerspec value;
  struct itimerspec ovalue;
  char buffer[PING_BUFFER_SIZE]; /* Kind of big to be on the stack */
  timer_t timerid;
  ssize_t nrecvd;
  bool timeout;
  int errcode;
  int ret;
  int sd;

  DEBUGASSERT(raddr != NULL);

  /* Allocate a POSIX timer */

  timeout                      = false;
  notify.sigev_notify          = SIGEV_SIGNAL;
  notify.sigev_signo           = CONFIG_NETUTILS_PING_SIGNO;
  notify.sigev_value.sival_ptr = (FAR void *)&timeout;

  ret = timer_create(PING_CLOCK, &notify, &timerid);
  if (ret < 0)
    {
      errcode = errno;
      DEBUGASSERT(errno > 0);
      return -errcode;
    }

  /* Attach a signal handler */

  act.sa_sigaction = ping_timeout;
  act.sa_flags     = SA_SIGINFO;

  (void)sigfillset(&act.sa_mask);
  (void)sigdelset(&act.sa_mask, CONFIG_NETUTILS_PING_SIGNO);

  ret = sigaction(CONFIG_NETUTILS_PING_SIGNO, &act, &oact);
  if (ret < 0)
    {
      errcode = errno;
      DEBUGASSERT(errno > 0);
      ret = -errcode;
      goto errout_with_timer;
    }

  /* Create the raw socket */

  sd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (sd < 0)
    {
      errcode = errno;
      DEBUGASSERT(errno > 0);
      ret = -errcode;
      goto errout_with_sigaction;
    }

  /* Format and send the ECHO request */

  icmpv6_echo_request(dev, destaddr);

  ret = sendto(sd, buffer, PING6_BUFFER_SIZE, &raddr,
               sizeof(struct sockaddr_in));

  if (ret < 0)
    {
      errcode = errno;
      DEBUGASSERT(errno > 0);
      ret = -errcode;
      goto errout_with_socket;
    }

  /* Start the timer */

  value.it_value.tv_sec     = timeout->tv_sec;
  value.it_value.tv_nsec    = timeout->tv_nsec;
  value.it_interval.tv_sec  = 0;
  value.it_interval.tv_nsec = 0;

  ret = timer_settime(timerid, 0, &value, NULL);
  if (ret < 0)
    {
      errcode = errno;
      DEBUGASSERT(errno > 0);
      ret = -errcode;
      goto errout_with_socket;
    }

  /* Wait for the echo reply */

  for (; ; )
    {
      nrecvd = recv(sd, buffer, PING_BUFFER_SIZE, 0);
      if (nrecvd < 0)
        {
          if (errno != EINTR)
            {
              int errcode = errno;
              DEBUGASSERT(errno > 0);

              nerr("ERROR: recv failed: %d\n", errcode);
              ret = -errcode;
              break;
            }
        }
      else if (nrecvd >= PING_BUFFER_SIZE)
        {
          FAR struct iphdr *iphdr = (FAR struct iphdr *)buffer;

          icmpv4 = (FAR struct icmp *)(buffer + (iphdr->ihl << 2));  /* skip ip hdr */
          if (icmpv4->icmp_type == ICMP_ECHO_REPLY)
            {
              ret = OK;
              break;
            }
        }
    }

  /* Stop the timer */

  value.it_value.tv_sec     = 0;
  value.it_value.tv_nsec    = 0;
  value.it_interval.tv_sec  = 0;
  value.it_interval.tv_nsec = 0;

  (void)timer_settime(timerid, 0, &value, &ovalue);
  if (ret < 0)
    {
      errcode = errno;
      DEBUGASSERT(errno > 0);
      ret = -errcode;
    }
  else
    {
      rountrip->tv_sec  = ovalue.it_value.tv_sec;
      rountrip->tv_nsec = ovalue.it_value.tv_nsec;
      ret = OK;
    }

errout_with_socket:
  close(sd);

errout_with_sigaction:
  (void) sigaction(CONFIG_NETUTILS_PING_SIGNO, &oact, NULL);

errout_with_timer:
  timer_delete(timerid);
  return ret;
}
