/****************************************************************************
 * examples/udpblaster/udpblaster.h
 *
 *   Copyright (C) 2015, 2018 Gregory Nutt. All rights reserved.
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

#ifndef __EXAMPLES_UDPBLASTER_UDPBLASTER_H
#define __EXAMPLES_UDPBLASTER_UDPBLASTER_H

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
#  define MIN(a,b) ((a)<(b)?(a):(b))
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

#endif /* __EXAMPLES_UDPBLASTER_UDPBLASTER_H */
