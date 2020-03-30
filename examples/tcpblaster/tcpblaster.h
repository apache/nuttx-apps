/****************************************************************************
 * examples/tcpblaster/tcpblaster.h
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
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

#ifndef __APPS_EXAMPLES_TCPBLASTER_H
#define __APPS_EXAMPLES_TCPBLASTER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <arpa/inet.h>

#ifdef TCPBLASTER_HOST
#else
# include <debug.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef TCPBLASTER_HOST
#undef HTONS
#ifdef CONFIG_ENDIAN_BIG
#  define HTONS(ns) (ns)
#else
#  define HTONS(ns) \
     (unsigned short) \
       (((((unsigned short)(ns)) & 0x00ff) << 8) | \
        ((((unsigned short)(ns)) >> 8) & 0x00ff))
#endif

#undef HTONL
#ifdef CONFIG_ENDIAN_BIG
#  define HTONL(nl) (nl)
#else
#  define HTONL(nl) \
     (unsigned long) \
       (((((unsigned long)(nl)) & 0x000000ffUL) << 24) | \
        ((((unsigned long)(nl)) & 0x0000ff00UL) <<  8) | \
        ((((unsigned long)(nl)) & 0x00ff0000UL) >>  8) | \
        ((((unsigned long)(nl)) & 0xff000000UL) >> 24))
#endif

#undef NTOHS
#define NTOHS(hs) HTONS(hs)

#undef NTOHL
#define NTOHL(hl) HTONL(hl)

   /* Have SO_LINGER */

#  define FAR
#  define TCPBLASTER_HAVE_SOLINGER 1

#else
#  ifdef CONFIG_NET_SOLINGER
#    define TCPBLASTER_HAVE_SOLINGER 1
#  else
#    undef TCPBLASTER_HAVE_SOLINGER
#  endif
#endif /* TCPBLASTER_HOST */

#ifdef CONFIG_EXAMPLES_TCPBLASTER_IPv6
#  define AF_INETX AF_INET6
#  define PF_INETX PF_INET6
#else
#  define AF_INETX AF_INET
#  define PF_INETX PF_INET
#endif

#ifndef CONFIG_EXAMPLES_TCPBLASTER_SERVER_PORTNO
#  define CONFIG_EXAMPLES_TCPBLASTER_SERVER_PORTNO 5471
#endif

#ifdef CONFIG_EXAMPLES_TCPBLASTER_SENDSIZE
#  define SENDSIZE CONFIG_EXAMPLES_TCPBLASTER_SENDSIZE
#else
#  define SENDSIZE 4096
#endif

#ifdef CONFIG_EXAMPLES_TCPBLASTER_GROUPSIZE
#  define GROUPSIZE CONFIG_EXAMPLES_TCPBLASTER_GROUPSIZE
#else
#  define GROUPSIZE 50
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_TCPBLASTER_IPv6
extern uint16_t g_tcpblasterserver_ipv6[8];
#else
extern uint32_t g_tcpblasterserver_ipv4;
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_TCPBLASTER_INIT
void tcpblaster_initialize(void);
#endif

void tcpblaster_cmdline(int argc, char **argv);
extern void tcpblaster_client(void);
extern void tcpblaster_server(void);

#endif /* __APPS_EXAMPLES_TCPBLASTER_H */
