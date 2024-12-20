/****************************************************************************
 * apps/netutils/ping/icmp_ping.c
 *
 * SPDX-License-Identifier: Apache-2.0
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
#include <nuttx/net/icmp.h>

#include "netutils/icmp_ping.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ICMP_IOBUFFER_SIZE(x) (sizeof(struct icmp_hdr_s) + (x))

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Data needed for ping, reduce stack usage. */

struct ping_priv_s
{
  struct sockaddr_in destaddr;
  struct sockaddr_in fromaddr;
  struct icmp_hdr_s outhdr;
  struct pollfd recvfd;
  socklen_t addrlen;
  clock_t kickoff;
  clock_t start;
  ssize_t nsent;
  ssize_t nrecvd;
  long elapsed;
  bool retry;
  int sockfd;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* NOTE: This will not work in the kernel build where there will be a
 * separate instance of g_pingid in every process space.
 */

static uint16_t g_pingid;
static volatile bool g_exiting;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sigexit
 ****************************************************************************/

static void sigexit(int signo)
{
  g_exiting = true;
}

/****************************************************************************
 * Name: ping_newid
 ****************************************************************************/

static inline uint16_t ping_newid(void)
{
  /* Revisit:  No thread safe */

  return ++g_pingid;
}

/****************************************************************************
 * Name: ping_gethostip
 *
 * Description:
 *   Call getaddrinfo() to get the IP address associated with a hostname.
 *
 * Input Parameters
 *   hostname - The host name to use in the nslookup.
 *   destr    - The location to return the IPv4 address.
 *
 * Returned Value:
 *   Zero (OK) on success; ERROR on failure.
 *
 ****************************************************************************/

static int ping_gethostip(FAR const char *hostname, FAR struct in_addr *dest)
{
#ifdef CONFIG_LIBC_NETDB
  /* Netdb DNS client support is enabled */

  FAR struct addrinfo *info;
  FAR struct sockaddr_in *addr;
  static const struct addrinfo s_hint =
  {
    .ai_family = AF_INET
  };

  if (getaddrinfo(hostname, NULL, &s_hint, &info) != OK)
    {
      return ERROR;
    }

  addr = (FAR struct sockaddr_in *)info->ai_addr;
  memcpy(dest, &addr->sin_addr, sizeof(struct in_addr));

  freeaddrinfo(info);
  return OK;

#else /* CONFIG_LIBC_NETDB */
  /* No host name support */

  /* Convert strings to numeric IPv4 address */

  int ret = inet_pton(AF_INET, hostname, dest);

  /* The inet_pton() function returns 1 if the conversion succeeds. It will
   * return 0 if the input is not a valid IPv4 dotted-decimal string or -1
   * with errno set to EAFNOSUPPORT if the address family argument is
   * unsupported.
   */

  return (ret > 0) ? OK : ERROR;

#endif /* CONFIG_LIBC_NETDB */
}

/****************************************************************************
 * Name: icmp_callback
 ****************************************************************************/

static void icmp_callback(FAR struct ping_result_s *result,
                          int code, long extra)
{
  result->code = code;
  result->extra = extra;
  result->info->callback(result);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: icmp_ping
 ****************************************************************************/

void icmp_ping(FAR const struct ping_info_s *info)
{
  struct ping_result_s result;
  FAR struct ping_priv_s *priv;
  FAR struct icmp_hdr_s *inhdr;
  FAR uint8_t *iobuffer;
  FAR uint8_t *ptr;
  int ret;
  int ch;
  int i;

  g_exiting = false;
  signal(SIGINT, sigexit);

  /* Initialize result structure */

  memset(&result, 0, sizeof(result));
  result.info = info;
  result.id = ping_newid();
  result.outsize = ICMP_IOBUFFER_SIZE(info->datalen);
  if (ping_gethostip(info->hostname, &result.dest) < 0)
    {
      icmp_callback(&result, ICMP_E_HOSTIP, 0);
      return;
    }

  /* Allocate memory to hold private data and ping buffer */

  priv = malloc(sizeof(*priv) + result.outsize);
  if (priv == NULL)
    {
      icmp_callback(&result, ICMP_E_MEMORY, 0);
      return;
    }

  iobuffer = (FAR uint8_t *)(priv + 1);

  priv->sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
  if (priv->sockfd < 0)
    {
      icmp_callback(&result, ICMP_E_SOCKET, errno);
      free(priv);
      return;
    }

  priv->kickoff = clock();

  memset(&priv->destaddr, 0, sizeof(struct sockaddr_in));
  priv->destaddr.sin_family      = AF_INET;
  priv->destaddr.sin_port        = 0;
  priv->destaddr.sin_addr.s_addr = result.dest.s_addr;

  memset(&priv->outhdr, 0, sizeof(struct icmp_hdr_s));
  priv->outhdr.type              = ICMP_ECHO_REQUEST;
  priv->outhdr.id                = htons(result.id);
  priv->outhdr.seqno             = htons(result.seqno);

  icmp_callback(&result, ICMP_I_BEGIN, 0);

  while (result.nrequests < info->count)
    {
      if (g_exiting)
        {
          break;
        }

      /* Copy the ICMP header into the I/O buffer */

      memcpy(iobuffer, &priv->outhdr, sizeof(struct icmp_hdr_s));

      /* Add some easily verifiable payload data */

      ptr = &iobuffer[sizeof(struct icmp_hdr_s)];
      ch  = 0x20;

      for (i = 0; i < info->datalen; i++)
        {
          *ptr++ = ch;
          if (++ch > 0x7e)
            {
              ch = 0x20;
            }
        }

      priv->start = clock();
      result.nrequests++;
      priv->nsent = sendto(priv->sockfd, iobuffer, result.outsize, 0,
                           (FAR struct sockaddr *)&priv->destaddr,
                           sizeof(struct sockaddr_in));
      if (priv->nsent < 0)
        {
          icmp_callback(&result, ICMP_E_SENDTO, errno);
          goto wait;
        }
      else if (priv->nsent != result.outsize)
        {
          icmp_callback(&result, ICMP_E_SENDSMALL, priv->nsent);
          goto wait;
        }

      priv->elapsed = 0;
      do
        {
          priv->retry          = false;
          priv->recvfd.fd      = priv->sockfd;
          priv->recvfd.events  = POLLIN;
          priv->recvfd.revents = 0;

          ret = poll(&priv->recvfd, 1,
                     info->timeout - priv->elapsed / USEC_PER_MSEC);
          if (ret < 0)
            {
              icmp_callback(&result, ICMP_E_POLL, errno);
              goto wait;
            }
          else if (ret == 0)
            {
              icmp_callback(&result, ICMP_W_TIMEOUT, info->timeout);
              goto wait;
            }

          if (priv->recvfd.revents & (POLLHUP | POLLERR))
            {
              icmp_callback(&result, ICMP_E_POLL, ENETDOWN);
              goto wait;
            }

          /* Get the ICMP response (ignoring the sender) */

          priv->addrlen = sizeof(struct sockaddr_in);
          priv->nrecvd  = recvfrom(priv->sockfd, iobuffer,
                                   result.outsize, 0,
                                   (FAR struct sockaddr *)&priv->fromaddr,
                                   &priv->addrlen);
          if (priv->nrecvd < 0)
            {
              icmp_callback(&result, ICMP_E_RECVFROM, errno);
              goto wait;
            }
          else if (priv->nrecvd < sizeof(struct icmp_hdr_s))
            {
              icmp_callback(&result, ICMP_E_RECVSMALL, priv->nrecvd);
              goto wait;
            }

          priv->elapsed = TICK2USEC(clock() - priv->start);
          inhdr         = (FAR struct icmp_hdr_s *)iobuffer;

          if (inhdr->type == ICMP_ECHO_REPLY)
            {
#ifndef CONFIG_SIM_NETUSRSOCK
              if (ntohs(inhdr->id) != result.id)
                {
                  icmp_callback(&result, ICMP_W_IDDIFF, ntohs(inhdr->id));
                  priv->retry = true;
                }
              else
#endif
              if (ntohs(inhdr->seqno) > result.seqno)
                {
                  icmp_callback(&result, ICMP_W_SEQNOBIG,
                                ntohs(inhdr->seqno));
                  priv->retry = true;
                }
              else if (ntohs(inhdr->seqno) < result.seqno)
                {
                  icmp_callback(&result, ICMP_W_SEQNOSMALL,
                                ntohs(inhdr->seqno));
                  priv->retry = true;
                }
              else
                {
                  bool verified = true;

                  icmp_callback(&result, ICMP_I_ROUNDTRIP, priv->elapsed);

                  /* Verify the payload data */

                  if (priv->nrecvd != result.outsize)
                    {
                      icmp_callback(&result, ICMP_W_RECVBIG, priv->nrecvd);
                      verified = false;
                    }
                  else
                    {
                      ptr = &iobuffer[sizeof(struct icmp_hdr_s)];
                      ch  = 0x20;

                      for (i = 0; i < info->datalen; i++, ptr++)
                        {
                          if (*ptr != ch)
                            {
                              icmp_callback(&result, ICMP_W_DATADIFF, 0);
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
              icmp_callback(&result, ICMP_W_TYPE, inhdr->type);
            }
        }
      while (priv->retry && info->delay > priv->elapsed / USEC_PER_MSEC &&
             info->timeout > priv->elapsed / USEC_PER_MSEC);

      /* Wait if necessary to preserved the requested ping rate */

wait:
      priv->elapsed = TICK2MSEC(clock() - priv->start);
      if (priv->elapsed < info->delay)
        {
          struct timespec rqt;
          unsigned int remaining;
          unsigned int sec;
          unsigned int frac;  /* In deciseconds */

          remaining   = info->delay - priv->elapsed;
          sec         = remaining / MSEC_PER_SEC;
          frac        = remaining - MSEC_PER_SEC * sec;

          rqt.tv_sec  = sec;
          rqt.tv_nsec = frac * NSEC_PER_MSEC;

          nanosleep(&rqt, NULL);
        }

      priv->outhdr.seqno = htons(++result.seqno);
    }

  icmp_callback(&result, ICMP_I_FINISH, TICK2USEC(clock() - priv->kickoff));
  close(priv->sockfd);
  free(priv);
}
