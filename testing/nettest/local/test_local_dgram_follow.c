/****************************************************************************
 * apps/testing/nettest/local/test_local_dgram_follow.c
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

/* Behaviour wider than connect() and poll() of a datagram socket.  Each case
 * fails without a change of its own, with or without connect() and poll().
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <poll.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <cmocka.h>

#include "test_local_dgram.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Wait for one datagram with recv() only */

static void local_dgram_recvstr(int fd, FAR const char *want)
{
  char buf[128];
  ssize_t n;

  n = recv(fd, buf, sizeof(buf) - 1, 0);
  assert_return_code(n, errno);
  buf[n] = '\0';
  assert_string_equal(buf, want);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_local_dgram_rebind
 *
 * Description:
 *   A name can be bound again after close(), and the new socket does not get
 *   the datagrams of the old socket.
 *
 ****************************************************************************/

void test_local_dgram_rebind(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  char big[16];

  assert_int_equal(local_dgram_sendto(st->s, st->p1, "old", 3, 0), 3);
  assert_int_equal(close(st->r1), 0);
  st->r1 = local_dgram_bind(st->p1, false);
  assert_return_code(st->r1, errno);

  errno = 0;
  assert_int_equal(recv(st->r1, big, sizeof(big), MSG_DONTWAIT), -1);
  assert_true(errno == EAGAIN || errno == EWOULDBLOCK);
  assert_int_equal(local_dgram_sendto(st->s, st->p1, "new", 3, 0), 3);
  local_dgram_recvstr(st->r1, "new");
}

/****************************************************************************
 * Name: test_local_dgram_source
 *
 * Description:
 *   recvfrom() gives the whole name of a bound sender, short or long.
 *
 ****************************************************************************/

void test_local_dgram_source(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  FAR const char *tail[2];
  struct sockaddr_un addr;
  socklen_t addrlen;
  char name[LOCAL_DGRAM_PATHLEN];
  char buf[16];
  ssize_t ret;
  int i;

  tail[0] = "s";
  tail[1] = "a-socket-with-a-long-name";
  for (i = 0; i < 2; i++)
    {
      snprintf(name, sizeof(name), "%s/%s", g_local_dgram_dir, tail[i]);
      st->r2 = local_dgram_bind(name, false);
      assert_return_code(st->r2, errno);
      assert_int_equal(local_dgram_sendto(st->r2, st->p1, "b", 1, 0), 1);
      memset(&addr, 0, sizeof(addr));
      addrlen = sizeof(addr);
      ret = recvfrom(st->r1, buf, sizeof(buf), 0,
                     (FAR struct sockaddr *)&addr, &addrlen);
      close(st->r2);
      st->r2 = -1;
      unlink(name);
      assert_int_equal(ret, 1);
      assert_string_equal(addr.sun_path, name);
    }
}

/****************************************************************************
 * Name: test_local_dgram_recvmsg_iov
 *
 * Description:
 *   recvmsg() of a datagram socket reads into two parts.
 *
 ****************************************************************************/

void test_local_dgram_recvmsg_iov(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct msghdr msg;
  struct iovec iov[2];
  char part1[3];
  char part2[3];
  ssize_t ret;

  assert_int_equal(local_dgram_sendto(st->s, st->p1, "abcd", 4, 0), 4);

  memset(part1, 0, sizeof(part1));
  memset(part2, 0, sizeof(part2));
  iov[0].iov_base = part1;
  iov[0].iov_len  = 2;
  iov[1].iov_base = part2;
  iov[1].iov_len  = 2;
  memset(&msg, 0, sizeof(msg));
  msg.msg_iov    = iov;
  msg.msg_iovlen = 2;
  ret = recvmsg(st->r1, &msg, 0);
  assert_int_equal(ret, 4);
  assert_string_equal(part1, "ab");
  assert_string_equal(part2, "cd");
}

/****************************************************************************
 * Name: test_local_dgram_poll_nval
 *
 * Description:
 *   poll() of a closed descriptor gives POLLNVAL for it and still reports
 *   the others.
 *
 ****************************************************************************/

void test_local_dgram_poll_nval(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct pollfd pfd[2];
  int closed;
  int ret;

  closed = socket(AF_UNIX, SOCK_DGRAM, 0);
  assert_return_code(closed, errno);
  close(closed);

  memset(pfd, 0, sizeof(pfd));
  pfd[0].fd     = closed;
  pfd[0].events = POLLIN;
  pfd[1].fd     = st->s;
  pfd[1].events = POLLOUT;

  ret = poll(pfd, 2, 0);
  assert_return_code(ret, errno);
  assert_int_equal(ret, 2);
  assert_int_equal(pfd[0].revents, POLLNVAL);
  assert_int_equal(pfd[1].revents, POLLOUT);
}

/****************************************************************************
 * Name: test_local_dgram_poll_zero
 *
 * Description:
 *   A datagram with no data is a datagram: POLLIN, then recv() returns 0.
 *
 ****************************************************************************/

void test_local_dgram_poll_zero(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  char buf[16];

  assert_int_equal(send(st->s, "", 0, 0), 0);
  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 1000), POLLIN);
  assert_int_equal(recv(st->r1, buf, sizeof(buf), 0), 0);
  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 0), 0);

  assert_int_equal(send(st->s, "next", 4, 0), 4);
  local_dgram_expect(st->r1, "next");
}
