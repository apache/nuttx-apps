/****************************************************************************
 * apps/examples/nettest/nettest.h
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

#  undef HTONS
#  undef HTONL
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
extern uint16_t g_nettestserver_ipv6[8];
#else
extern uint32_t g_nettestserver_ipv4;
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
