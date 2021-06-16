/****************************************************************************
 * apps/examples/tcpblaster/tcpblaster.h
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
/* HTONS/L macros are unique to NuttX */

#  undef HTONS
#  undef HTONL
#  define HTONS(ns) htons(ns)
#  define HTONL(nl) htonl(nl)

#  undef NTOHS
#  undef NTOHL
#  define NTOHS(hs) ntohs(hs)
#  define NTOHL(hl) ntohl(hl)

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
