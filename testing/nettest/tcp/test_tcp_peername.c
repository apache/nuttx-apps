/****************************************************************************
 * apps/testing/nettest/tcp/test_tcp_peername.c
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
#include <netinet/in.h>
#include <poll.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <cmocka.h>

#include "test_tcp.h"
#include "utils.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_POLL 1000  /* ms, loopback answers at once */

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct tcp_peername_state_s
{
  int fds[3];
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_tcp_peername_setup
 ****************************************************************************/

int test_tcp_peername_setup(FAR void **state)
{
  FAR struct tcp_peername_state_s *st = malloc(sizeof(*st));

  assert_non_null(st);
  st->fds[0] = -1;
  st->fds[1] = -1;
  st->fds[2] = -1;
  *state = st;
  return 0;
}

/****************************************************************************
 * Name: test_tcp_peername_teardown
 ****************************************************************************/

int test_tcp_peername_teardown(FAR void **state)
{
  FAR struct tcp_peername_state_s *st = *state;
  int i;

  for (i = 0; i < 3; i++)
    {
      if (st->fds[i] >= 0)
        {
          close(st->fds[i]);
        }
    }

  free(st);
  return 0;
}

/****************************************************************************
 * Name: test_tcp_peername_ipv6
 *
 * Description:
 *   On a connection over ::1, getpeername() of each end gives the address
 *   and the port that getsockname() of the other end gives.
 *
 ****************************************************************************/

void test_tcp_peername_ipv6(FAR void **state)
{
  FAR struct tcp_peername_state_s *st = *state;
  struct sockaddr_in6 listen_addr;
  struct sockaddr_in6 local;
  struct sockaddr_in6 peer;
  socklen_t addrlen;
  struct pollfd pfd;

  st->fds[0] = socket(AF_INET6, SOCK_STREAM | SOCK_CLOEXEC, 0);
  assert_return_code(st->fds[0], errno);
  nettest_lo_addr((FAR struct sockaddr *)&listen_addr, AF_INET6);
  listen_addr.sin6_port = 0;
  assert_return_code(bind(st->fds[0], (FAR struct sockaddr *)&listen_addr,
                          sizeof(listen_addr)), errno);
  assert_return_code(listen(st->fds[0], 1), errno);
  addrlen = sizeof(listen_addr);
  assert_return_code(getsockname(st->fds[0],
                                 (FAR struct sockaddr *)&listen_addr,
                                 &addrlen), errno);

  st->fds[1] = socket(AF_INET6, SOCK_STREAM | SOCK_CLOEXEC, 0);
  assert_return_code(st->fds[1], errno);
  assert_return_code(connect(st->fds[1],
                             (FAR struct sockaddr *)&listen_addr,
                             sizeof(listen_addr)), errno);
  pfd.fd      = st->fds[0];
  pfd.events  = POLLIN;
  pfd.revents = 0;
  assert_int_equal(poll(&pfd, 1, TEST_POLL), 1);
  st->fds[2] = accept4(st->fds[0], NULL, NULL, SOCK_CLOEXEC);
  assert_return_code(st->fds[2], errno);

  /* The client: its peer is the listener's address and port */

  addrlen = sizeof(local);
  assert_return_code(getsockname(st->fds[1], (FAR struct sockaddr *)&local,
                                 &addrlen), errno);
  addrlen = sizeof(peer);
  memset(&peer, 0, sizeof(peer));
  assert_return_code(getpeername(st->fds[1], (FAR struct sockaddr *)&peer,
                                 &addrlen), errno);
  assert_int_equal(addrlen, sizeof(peer));
  assert_int_equal(peer.sin6_family, AF_INET6);
  assert_true(IN6_IS_ADDR_LOOPBACK(&peer.sin6_addr));
  assert_int_equal(ntohs(peer.sin6_port), ntohs(listen_addr.sin6_port));
  assert_int_not_equal(ntohs(peer.sin6_port), ntohs(local.sin6_port));

  /* The accepted end: its peer is the client's port */

  addrlen = sizeof(peer);
  memset(&peer, 0, sizeof(peer));
  assert_return_code(getpeername(st->fds[2], (FAR struct sockaddr *)&peer,
                                 &addrlen), errno);
  assert_true(IN6_IS_ADDR_LOOPBACK(&peer.sin6_addr));
  assert_int_equal(ntohs(peer.sin6_port), ntohs(local.sin6_port));
}
