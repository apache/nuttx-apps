/****************************************************************************
 * apps/netutils/ping/icmpv6_ping.c
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

#include <sys/socket.h>

#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <poll.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>

#ifdef CONFIG_LIBC_NETDB
#  include <netdb.h>
#endif

#include <arpa/inet.h>

#include <nuttx/clock.h>
#include <nuttx/net/icmpv6.h>
#include "netutils/icmpv6_ping.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ICMPv6_IOBUFFER_SIZE(x) (SIZEOF_ICMPV6_ECHO_REQUEST_S(0) + (x))

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* NOTE: This will not work in the kernel build where there will be a
 * separate instance of g_ping6_id in every process space.
 */

static uint16_t g_ping6_id;
static volatile bool g_exiting6;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sigexit
 ****************************************************************************/

static void sigexit(int signo)
{
  g_exiting6 = true;
}

/****************************************************************************
 * Name: ping6_newid
 ****************************************************************************/

static inline uint16_t ping6_newid(void)
{
  /* Revisit:  No thread safe */

  return ++g_ping6_id;
}

/****************************************************************************
 * Name: ping6_gethostip
 *
 * Description:
 *   Call getaddrinfo() to get the IP address associated with a hostname.
 *
 * Input Parameters
 *   hostname - The host name to use in the nslookup.
 *   dest     - The location to return the IPv6 address.
 *
 * Returned Value:
 *   Zero (OK) on success; ERROR on failure.
 *
 ****************************************************************************/

static int ping6_gethostip(FAR const char *hostname,
                           FAR struct in6_addr *dest)
{
#ifdef CONFIG_LIBC_NETDB
  /* Netdb DNS client support is enabled */

  FAR struct addrinfo hint;
  FAR struct addrinfo *info;
  FAR struct sockaddr_in6 *addr;

  memset(&hint, 0, sizeof(hint));
  hint.ai_family = AF_INET6;

  if (getaddrinfo(hostname, NULL, &hint, &info) != OK)
    {
      return ERROR;
    }

  addr = (FAR struct sockaddr_in6 *)info->ai_addr;
  memcpy(dest, &addr->sin6_addr, sizeof(struct in6_addr));

  freeaddrinfo(info);
  return OK;

#else /* CONFIG_LIBC_NETDB */
  /* No host name support */

  /* Convert strings to numeric IPv6 address */

  int ret = inet_pton(AF_INET6, hostname, dest->s6_addr16);

  /* The inet_pton() function returns 1 if the conversion succeeds. It will
   * return 0 if the input is not a valid IPv6 address string, or -1 with
   * errno set to EAFNOSUPPORT if the address family argument is unsupported.
   */

  return (ret > 0) ? OK : ERROR;

#endif /* CONFIG_LIBC_NETDB */
}

/****************************************************************************
 * Name: icmp6_callback
 ****************************************************************************/

static void icmp6_callback(FAR struct ping6_result_s *result,
                           int code, long extra)
{
  result->code = code;
  result->extra = extra;
  result->info->callback(result);
}

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Name: icmp6_ping
 ****************************************************************************/

