/****************************************************************************
 * apps/testing/nettest/udp/test_udp_epoll.c
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
#include <sys/epoll.h>
#include <sys/socket.h>

#include "test_udp.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_UDP_EPOLL_WAIT   1000  /* ms, time for an event that must come */
#define TEST_UDP_EPOLL_ROUNDS 5

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_udp_epoll_out
 *
 * Description:
 *   An epoll set, kept in 'state', with the socket 'sd' for
 *   EPOLLOUT | EPOLLET.  The first edge (the socket is writable) is taken.
 *
 ****************************************************************************/

static int test_udp_epoll_out(FAR void **state, int sd)
{
  FAR struct test_udp_state_s *udp_state = *state;
  struct epoll_event ev;
  int epfd;

  assert_true(udp_state->nsd < TEST_UDP_NSOCKS);
  epfd = epoll_create1(EPOLL_CLOEXEC);
  assert_return_code(epfd, errno);
  udp_state->sd[udp_state->nsd++] = epfd;

  memset(&ev, 0, sizeof(ev));
  ev.events = EPOLLOUT | EPOLLET;
  assert_return_code(epoll_ctl(epfd, EPOLL_CTL_ADD, sd, &ev), errno);
  assert_int_equal(epoll_wait(epfd, &ev, 1, TEST_UDP_EPOLL_WAIT), 1);
  assert_true((ev.events & EPOLLOUT) != 0);
  return epfd;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_udp_epollet_idle
 *
 * Description:
 *   An idle EPOLLOUT | EPOLLET socket is reported once.  Datagrams that
 *   other sockets send make the device poll every UDP socket, and this is
 *   no event.
 *
 ****************************************************************************/

void test_udp_epollet_idle(FAR void **state)
{
#ifndef CONFIG_NET_UDP_WRITE_BUFFERS
  struct epoll_event ev;
  int port = TEST_UDP_PORT + 30;
  int epfd;
  int err;
  int sd;
  int i;

  sd = test_udp_socket(state, AF_INET, SOCK_DGRAM, -1, false);
  assert_return_code(test_udp_bind(sd, AF_INET, port, &err), err);
  epfd = test_udp_epoll_out(state, sd);

  for (i = 0; i < TEST_UDP_EPOLL_ROUNDS; i++)
    {
      test_udp_send4(port + 1);
      assert_int_equal(epoll_wait(epfd, &ev, 1, 0), 0);
    }
#else
  /* With write buffers a send can fail with EAGAIN: each poll of the
   * device reports POLLOUT, so that no edge is lost.
   */

  skip();
#endif
}

/****************************************************************************
 * Name: test_udp_epollet_send
 *
 * Description:
 *   An EPOLLOUT | EPOLLET socket is reported again when it sends a
 *   datagram.
 *
 ****************************************************************************/

void test_udp_epollet_send(FAR void **state)
{
  struct epoll_event ev;
  struct sockaddr_in addr;
  int port = TEST_UDP_PORT + 32;
  int epfd;
  int err;
  int sd;
  int i;

  sd = test_udp_socket(state, AF_INET, SOCK_DGRAM, -1, false);
  assert_return_code(test_udp_bind(sd, AF_INET, port, &err), err);
  epfd = test_udp_epoll_out(state, sd);

  memset(&addr, 0, sizeof(addr));
  addr.sin_family      = AF_INET;
  addr.sin_port        = htons(port + 1);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  for (i = 0; i < TEST_UDP_EPOLL_ROUNDS; i++)
    {
      assert_int_equal(sendto(sd, "et", 2, 0, (FAR struct sockaddr *)&addr,
                              sizeof(addr)), 2);
      assert_int_equal(epoll_wait(epfd, &ev, 1, TEST_UDP_EPOLL_WAIT), 1);
      assert_true((ev.events & EPOLLOUT) != 0);
    }
}
