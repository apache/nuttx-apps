/****************************************************************************
 * apps/testing/nettest/udp/test_udp_bind.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <cmocka.h>
#include <sys/socket.h>

#include "test_udp.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_udp_bind_v6_v4
 *
 * Description:
 *   A dual-stack IPv6 socket on [::]:p, then an IPv4 socket on p: the
 *   second bind() fails with EADDRINUSE.
 *
 ****************************************************************************/

void test_udp_bind_v6_v4(FAR void **state)
{
  int port = TEST_UDP_PORT + 20;
  int r6;
  int r4;
  int s6;
  int s4;
  int err;

  s6 = test_udp_socket(state, AF_INET6, SOCK_DGRAM, -1, false);
  r6 = test_udp_bind(s6, AF_INET6, port, &err);
  assert_return_code(r6, err);
  s4 = test_udp_socket(state, AF_INET, SOCK_DGRAM, -1, false);
  r4 = test_udp_bind(s4, AF_INET, port, &err);
  assert_int_equal(r4, -1);
  assert_int_equal(err, EADDRINUSE);
}

/****************************************************************************
 * Name: test_udp_bind_v4_v6
 *
 * Description:
 *   An IPv4 socket on p, then a dual-stack IPv6 socket on [::]:p: the
 *   second bind() fails with EADDRINUSE.
 *
 ****************************************************************************/

void test_udp_bind_v4_v6(FAR void **state)
{
  int port = TEST_UDP_PORT + 21;
  int r6;
  int r4;
  int s6;
  int s4;
  int err;

  s4 = test_udp_socket(state, AF_INET, SOCK_DGRAM, -1, false);
  r4 = test_udp_bind(s4, AF_INET, port, &err);
  assert_return_code(r4, err);
  s6 = test_udp_socket(state, AF_INET6, SOCK_DGRAM, -1, false);
  r6 = test_udp_bind(s6, AF_INET6, port, &err);
  assert_int_equal(r6, -1);
  assert_int_equal(err, EADDRINUSE);
}

/****************************************************************************
 * Name: test_udp_bind_v6only_v4
 *
 * Description:
 *   With IPV6_V6ONLY on the IPv6 socket, both binds succeed (IPv6 first).
 *
 ****************************************************************************/

void test_udp_bind_v6only_v4(FAR void **state)
{
  int port = TEST_UDP_PORT + 22;
  int r6;
  int r4;
  int s6;
  int s4;
  int err;

  s6 = test_udp_socket(state, AF_INET6, SOCK_DGRAM, 1, false);
  r6 = test_udp_bind(s6, AF_INET6, port, &err);
  assert_return_code(r6, err);
  s4 = test_udp_socket(state, AF_INET, SOCK_DGRAM, -1, false);
  r4 = test_udp_bind(s4, AF_INET, port, &err);
  assert_return_code(r4, err);
}

/****************************************************************************
 * Name: test_udp_bind_v4_v6only
 *
 * Description:
 *   With IPV6_V6ONLY on the IPv6 socket, both binds succeed (IPv4 first).
 *
 ****************************************************************************/

void test_udp_bind_v4_v6only(FAR void **state)
{
  int port = TEST_UDP_PORT + 23;
  int r6;
  int r4;
  int s6;
  int s4;
  int err;

  s4 = test_udp_socket(state, AF_INET, SOCK_DGRAM, -1, false);
  r4 = test_udp_bind(s4, AF_INET, port, &err);
  assert_return_code(r4, err);
  s6 = test_udp_socket(state, AF_INET6, SOCK_DGRAM, 1, false);
  r6 = test_udp_bind(s6, AF_INET6, port, &err);
  assert_return_code(r6, err);
}

/****************************************************************************
 * Name: test_udp_bind_reuse
 *
 * Description:
 *   With SO_REUSEADDR on both, a dual-stack IPv6 socket and an IPv4 socket
 *   bind the same port, in both orders.
 *
 ****************************************************************************/

void test_udp_bind_reuse(FAR void **state)
{
  int port = TEST_UDP_PORT + 24;
  int r6;
  int r4;
  int s6;
  int s4;
  int err;

  s6 = test_udp_socket(state, AF_INET6, SOCK_DGRAM, -1, true);
  r6 = test_udp_bind(s6, AF_INET6, port, &err);
  assert_return_code(r6, err);
  s4 = test_udp_socket(state, AF_INET, SOCK_DGRAM, -1, true);
  r4 = test_udp_bind(s4, AF_INET, port, &err);
  assert_return_code(r4, err);

  s4 = test_udp_socket(state, AF_INET, SOCK_DGRAM, -1, true);
  r4 = test_udp_bind(s4, AF_INET, port + 1, &err);
  assert_return_code(r4, err);
  s6 = test_udp_socket(state, AF_INET6, SOCK_DGRAM, -1, true);
  r6 = test_udp_bind(s6, AF_INET6, port + 1, &err);
  assert_return_code(r6, err);
}

/****************************************************************************
 * Name: test_udp_bind_v6addr
 *
 * Description:
 *   An IPv4 socket on 0.0.0.0:p, then a dual-stack IPv6 socket on [::1]:p:
 *   ::1 takes no IPv4 traffic, so both binds succeed.
 *
 ****************************************************************************/

void test_udp_bind_v6addr(FAR void **state)
{
  struct sockaddr_in6 addr;
  int port = TEST_UDP_PORT + 26;
  int r6;
  int r4;
  int s6;
  int s4;
  int err;

  s4 = test_udp_socket(state, AF_INET, SOCK_DGRAM, -1, false);
  r4 = test_udp_bind(s4, AF_INET, port, &err);
  assert_return_code(r4, err);

  s6 = test_udp_socket(state, AF_INET6, SOCK_DGRAM, -1, false);
  memset(&addr, 0, sizeof(addr));
  addr.sin6_family = AF_INET6;
  addr.sin6_port   = htons(port);
  addr.sin6_addr   = in6addr_loopback;
  r6  = bind(s6, (FAR struct sockaddr *)&addr, sizeof(addr));
  err = r6 < 0 ? errno : 0;
  assert_return_code(r6, err);
}