void icmp6_ping(FAR const struct ping6_info_s *info)
{
  struct ping6_result_s result;
  struct sockaddr_in6 destaddr;
  struct sockaddr_in6 fromaddr;
  struct icmpv6_echo_request_s outhdr;
  FAR struct icmpv6_echo_reply_s *inhdr;
  struct pollfd recvfd;
  FAR uint8_t *iobuffer;
  FAR uint8_t *ptr;
  long elapsed;
  clock_t kickoff;
  clock_t start;
  socklen_t addrlen;
  ssize_t nsent;
  ssize_t nrecvd;
  bool retry;
  int sockfd;
  int ret;
  int ch;
  int i;

  g_exiting6 = false;
  signal(SIGINT, sigexit);

  /* Initialize result structure */

  memset(&result, 0, sizeof(result));
  result.info = info;
  result.id = ping6_newid();
  result.outsize = ICMPv6_IOBUFFER_SIZE(info->datalen);
  if (ping6_gethostip(info->hostname, &result.dest) < 0)
    {
      icmp6_callback(&result, ICMPv6_E_HOSTIP, 0);
      return;
    }

  /* Allocate memory to hold ping buffer */

  iobuffer = (FAR uint8_t *)malloc(result.outsize);
  if (iobuffer == NULL)
    {
      icmp6_callback(&result, ICMPv6_E_MEMORY, 0);
      return;
    }

  sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_ICMP6);
  if (sockfd < 0)
    {
      icmp6_callback(&result, ICMPv6_E_SOCKET, errno);
      free(iobuffer);
      return;
    }

  kickoff = clock();

  memset(&destaddr, 0, sizeof(struct sockaddr_in6));
  destaddr.sin6_family     = AF_INET6;
  destaddr.sin6_port       = 0;
  memcpy(&destaddr.sin6_addr, &result.dest, sizeof(struct in6_addr));

  memset(&outhdr, 0, SIZEOF_ICMPV6_ECHO_REQUEST_S(0));
  outhdr.type              = ICMPv6_ECHO_REQUEST;
  outhdr.id                = htons(result.id);
  outhdr.seqno             = htons(result.seqno);

  icmp6_callback(&result, ICMPv6_I_BEGIN, 0);

  while (result.nrequests < info->count)
    {
      if (g_exiting6)
        {
          break;
        }

      /* Copy the ICMP header into the I/O buffer */

      memcpy(iobuffer, &outhdr, SIZEOF_ICMPV6_ECHO_REQUEST_S(0));

      /* Add some easily verifiable payload data */

      ptr = &iobuffer[SIZEOF_ICMPV6_ECHO_REQUEST_S(0)];
      ch  = 0x20;

      for (i = 0; i < info->datalen; i++)
        {
          *ptr++ = ch;
          if (++ch > 0x7e)
            {
              ch = 0x20;
            }
        }

      start = clock();
      nsent = sendto(sockfd, iobuffer, result.outsize, 0,
                     (FAR struct sockaddr *)&destaddr,
                     sizeof(struct sockaddr_in6));
      if (nsent < 0)
        {
          icmp6_callback(&result, ICMPv6_E_SENDTO, errno);
          goto done;
        }
      else if (nsent != result.outsize)
        {
          icmp6_callback(&result, ICMPv6_E_SENDSMALL, nsent);
          goto done;
        }

      result.nrequests++;

      elapsed = 0;
      do
        {
          retry           = false;

          recvfd.fd       = sockfd;
          recvfd.events   = POLLIN;
          recvfd.revents  = 0;

          ret = poll(&recvfd, 1, info->timeout - elapsed / USEC_PER_MSEC);
          if (ret < 0)
            {
              icmp6_callback(&result, ICMPv6_E_POLL, errno);
              goto done;
            }
          else if (ret == 0)
            {
              icmp6_callback(&result, ICMPv6_W_TIMEOUT, info->timeout);
              continue;
            }

          /* Get the ICMP response (ignoring the sender) */

          addrlen = sizeof(struct sockaddr_in6);
          nrecvd  = recvfrom(sockfd, iobuffer, result.outsize, 0,
                             (FAR struct sockaddr *)&fromaddr, &addrlen);
          if (nrecvd < 0)
            {
              icmp6_callback(&result, ICMPv6_E_RECVFROM, errno);
              goto done;
            }
          else if (nrecvd < SIZEOF_ICMPV6_ECHO_REPLY_S(0))
            {
              icmp6_callback(&result, ICMPv6_E_RECVSMALL, nrecvd);
              goto done;
            }

          elapsed = TICK2USEC(clock() - start);
          inhdr   = (FAR struct icmpv6_echo_reply_s *)iobuffer;

          if (inhdr->type == ICMPv6_ECHO_REPLY)
            {
#ifndef CONFIG_SIM_NETUSRSOCK
              if (ntohs(inhdr->id) != result.id)
                {
                  icmp6_callback(&result, ICMPv6_W_IDDIFF, ntohs(inhdr->id));
                  retry = true;
                }
              else
#endif
              if (ntohs(inhdr->seqno) > result.seqno)
                {
                  icmp6_callback(&result, ICMPv6_W_SEQNOBIG,
                                 ntohs(inhdr->seqno));
                  retry = true;
                }
              else if (ntohs(inhdr->seqno) < result.seqno)
                {
                  icmp6_callback(&result, ICMPv6_W_SEQNOSMALL,
                                 ntohs(inhdr->seqno));
                  retry = true;
                }
              else
                {
                  bool verified = true;
                  long pktdelay = elapsed;

                  icmp6_callback(&result, ICMPv6_I_ROUNDTRIP, pktdelay);

                  /* Verify the payload data */

                  if (nrecvd != result.outsize)
                    {
                      icmp6_callback(&result, ICMPv6_W_RECVBIG, nrecvd);
                      verified = false;
                    }
                  else
                    {
                      ptr = &iobuffer[SIZEOF_ICMPV6_ECHO_REPLY_S(0)];
                      ch  = 0x20;

                      for (i = 0; i < info->datalen; i++, ptr++)
                        {
                          if (*ptr != ch)
                            {
                              icmp6_callback(&result, ICMPv6_W_DATADIFF, 0);
                              verified = false;
                              break;
                            }

                          if (++ch > 0x7e)
                            {
                              ch = 0x20;
                            }
                        }
                    }

                  /* Only count the number of good replies */

                  if (verified)
                    {
                      result.nreplies++;
                    }
                }
            }
          else
            {
              icmp6_callback(&result, ICMPv6_W_TYPE, inhdr->type);
            }
        }
      while (retry && info->delay > elapsed / USEC_PER_MSEC &&
             info->timeout > elapsed / USEC_PER_MSEC);

      /* Wait if necessary to preserved the requested ping rate */

      elapsed = TICK2MSEC(clock() - start);
      if (elapsed < info->delay)
        {
          struct timespec rqt;
          unsigned int remaining;
          unsigned int sec;
          unsigned int frac;  /* In deciseconds */

          remaining   = info->delay - elapsed;
          sec         = remaining / MSEC_PER_SEC;
          frac        = remaining - MSEC_PER_SEC * sec;

          rqt.tv_sec  = sec;
          rqt.tv_nsec = frac * NSEC_PER_MSEC;

          nanosleep(&rqt, NULL);
        }

      outhdr.seqno = htons(++result.seqno);
    }

done:
  icmp6_callback(&result, ICMPv6_I_FINISH, TICK2USEC(clock() - kickoff));
  close(sockfd);
  free(iobuffer);
}
