/****************************************************************************
 * apps/examples/udp/udp.h
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
/* HTONS/L macros are unique to NuttX */

#  undef HTONS
#  undef HTONL
#  define HTONS(ns) htons(ns)
#  define HTONL(nl) htonl(nl)

#  undef NTOHS
#  undef NTOHL
#  define NTOHS(hs) ntohs(hs)
#  define NTOHL(hl) ntohl(hl)

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
extern uint16_t g_udpserver_ipv6[8];
#else
extern uint32_t g_udpserver_ipv4;
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
