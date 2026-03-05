/****************************************************************************
 * apps/include/netutils/icmp_pub.h
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

#ifndef __APPS_INCLUDE_NETUTILS_ICMP_PUB_H
#define __APPS_INCLUDE_NETUTILS_ICMP_PUB_H

#define ICMP_E_HOSTIP      -1  /* extra: not used      */

struct ping_result_s;

struct ping_info_s
{
  FAR const char *hostname; /* Host name to ping */
#ifdef CONFIG_NET_BINDTODEVICE
  FAR const char *devname;  /* Device name to bind */
#endif
  uint16_t flag;            /* v4 or v6 */
  uint16_t count;           /* Number of pings requested */
  uint16_t datalen;         /* Number of bytes to be sent */
  uint16_t delay;           /* Deciseconds to delay between pings */
  uint16_t timeout;         /* Deciseconds to wait response before timeout */
  FAR void *priv;           /* Private context for callback */
  void (*callback)(FAR const struct ping_result_s *result);
};

struct ping_result_s
{
  int code;                 /* Notice code ICMPv6_I/E/W_XXX */
  long extra;               /* Extra information for code */
  union
    {
#ifdef CONFIG_SYSTEM_PING
      struct in_addr v4;    /* Target address to ping */
#endif
#ifdef CONFIG_SYSTEM_PING6
     struct in6_addr v6;   /* Target address to ping */
#endif
  } dest;
  uint16_t nrequests;       /* Number of ICMP ECHO requests sent */
  uint16_t nreplies;        /* Number of matching ICMP ECHO replies received */
  uint16_t outsize;         /* Bytes(include ICMP header) to be sent */
  uint16_t id;              /* ICMPv6_ECHO id */
  uint16_t seqno;           /* ICMPv6_ECHO seqno */
  FAR const struct ping_info_s *info;
};

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

void icmp_ping(FAR const struct ping_info_s *info);
void icmp6_ping(FAR const struct ping_info_s *info);

#undef EXTERN
#ifdef __cplusplus
}
#endif
#endif /* __APPS_INCLUDE_NETUTILS_ICMP_PUB_H */
