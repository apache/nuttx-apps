/****************************************************************************
 * apps/include/netutils/icmpv6_ping.h
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

#ifndef __APPS_INCLUDE_NETUTILS_ICMPV6_PING_H
#define __APPS_INCLUDE_NETUTILS_ICMPV6_PING_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <netinet/in.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Positive number represent information */

#define ICMPv6_I_BEGIN       0   /* extra: not used      */
#define ICMPv6_I_ROUNDTRIP   1   /* extra: packet delay  */
#define ICMPv6_I_FINISH      2   /* extra: elapsed time  */

/* Negative odd number represent error(unrecoverable) */

#define ICMPv6_E_HOSTIP      -1  /* extra: not used      */
#define ICMPv6_E_MEMORY      -3  /* extra: not used      */
#define ICMPv6_E_SOCKET      -5  /* extra: error code    */
#define ICMPv6_E_SENDTO      -7  /* extra: error code    */
#define ICMPv6_E_SENDSMALL   -9  /* extra: sent bytes    */
#define ICMPv6_E_POLL        -11 /* extra: error code    */
#define ICMPv6_E_RECVFROM    -13 /* extra: error code    */
#define ICMPv6_E_RECVSMALL   -15 /* extra: recv bytes    */

/* Negative even number represent warning(recoverable) */

#define ICMPv6_W_TIMEOUT     -2  /* extra: timeout value */
#define ICMPv6_W_IDDIFF      -4  /* extra: recv id       */
#define ICMPv6_W_SEQNOBIG    -6  /* extra: recv seqno    */
#define ICMPv6_W_SEQNOSMALL  -8  /* extra: recv seqno    */
#define ICMPv6_W_RECVBIG     -10 /* extra: recv bytes    */
#define ICMPv6_W_DATADIFF    -12 /* extra: not used      */
#define ICMPv6_W_TYPE        -14 /* extra: recv type     */

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct ping6_result_s;

struct ping6_info_s
{
  FAR const char *hostname; /* Host name to ping */
  uint16_t count;           /* Number of pings requested */
  uint16_t datalen;         /* Number of bytes to be sent */
  uint16_t delay;           /* Deciseconds to delay between pings */
  uint16_t timeout;         /* Deciseconds to wait response before timeout */
  FAR void *priv;           /* Private context for callback */
  void (*callback)(FAR const struct ping6_result_s *result);
};

struct ping6_result_s
{
  int code;                 /* Notice code ICMPv6_I/E/W_XXX */
  int extra;                /* Extra information for code */
  struct in6_addr dest;     /* Target address to ping */
  uint16_t nrequests;       /* Number of ICMP ECHO requests sent */
  uint16_t nreplies;        /* Number of matching ICMP ECHO replies received */
  uint16_t outsize;         /* Bytes(include ICMP header) to be sent */
  uint16_t id;              /* ICMPv6_ECHO id */
  uint16_t seqno;           /* ICMPv6_ECHO seqno */
  FAR const struct ping6_info_s *info;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

void icmp6_ping(FAR const struct ping6_info_s *info);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_NETUTILS_ICMP_PING_H */
