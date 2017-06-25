/****************************************************************************
 * examples/nettest/nettest.h
 *
 *   Copyright (C) 2007, 2009, 2015 Gregory Nutt. All rights reserved.
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

#ifndef __EXAMPLES_NETTEST_H
#define __EXAMPLES_NETTEST_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <arpa/inet.h>

#ifdef NETTEST_HOST
#else
# include <debug.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef NETTEST_HOST
   /* HTONS/L macros are unique to uIP */

#  define HTONS(a)       htons(a)
#  define HTONL(a)       htonl(a)

   /* Have SO_LINGER */

#  define NETTEST_HAVE_SOLINGER 1

#else
#  ifdef CONFIG_NET_SOLINGER
#    define NETTEST_HAVE_SOLINGER 1
#  else
#    undef NETTEST_HAVE_SOLINGER
#  endif
#endif /* NETTEST_HOST */

#ifdef CONFIG_EXAMPLES_NETTEST_IPv6
#  define AF_INETX AF_INET6
#  define PF_INETX PF_INET6
#else
#  define AF_INETX AF_INET
#  define PF_INETX PF_INET
#endif

#ifndef CONFIG_EXAMPLES_NETTEST_SERVER_PORTNO
#  define CONFIG_EXAMPLES_NETTEST_SERVER_PORTNO 5471
#endif

#ifdef CONFIG_EXAMPLES_NETTEST_SENDSIZE
#  define SENDSIZE CONFIG_EXAMPLES_NETTEST_SENDSIZE
#else
#  define SENDSIZE 4096
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_NETTEST_IPv6
uint16_t g_nettestserver_ipv6[8];
#else
uint32_t g_nettestserver_ipv4;
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_NETTEST_INIT
void nettest_initialize(void);
#endif

void nettest_cmdline(int argc, char **argv);
extern void nettest_client(void);
extern void nettest_server(void);

#endif /* __EXAMPLES_NETTEST_H */
