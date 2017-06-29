/****************************************************************************
 * examples/udp/udp.h
 *
 *   Copyright (C) 2007, 2008, 2015, 2017 Gregory Nutt. All rights reserved.
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

#ifndef __EXAMPLES_UDP_UDP_H
#define __EXAMPLES_UDP_UDP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#ifdef EXAMPLES_UDP_HOST
#else
# include <debug.h>
#endif

#include <arpa/inet.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef EXAMPLES_UDP_HOST
   /* HTONS/L macros are unique to uIP-based networks */

#  ifdef CONFIG_ENDIAN_BIG
#    define HTONS(ns) (ns)
#    define HTONL(nl) (nl)
#  else
#    define HTONS(ns) \
       (unsigned short) \
         (((((unsigned short)(ns)) & 0x00ff) << 8) | \
         ((((unsigned short)(ns)) >> 8) & 0x00ff))
#      define HTONL(nl) \
       (unsigned long) \
         (((((unsigned long)(nl)) & 0x000000ffUL) << 24) | \
         ((((unsigned long)(nl)) & 0x0000ff00UL) <<  8) | \
         ((((unsigned long)(nl)) & 0x00ff0000UL) >>  8) | \
         ((((unsigned long)(nl)) & 0xff000000UL) >> 24))
#  endif

#  define NTOHS(hs) HTONS(hs)
#  define NTOHL(hl) HTONL(hl)
#  define FAR
#endif

#ifdef CONFIG_EXAMPLES_UDP_IPv6
#  define AF_INETX AF_INET6
#  define PF_INETX PF_INET6
#else
#  define AF_INETX AF_INET
#  define PF_INETX PF_INET
#endif

#ifndef CONFIG_EXAMPLES_UDP_SERVER_PORTNO
#  define CONFIG_EXAMPLES_UDP_SERVER_PORTNO 5471
#endif

#ifndef CONFIG_EXAMPLES_UDP_CLIENT_PORTNO
#  define CONFIG_EXAMPLES_UDP_CLIENT_PORTNO 5472
#endif

#define ASCIISIZE  (0x7f - 0x20)
#define SENDSIZE   (ASCIISIZE+1)

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_UDP_IPv6
uint16_t g_udpserver_ipv6[8];
#else
uint32_t g_udpserver_ipv4;
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_UDP_NETINIT
int udp_netinit(void);
#endif

void udp_cmdline(int argc, char **argv);
void udp_client(void);
void udp_server(void);

#endif /* __EXAMPLES_UDP_UDP_H */
