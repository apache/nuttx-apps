/****************************************************************************
 * apps/testing/nettest/udp/test_udp_v6only.c
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
 * Name: test_udp_v6only_default
 ****************************************************************************/

void test_udp_v6only_default(FAR void **state)
{
  int sd;

  sd = test_udp_socket(state, AF_INET6, SOCK_DGRAM, -1, false);
  assert_int_equal(test_udp_get_v6only(sd), 0);
}

/****************************************************************************
 * Name: test_udp_v6only_mapped
 *
 * Description:
 *   Without the option, an IPv6 socket on [::] gets an IPv4 datagram from
 *   an IPv4-mapped address.
 *
 ****************************************************************************/

void test_udp_v6only_mapped(FAR void **state)
{
  struct sockaddr_in6 from;
  int port = TEST_UDP_PORT;
  int ret;
  int err;
  int sd;

  sd = test_udp_socket(state, AF_INET6, SOCK_DGRAM, -1, false);
  ret = test_udp_bind(sd, AF_INET6, port, &err);
  assert_return_code(ret, err);
  test_udp_send4(port);
  ret = test_udp_recv(sd, &from);
  assert_int_equal(ret, 2);
  assert_true(IN6_IS_ADDR_V4MAPPED(&from.sin6_addr));
}

/****************************************************************************
 * Name: test_udp_v6only_set0
 *
 * Description:
 *   Set 1, then 0 before bind(): the socket gets IPv4 datagrams.
 *
 ****************************************************************************/

void test_udp_v6only_set0(FAR void **state)
{
  struct sockaddr_in6 from;
  int port = TEST_UDP_PORT + 1;
  int ret;
  int err;
  int sd;

  sd = test_udp_socket(state, AF_INET6, SOCK_DGRAM, 1, false);
  ret = test_udp_set_v6only(sd, 0);
  assert_return_code(ret, errno);
  assert_int_equal(test_udp_get_v6only(sd), 0);
  ret = test_udp_bind(sd, AF_INET6, port, &err);
  assert_return_code(ret, err);
  test_udp_send4(port);
  ret = test_udp_recv(sd, &from);
  assert_int_equal(ret, 2);
}

/****************************************************************************
 * Name: test_udp_v6only_set1
 *
 * Description:
 *   With the option, an IPv6 socket on [::] gets no IPv4 datagram.
 *
 ****************************************************************************/

void test_udp_v6only_set1(FAR void **state)
{
  struct sockaddr_in6 from;
  int port = TEST_UDP_PORT + 2;
  int ret;
  int err;
  int sd;

  sd = test_udp_socket(state, AF_INET6, SOCK_DGRAM, 1, false);
  assert_int_equal(test_udp_get_v6only(sd), 1);
  ret = test_udp_bind(sd, AF_INET6, port, &err);
  assert_return_code(ret, err);
  test_udp_send4(port);
  ret = test_udp_recv(sd, &from);
  assert_int_equal(ret, -EAGAIN);
}

/****************************************************************************
 * Name: test_udp_v6only_both
 *
 * Description:
 *   An IPv6 socket with the option, then an IPv4 socket on the same port:
 *   the IPv4 datagram goes to the IPv4 socket.
 *
 ****************************************************************************/

void test_udp_v6only_both(FAR void **state)
{
  struct sockaddr_in6 from;
  int port = TEST_UDP_PORT + 3;
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
  test_udp_send4(port);
  r6 = test_udp_recv(s6, &from);
  r4 = test_udp_recv(s4, &from);
  assert_int_equal(r6, -EAGAIN);
  assert_int_equal(r4, 2);
}

/****************************************************************************
 * Name: test_udp_v6only_send
 *
 * Description:
 *   With the option, sendto() and connect() to ::ffff:127.0.0.1 fail with
 *   ENETUNREACH.
 *
 ****************************************************************************/

