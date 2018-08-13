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

#define ICMPv6BUF ((struct icmpv6_iphdr_s *)&dev->d_buf[NET_LL_HDRLEN(dev)])
#define ICMPv6ECHOREQ \
  ((struct icmpv6_echo_request_s *)&dev->d_buf[NET_LL_HDRLEN(dev) + IPv6_HDRLEN])
#define ICMPv6ECHOREPLY \
  ((struct icmpv6_echo_reply_s *)&dev->d_buf[NET_LL_HDRLEN(dev) + IPv6_HDRLEN])

/* Use the monotonic clock if it is available */

#ifdef CONFIG_CLOCK_MONOTONIC
#  define PING_CLOCK  CLOCK_MONOTONIC
#else
#  define PING_CLOCK  CLOCK_REALTIME
#endif

#ifndef CONFIG_NETUTILS_PING_SIGNO
#  define CONFIG_NETUTILS_PING_SIGNO 13
#endif

#define ICMPv6_DATALEN     56
#define IPv6_PAYLOAD_SIZE (ICMPv6_HDRLEN + ICMPv6_DATALEN)
#define PING6_BUFFER_SIZE (IPv6_HDRLEN + IPv6_PAYLOAD_SIZE)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: icmpv6_echo_request
 *
 * Description:
 *   Format an ICMPv6 Echo Request message.
 *
 * Parameters:
 *
 *
 * Return:
 *   None
 *
 ****************************************************************************/

static void icmpv6_echo_request(FAR struct net_driver_s *dev,
                                FAR struct icmpv6_ping_s *pstate)
{
  FAR struct icmpv6_iphdr_s *icmp;
  FAR struct icmpv6_echo_request_s *req;
  uint16_t reqlen;
  int i;

  ninfo("Send ECHO request: seqno=%d\n", pstate->png_seqno);

  /* Set up the IPv6 header (most is probably already in place) */

  icmp          = ICMPv6BUF;
  icmp->vtc     = 0x60;                    /* Version/traffic class (MS) */
  icmp->tcf     = 0;                       /* Traffic class (LS)/Flow label (MS) */
  icmp->flow    = 0;                       /* Flow label (LS) */

  /* Length excludes the IPv6 header */

  reqlen        = SIZEOF_ICMPV6_ECHO_REQUEST_S(pstate->png_datlen);
  icmp->len[0]  = (reqlen >> 8);
  icmp->len[1]  = (reqlen & 0xff);

  icmp->proto   = IP_PROTO_ICMP6;          /* Next header */
  icmp->ttl     = IP_TTL;                  /* Hop limit */

  /* Set the multicast destination IP address */

  net_ipv6addr_copy(icmp->destipaddr,  pstate->png_addr);

  /* Add out IPv6 address as the source address */

  net_ipv6addr_copy(icmp->srcipaddr, dev->d_ipv6addr);

  /* Set up the ICMPv6 Echo Request message */

  req           = ICMPv6ECHOREQ;
  req->type     = ICMPv6_ECHO_REQUEST; /* Message type */
  req->code     = 0;                   /* Message qualifier */
  req->id       = htons(pstate->png_id);
  req->seqno    = htons(pstate->png_seqno);

  /* Add some easily verifiable data */

  for (i = 0; i < pstate->png_datlen; i++)
    {
      req->data[i] = i;
    }

  /* Calculate the checksum over both the ICMP header and payload */

  icmp->chksum  = 0;
  icmp->chksum  = ~icmpv6_chksum(dev);

  /* Set the size to the size of the IPv6 header and the payload size */

  IFF_SET_IPv6(dev->d_flags);

  dev->d_sndlen = reqlen;
  dev->d_len    = reqlen + IPv6_HDRLEN;
}

static void ping6_timeout(int signo, FAR siginfo_t *info, FAR void *arg)
{
  FAR bool *timeout = (FAR bool *)arg;
  *timeout = true;
}

int ipv6_ping(FAR struct sockaddr_in6 *raddr,
              FAR const struct timespec *timeout,
              FAR struct timespec *roundtrip)
{
  FAR struct icmp6_hdr *icmpv6;
  struct sigevent notify;
  struct sigaction act;
  struct sigaction oact;
  struct itimerspec value;
  struct itimerspec ovalue;
  char buffer[PING6_BUFFER_SIZE];
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

  sd = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
  if (sd < 0)
    {
      errcode = errno;
      DEBUGASSERT(errno > 0);
      ret = -errcode;
      goto errout_with_sigaction;
    }

  /* Format and send the ECHO request */

  icmpv6_echo_request(FAR struct net_driver_s *dev,
                      FAR struct icmpv6_ping_s *pstate);

  ret = sendto(sd, buffer, PING6_BUFFER_SIZE, &raddr,
               sizeof(struct sockaddr_in6));

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
      nrecvd = recv(sd, buffer, PING6_BUFFER_SIZE, 0);
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
      else if (nrecvd >= PING6_BUFFER_SIZE &&
               icmpv6->icmp6_type == ICMPv6_ECHO_REPLY)
        {
          ret = OK;
          break;
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
