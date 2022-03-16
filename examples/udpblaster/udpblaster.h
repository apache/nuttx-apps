/****************************************************************************
 * apps/examples/udpblaster/udpblaster.h
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

#ifndef __APPS_EXAMPLES_UDPBLASTER_UDPBLASTER_H
#define __APPS_EXAMPLES_UDPBLASTER_UDPBLASTER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "config.h"

#include <arpa/inet.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef UDPBLASTER_HOST
/* HTONS/L macros are unique to NuttX */

#  undef HTONS
#  undef HTONL
#  define HTONS(a)       htons(a)
#  define HTONL(a)       htonl(a)

/* Have SO_LINGER */

#  define UDPBLASTER_HAVE_SOLINGER 1

#else
#  ifdef CONFIG_NET_SOLINGER
#    define UDPBLASTER_HAVE_SOLINGER 1
#  else
#    undef UDPBLASTER_HAVE_SOLINGER
#  endif
#endif /* UDPBLASTER_HOST */

#ifdef CONFIG_EXAMPLES_UDPBLASTER_IPv6
#  define AF_INETX AF_INET6
#  define PF_INETX PF_INET6
#else
#  define AF_INETX AF_INET
#  define PF_INETX PF_INET
#endif

#ifdef CONFIG_NET_6LOWPAN
#  define UDPBLASTER_HOST_PORTNO    0xf0b0
#  define UDPBLASTER_TARGET_PORTNO  0xf0b1
#else
#  define UDPBLASTER_HOST_PORTNO    5471
#  define UDPBLASTER_TARGET_PORTNO  5472
#endif

#define ETH_HDRLEN         14    /* Size of the Ethernet header */
#define IPv4_HDRLEN        20    /* Size of IPv4 header */
#define IPv6_HDRLEN        40    /* Size of IPv6 header */
#define UDP_HDRLEN          8    /* Size of UDP header */

#if defined(CONFIG_NET_ETHERNET)
#  define UDPBLASTER_PKTSIZE CONFIG_NET_ETH_PKTSIZE
#  ifdef CONFIG_EXAMPLES_UDPBLASTER_IPv6
#    define UDPBLASTER_MSS   (UDPBLASTER_PKTSIZE - ETH_HDRLEN - IPv6_HDRLEN - UDP_HDRLEN)
#  else
#    define UDPBLASTER_MSS   (UDPBLASTER_PKTSIZE - ETH_HDRLEN - IPv4_HDRLEN - UDP_HDRLEN)
#  endif
#elif defined(CONFIG_NET_LOOPBACK)
#  define UDPBLASTER_PKTSIZE 1518
#  ifdef CONFIG_EXAMPLES_UDPBLASTER_IPv6
#    define UDPBLASTER_MSS   (UDPBLASTER_PKTSIZE - IPv6_HDRLEN - UDP_HDRLEN)
#  else
#    define UDPBLASTER_MSS   (UDPBLASTER_PKTSIZE - IPv4_HDRLEN - UDP_HDRLEN)
#  endif
#elif defined(CONFIG_NET_6LOWPAN)
#  define UDPBLASTER_PKTSIZE CONFIG_NET_6LOWPAN_PKTSIZE
#  define UDPBLASTER_MSS     (CONFIG_NET_6LOWPAN_PKTSIZE - IPv6_HDRLEN - UDP_HDRLEN)
#elif defined(CONFIG_NET_SLIP)
#  define UDPBLASTER_PKTSIZE CONFIG_NET_SLIP_PKTSIZE
#  ifdef CONFIG_EXAMPLES_UDPBLASTER_IPv6
#    define UDPBLASTER_MSS   (UDPBLASTER_PKTSIZE - IPv6_HDRLEN - UDP_HDRLEN)
#  else
#    define UDPBLASTER_MSS   (UDPBLASTER_PKTSIZE - IPv4_HDRLEN - UDP_HDRLEN)
#  endif
#else
#  error "Additional link layer definitions needed"
#endif

#ifndef MIN
#  define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#define UDPBLASTER_SENDSIZE MIN(UDPBLASTER_MSS, g_udpblaster_strlen)

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

extern const char g_udpblaster_text[];
extern const int g_udpblaster_strlen;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#endif /* __APPS_EXAMPLES_UDPBLASTER_UDPBLASTER_H */
