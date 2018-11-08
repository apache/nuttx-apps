/****************************************************************************
 * apps/netutils/ping/icmpv6_ping.c
 *
 *   Copyright (C) 2018 Pinecone Inc. All rights reserved.
 *   Author: Guiding Li<liguiding@pinecone.net>
 *
 * Extracted from logic originally written by:
 *
 *   Copyright (C) 2017-2018 Gregory Nutt. All rights reserved.
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

#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <poll.h>
#include <string.h>
#include <errno.h>

#if defined(CONFIG_LIBC_NETDB) && defined(CONFIG_NETDB_DNSCLIENT)
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

static uint16_t g_ping6_id = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

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
 *   Call gethostbyname() to get the IP address associated with a hostname.
 *
 * Input Parameters
 *   hostname - The host name to use in the nslookup.
 *   ipv4addr - The location to return the IPv4 address.
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

static int ping6_gethostip(FAR const char *hostname, FAR struct in6_addr *dest)
{
#if defined(CONFIG_LIBC_NETDB) && defined(CONFIG_NETDB_DNSCLIENT)
  /* Netdb DNS client support is enabled */

  FAR struct hostent *he;

  he = gethostbyname(hostname);
  if (he == NULL)
    {
      return -ENOENT;
    }
  else if (he->h_addrtype == AF_INET6)
    {
      memcpy(dest, he->h_addr, sizeof(struct in6_addr));
    }
  else
    {
      return -ENOEXEC;
    }

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

static void icmp6_callback(FAR struct ping6_result_s *result, int code, int extra)
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
  int32_t elapsed;
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
                     (FAR struct sockaddr*)&destaddr,
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

          ret = poll(&recvfd, 1, info->timeout - elapsed);
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

          elapsed = (unsigned int)TICK2MSEC(clock() - start);
          inhdr   = (FAR struct icmpv6_echo_reply_s *)iobuffer;

          if (inhdr->type == ICMPv6_ECHO_REPLY)
            {
              if (ntohs(inhdr->id) != result.id)
                {
                  icmp6_callback(&result, ICMPv6_W_IDDIFF, ntohs(inhdr->id));
                  retry = true;
                }
              else if (ntohs(inhdr->seqno) > result.seqno)
                {
                  icmp6_callback(&result, ICMPv6_W_SEQNOBIG, ntohs(inhdr->seqno));
                  retry = true;
                }
              else
                {
                  bool verified = true;
                  int32_t pktdelay = elapsed;

                  if (ntohs(inhdr->seqno) < result.seqno)
                    {
                      icmp6_callback(&result, ICMPv6_W_SEQNOSMALL, ntohs(inhdr->seqno));
                      pktdelay += info->delay;
                      retry     = true;
                    }

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
      while (retry && info->delay > elapsed && info->timeout > elapsed);

      /* Wait if necessary to preserved the requested ping rate */

      elapsed = (unsigned int)TICK2MSEC(clock() - start);
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

          (void)nanosleep(&rqt, NULL);
        }

      outhdr.seqno = htons(++result.seqno);
    }

done:
  icmp6_callback(&result, ICMPv6_I_FINISH, TICK2MSEC(clock() - kickoff));
  close(sockfd);
  free(iobuffer);
}
