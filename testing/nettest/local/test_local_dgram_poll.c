/****************************************************************************
 * apps/testing/nettest/local/test_local_dgram_poll.c
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
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>
#include <cmocka.h>

#include "test_local_dgram.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct local_dgram_later_s
{
  int fd;
  FAR const char *msg;
  size_t len;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Send one datagram after 200 ms */

static FAR void *local_dgram_send_later(FAR void *arg)
{
  FAR struct local_dgram_later_s *later = arg;

  usleep(200000);
  send(later->fd, later->msg, later->len, 0);
  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_local_dgram_pollin0
 *
 * Description:
 *   poll(POLLIN) of a receiver with no datagram reports no event.
 *
 ****************************************************************************/

void test_local_dgram_pollin0(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct pollfd pfd;
  int ret;

  pfd.fd      = st->r1;
  pfd.events  = POLLIN;
  pfd.revents = 0;
  ret = poll(&pfd, 1, 0);
  assert_return_code(ret, errno);
  assert_int_equal(ret, 0);
}

/****************************************************************************
 * Name: test_local_dgram_pollin
 *
 * Description:
 *   poll(POLLIN) after a send() reports POLLIN.
 *
 ****************************************************************************/

void test_local_dgram_pollin(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct pollfd pfd;
  int ret;

  ret = send(st->s, "four", 4, 0);
  assert_int_equal(ret, 4);

  pfd.fd      = st->r1;
  pfd.events  = POLLIN;
  pfd.revents = 0;
  ret = poll(&pfd, 1, 1000);
  assert_return_code(ret, errno);
  assert_int_equal(ret, 1);
  assert_true((pfd.revents & POLLIN) != 0);
  local_dgram_expect(st->r1, "four");
}

/****************************************************************************
 * Name: test_local_dgram_nohup
 *
 * Description:
 *   A datagram that is too large for the receiver does not make poll()
 *   of the receiver report POLLHUP.
 *
 ****************************************************************************/

void test_local_dgram_nohup(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_later_s later;
  struct pollfd pfd;
  pthread_t thread;
  int ret;

  later.fd  = st->s;
  later.msg = st->big;
  later.len = LOCAL_DGRAM_BIGLEN;
  ret = pthread_create(&thread, NULL, local_dgram_send_later, &later);
  assert_int_equal(ret, 0);

  pfd.fd      = st->r1;
  pfd.events  = POLLIN;
  pfd.revents = 0;
  ret = poll(&pfd, 1, 600);
  pthread_join(thread, NULL);
  local_dgram_drain(st->r1, st->big, LOCAL_DGRAM_BIGLEN);

  assert_return_code(ret, errno);
  assert_true((pfd.revents & POLLHUP) == 0);
}

/****************************************************************************
 * Name: test_local_dgram_pollout
 *
 * Description:
 *   poll(POLLOUT) of a connected sender reports POLLOUT.
 *
 ****************************************************************************/

void test_local_dgram_pollout(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct pollfd pfd;
  int ret;

  pfd.fd      = st->s;
  pfd.events  = POLLOUT;
  pfd.revents = 0;
  ret = poll(&pfd, 1, 0);
  assert_return_code(ret, errno);
  assert_int_equal(ret, 1);
  assert_true((pfd.revents & POLLOUT) != 0);
}

/****************************************************************************
 * Name: test_local_dgram_pollwait
 *
 * Description:
 *   poll(POLLIN) wakes up when another thread sends a datagram.
 *
 ****************************************************************************/

void test_local_dgram_pollwait(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_later_s later;
  struct pollfd pfd;
  struct timespec t0;
  struct timespec t1;
  pthread_t thread;
  long ms;
  int ret;

  later.fd  = st->s;
  later.msg = "five";
  later.len = 4;

  pfd.fd      = st->r1;
  pfd.events  = POLLIN;
  pfd.revents = 0;
  clock_gettime(CLOCK_MONOTONIC, &t0);
  ret = pthread_create(&thread, NULL, local_dgram_send_later, &later);
  assert_int_equal(ret, 0);

  ret = poll(&pfd, 1, 3000);
  clock_gettime(CLOCK_MONOTONIC, &t1);
  pthread_join(thread, NULL);

  ms = (t1.tv_sec - t0.tv_sec) * 1000 +
       (t1.tv_nsec - t0.tv_nsec) / 1000000;
  assert_return_code(ret, errno);
  assert_int_equal(ret, 1);
  assert_true((pfd.revents & POLLIN) != 0);
  assert_in_range(ms, 100, 2500);
  local_dgram_expect(st->r1, "five");
}

/****************************************************************************
 * Name: test_local_dgram_select
 *
 * Description:
 *   select() reports the receiver readable after a send().
 *
 ****************************************************************************/

void test_local_dgram_select(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct timeval tv;
  fd_set rset;
  int ret;

  ret = send(st->s, "six", 3, 0);
  assert_int_equal(ret, 3);

  FD_ZERO(&rset);
  FD_SET(st->r1, &rset);
  tv.tv_sec  = 1;
  tv.tv_usec = 0;
  ret = select(st->r1 + 1, &rset, NULL, NULL, &tv);
  assert_return_code(ret, errno);
  assert_int_equal(ret, 1);
  assert_true(FD_ISSET(st->r1, &rset));
  local_dgram_expect(st->r1, "six");
}

/****************************************************************************
 * Name: test_local_dgram_dontwait
 *
 * Description:
 *   recv(MSG_DONTWAIT) of an empty socket fails with EAGAIN.
 *
 ****************************************************************************/

void test_local_dgram_dontwait(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  char buf[16];
  ssize_t ret;

  errno = 0;
  ret = recv(st->r1, buf, sizeof(buf), MSG_DONTWAIT);
  assert_int_equal(ret, -1);
  assert_true(errno == EAGAIN || errno == EWOULDBLOCK);
}

/****************************************************************************
 * Name: test_local_dgram_nonblock
 *
 * Description:
 *   recv() of an empty socket with O_NONBLOCK set after bind() fails with
 *   EAGAIN.
 *
 ****************************************************************************/

void test_local_dgram_nonblock(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  char buf[16];
  ssize_t ret;
  int flags;

  st->r2 = local_dgram_bind(st->p2, false);
  assert_return_code(st->r2, errno);

  flags = fcntl(st->r2, F_GETFL);
  assert_return_code(flags, errno);
  ret = fcntl(st->r2, F_SETFL, flags | O_NONBLOCK);
  assert_return_code(ret, errno);

  errno = 0;
  ret = recv(st->r2, buf, sizeof(buf), 0);
  assert_int_equal(ret, -1);
  assert_true(errno == EAGAIN || errno == EWOULDBLOCK);
}
