/****************************************************************************
 * apps/testing/nettest/udp/test_udp.h
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

#ifndef __APPS_TESTING_NETTEST_UDP_TEST_UDP_H
#define __APPS_TESTING_NETTEST_UDP_TEST_UDP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <netinet/in.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_UDP_PORT    5100          /* First port; each test has its own */
#define TEST_UDP_WAIT_US (200 * 1000)  /* Time for lo to deliver */
#define TEST_UDP_NSOCKS  4             /* Sockets of one test */

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* The sockets of one test, closed by test_udp_teardown() also when an
 * assertion ends the test.
 */

struct test_udp_state_s
{
  int sd[TEST_UDP_NSOCKS];
  int nsd;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: test_udp_setup, test_udp_teardown
 ****************************************************************************/

int test_udp_setup(FAR void **state);
int test_udp_teardown(FAR void **state);

/****************************************************************************
 * Name: test_udp_set_v6only, test_udp_get_v6only
 *
 * Description:
 *   Set IPV6_V6ONLY (the result of setsockopt()), or read it (the value,
 *   or -errno).
 *
 ****************************************************************************/

int test_udp_set_v6only(int sd, int val);
int test_udp_get_v6only(int sd);

/****************************************************************************
 * Name: test_udp_socket, test_udp_bind
 *
 * Description:
 *   A socket of 'domain' and 'type', kept in 'state'.  test_udp_socket()
 *   sets IPV6_V6ONLY to 'v6only' (if >= 0) and SO_REUSEADDR (if 'reuse').
 *   test_udp_bind()
 *   binds it to the wildcard address and 'port' and gives the result of
 *   bind(), with errno in 'err'.
 *
 ****************************************************************************/

int test_udp_socket(FAR void **state, int domain, int type, int v6only,
                    bool reuse);
int test_udp_bind(int sd, int domain, int port, FAR int *err);

/****************************************************************************
 * Name: test_udp_send4, test_udp_send6, test_udp_recv
 *
 * Description:
 *   Send one datagram to 127.0.0.1 or ::1 at 'port' and let lo deliver
 *   it.  Read one datagram without waiting: its size, or -errno.
 *
 ****************************************************************************/

void test_udp_send4(int port);
void test_udp_send6(int port);
int test_udp_recv(int sd, FAR struct sockaddr_in6 *from);

/****************************************************************************
 * Name: test_udp_v6only_*
 *
 * Description:
 *   IPV6_V6ONLY on UDP and TCP sockets of AF_INET6.
 *
 ****************************************************************************/

void test_udp_v6only_default(FAR void **state);
void test_udp_v6only_mapped(FAR void **state);
void test_udp_v6only_set0(FAR void **state);
void test_udp_v6only_set1(FAR void **state);
void test_udp_v6only_both(FAR void **state);
void test_udp_v6only_send(FAR void **state);
void test_udp_v6only_bound(FAR void **state);
#if defined(CONFIG_NET_TCP) && defined(CONFIG_NET_TCPBACKLOG)
void test_udp_v6only_tcp_set1(FAR void **state);
void test_udp_v6only_tcp_set0(FAR void **state);
void test_udp_v6only_tcp_ipv4(FAR void **state);
void test_udp_v6only_tcp_bound(FAR void **state);
#endif

/****************************************************************************
 * Name: test_udp_family_*
 *
 * Description:
 *   A UDP socket of AF_INET gets no IPv6 datagram, a UDP socket of AF_INET6
 *   on ::1 gets no IPv4 datagram, and a datagram to [::] or 0.0.0.0 goes
 *   to the loopback address.
 *
 ****************************************************************************/

void test_udp_family_alone(FAR void **state);
void test_udp_family_both(FAR void **state);
void test_udp_family_v6addr(FAR void **state);
void test_udp_family_connect(FAR void **state);
void test_udp_family_sendto6(FAR void **state);
void test_udp_family_sendto4(FAR void **state);

/****************************************************************************
 * Name: test_udp_bind_*
 *
 * Description:
 *   A dual-stack UDP socket of AF_INET6 and a UDP socket of AF_INET cannot
 *   bind the same port, in either order, unless the IPv6 socket has
 *   IPV6_V6ONLY or both have SO_REUSEADDR.
 *
 ****************************************************************************/

void test_udp_bind_v6_v4(FAR void **state);
void test_udp_bind_v4_v6(FAR void **state);
void test_udp_bind_v6only_v4(FAR void **state);
void test_udp_bind_v4_v6only(FAR void **state);
void test_udp_bind_reuse(FAR void **state);
void test_udp_bind_v6addr(FAR void **state);

#endif /* __APPS_TESTING_NETTEST_UDP_TEST_UDP_H */
