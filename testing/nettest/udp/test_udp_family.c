/****************************************************************************
 * apps/testing/nettest/udp/test_udp_family.c
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
 * Name: test_udp_family_alone
 *
 * Description:
 *   An IPv4 socket alone on its port gets no IPv6 datagram.
 *
 ****************************************************************************/

void test_udp_family_alone(FAR void **state)
{
  struct sockaddr_in6 from;
  int port = TEST_UDP_PORT + 10;
  int ret;
  int err;
  int s4;

  s4  = test_udp_socket(state, AF_INET, SOCK_DGRAM, -1, false);
  ret = test_udp_bind(s4, AF_INET, port, &err);
  assert_return_code(ret, err);
  test_udp_send6(port);
  ret = test_udp_recv(s4, &from);
  assert_int_equal(ret, -EAGAIN);
}

/****************************************************************************
 * Name: test_udp_family_both
 *
 * Description:
 *   An IPv4 socket, then an IPv6 socket with IPV6_V6ONLY on the same port:
 *   the IPv6 datagram goes to the IPv6 socket only.
 *
 ****************************************************************************/

void test_udp_family_both(FAR void **state)
{
  struct sockaddr_in6 from;
  int port = TEST_UDP_PORT + 11;
  int r4;
  int r6;
  int s4;
  int s6;
  int err;

  s4 = test_udp_socket(state, AF_INET, SOCK_DGRAM, -1, false);
  r4 = test_udp_bind(s4, AF_INET, port, &err);
  assert_return_code(r4, err);
  s6 = test_udp_socket(state, AF_INET6, SOCK_DGRAM, 1, false);
  r6 = test_udp_bind(s6, AF_INET6, port, &err);
  assert_return_code(r6, err);
  test_udp_send6(port);
  r6 = test_udp_recv(s6, &from);
  r4 = test_udp_recv(s4, &from);
  assert_int_equal(r4, -EAGAIN);
  assert_int_equal(r6, 2);
}

/****************************************************************************
 * Name: test_udp_family_v6addr
 *
 * Description:
 *   A UDP socket of AF_INET6 on [::1]:p gets no IPv4 datagram to
 *   127.0.0.1:p: ::1 is not an IPv4 address.
 *
 ****************************************************************************/

void test_udp_family_v6addr(FAR void **state)
{
  struct sockaddr_in6 from;
  struct sockaddr_in6 addr;
  int port = TEST_UDP_PORT + 12;
  int ret;
  int err;
  int s6;

  s6 = test_udp_socket(state, AF_INET6, SOCK_DGRAM, -1, false);
  memset(&addr, 0, sizeof(addr));
  addr.sin6_family = AF_INET6;
  addr.sin6_port   = htons(port);
  addr.sin6_addr   = in6addr_loopback;
  ret = bind(s6, (FAR struct sockaddr *)&addr, sizeof(addr));
  err = ret < 0 ? errno : 0;
  assert_return_code(ret, err);
  test_udp_send4(port);
  ret = test_udp_recv(s6, &from);
  assert_int_equal(ret, -EAGAIN);
}

/****************************************************************************
 * Name: test_udp_family_connect
 *
 * Description:
 *   connect() to [::]:p connects to ::1, as connect() to 0.0.0.0 connects
 *   to 127.0.0.1: a UDP socket of AF_INET6 on [::]:p gets the datagram.
 *
 ****************************************************************************/

void test_udp_family_connect(FAR void **state)
{
  struct sockaddr_in6 from;
  struct sockaddr_in6 addr;
  int port = TEST_UDP_PORT + 13;
  int ret;
  int err;
  int s6;
  int sd;

  s6  = test_udp_socket(state, AF_INET6, SOCK_DGRAM, -1, false);
  ret = test_udp_bind(s6, AF_INET6, port, &err);
  assert_return_code(ret, err);

  sd = test_udp_socket(state, AF_INET6, SOCK_DGRAM, -1, false);
  memset(&addr, 0, sizeof(addr));
  addr.sin6_family = AF_INET6;
  addr.sin6_port   = htons(port);
  addr.sin6_addr   = in6addr_any;
  ret = connect(sd, (FAR struct sockaddr *)&addr, sizeof(addr));
  assert_return_code(ret, errno);
  ret = send(sd, "v6", 2, 0);
  assert_return_code(ret, errno);
  usleep(TEST_UDP_WAIT_US);
  ret = test_udp_recv(s6, &from);
  assert_int_equal(ret, 2);
  assert_true(IN6_IS_ADDR_LOOPBACK(&from.sin6_addr));
}

/****************************************************************************
 * Name: test_udp_family_sendto6
 *
 * Description:
 *   sendto() to [::]:p on a socket that is not connected sends to ::1.
 *
 ****************************************************************************/

void test_udp_family_sendto6(FAR void **state)
{
  struct sockaddr_in6 from;
  struct sockaddr_in6 addr;
  int port = TEST_UDP_PORT + 14;
  int ret;
  int err;
  int s6;
  int sd;

  s6  = test_udp_socket(state, AF_INET6, SOCK_DGRAM, -1, false);
  ret = test_udp_bind(s6, AF_INET6, port, &err);
  assert_return_code(ret, err);

  sd = test_udp_socket(state, AF_INET6, SOCK_DGRAM, -1, false);
  memset(&addr, 0, sizeof(addr));
  addr.sin6_family = AF_INET6;
  addr.sin6_port   = htons(port);
  addr.sin6_addr   = in6addr_any;
  ret = sendto(sd, "v6", 2, 0, (FAR struct sockaddr *)&addr, sizeof(addr));
  assert_return_code(ret, errno);
  usleep(TEST_UDP_WAIT_US);
  ret = test_udp_recv(s6, &from);
  assert_int_equal(ret, 2);
  assert_true(IN6_IS_ADDR_LOOPBACK(&from.sin6_addr));
}

/****************************************************************************
 * Name: test_udp_family_sendto4
 *
 * Description:
 *   sendto() to 0.0.0.0:p on a socket that is not connected sends to
 *   127.0.0.1.
 *
 ****************************************************************************/

void test_udp_family_sendto4(FAR void **state)
{
  struct sockaddr_in6 from;
  struct sockaddr_in addr;
  int port = TEST_UDP_PORT + 15;
  int ret;
  int err;
  int s4;
  int sd;

  s4  = test_udp_socket(state, AF_INET, SOCK_DGRAM, -1, false);
  ret = test_udp_bind(s4, AF_INET, port, &err);
  assert_return_code(ret, err);

  sd = test_udp_socket(state, AF_INET, SOCK_DGRAM, -1, false);
  memset(&addr, 0, sizeof(addr));
  addr.sin_family      = AF_INET;
  addr.sin_port        = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  ret = sendto(sd, "v4", 2, 0, (FAR struct sockaddr *)&addr, sizeof(addr));
  assert_return_code(ret, errno);
  usleep(TEST_UDP_WAIT_US);
  ret = test_udp_recv(s4, &from);
  assert_int_equal(ret, 2);
}