void test_udp_v6only_send(FAR void **state)
{
  struct sockaddr_in6 addr;
  int ret;
  int err;
  int sd;

  sd = test_udp_socket(state, AF_INET6, SOCK_DGRAM, 1, false);

  memset(&addr, 0, sizeof(addr));
  addr.sin6_family           = AF_INET6;
  addr.sin6_port             = htons(TEST_UDP_PORT + 4);
  addr.sin6_addr.s6_addr[10] = 0xff;
  addr.sin6_addr.s6_addr[11] = 0xff;
  addr.sin6_addr.s6_addr[12] = 127;
  addr.sin6_addr.s6_addr[15] = 1;

  ret = sendto(sd, "v6", 2, 0, (FAR struct sockaddr *)&addr, sizeof(addr));
  err = ret < 0 ? errno : 0;
  assert_int_equal(ret, -1);
  assert_int_equal(err, ENETUNREACH);

  ret = connect(sd, (FAR struct sockaddr *)&addr, sizeof(addr));
  err = ret < 0 ? errno : 0;
  assert_int_equal(ret, -1);
  assert_int_equal(err, ENETUNREACH);
}

/****************************************************************************
 * Name: test_udp_v6only_bound
 *
 * Description:
 *   After bind(), setting the option fails with EINVAL.
 *
 ****************************************************************************/

void test_udp_v6only_bound(FAR void **state)
{
  int ret;
  int err;
  int sd;

  sd = test_udp_socket(state, AF_INET6, SOCK_DGRAM, -1, false);
  ret = test_udp_bind(sd, AF_INET6, TEST_UDP_PORT + 5, &err);
  assert_return_code(ret, err);
  ret = test_udp_set_v6only(sd, 1);
  err = ret < 0 ? errno : 0;
  assert_int_equal(ret, -1);
  assert_int_equal(err, EINVAL);
}

#if defined(CONFIG_NET_TCP) && defined(CONFIG_NET_TCPBACKLOG)
/****************************************************************************
 * Name: test_udp_v6only_tcp_set1
 ****************************************************************************/

void test_udp_v6only_tcp_set1(FAR void **state)
{
  int sd;

  sd = test_udp_socket(state, AF_INET6, SOCK_STREAM, 1, false);
  assert_int_equal(test_udp_get_v6only(sd), 1);
}

/****************************************************************************
 * Name: test_udp_v6only_tcp_set0
 *
 * Description:
 *   A TCP socket of AF_INET6 is always IPv6-only: 0 fails, get gives 1.
 *
 ****************************************************************************/

void test_udp_v6only_tcp_set0(FAR void **state)
{
  int ret;
  int err;
  int sd;

  sd = test_udp_socket(state, AF_INET6, SOCK_STREAM, -1, false);
  ret = test_udp_set_v6only(sd, 0);
  err = ret < 0 ? errno : 0;
  assert_int_equal(ret, -1);
  assert_int_equal(err, ENOPROTOOPT);
  assert_int_equal(test_udp_get_v6only(sd), 1);
}

/****************************************************************************
 * Name: test_udp_v6only_tcp_ipv4
 *
 * Description:
 *   An IPv6 listener gets no IPv4 connection.
 *
 ****************************************************************************/

void test_udp_v6only_tcp_ipv4(FAR void **state)
{
  struct sockaddr_in addr;
  int port = TEST_UDP_PORT + 6;
  int ret;
  int err;
  int ls;
  int cs;

  ls  = test_udp_socket(state, AF_INET6, SOCK_STREAM, -1, false);
  ret = test_udp_bind(ls, AF_INET6, port, &err);
  assert_return_code(ret, err);
  ret = listen(ls, 2);
  assert_return_code(ret, errno);

  cs = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
  assert_return_code(cs, errno);
  memset(&addr, 0, sizeof(addr));
  addr.sin_family      = AF_INET;
  addr.sin_port        = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  ret = connect(cs, (FAR struct sockaddr *)&addr, sizeof(addr));
  close(cs);
  assert_int_equal(ret, -1);
}

/****************************************************************************
 * Name: test_udp_v6only_tcp_bound
 *
 * Description:
 *   After bind(), setting the option on a TCP socket fails with EINVAL.
 *
 ****************************************************************************/

void test_udp_v6only_tcp_bound(FAR void **state)
{
  int ret;
  int err;
  int sd;

  sd  = test_udp_socket(state, AF_INET6, SOCK_STREAM, -1, false);
  ret = test_udp_bind(sd, AF_INET6, TEST_UDP_PORT + 7, &err);
  assert_return_code(ret, err);
  ret = test_udp_set_v6only(sd, 1);
  err = ret < 0 ? errno : 0;
  assert_int_equal(ret, -1);
  assert_int_equal(err, EINVAL);
}
#endif
