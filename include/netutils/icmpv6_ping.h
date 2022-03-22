/****************************************************************************
 * apps/include/netutils/icmpv6_ping.h
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

#define ICMPv6_I_OK          0   /* extra: not used      */
#define ICMPv6_I_BEGIN       1   /* extra: not used      */
#define ICMPv6_I_ROUNDTRIP   2   /* extra: packet delay  */
#define ICMPv6_I_FINISH      3   /* extra: elapsed time  */

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
  long extra;               /* Extra information for code */
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
