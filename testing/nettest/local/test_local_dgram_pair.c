/****************************************************************************
 * apps/testing/nettest/local/test_local_dgram_pair.c
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

/* socketpair(AF_UNIX, SOCK_DGRAM): send() and recv() of whole datagrams in
 * both directions.  The ends are r2 and r3.
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
#include <string.h>
#include <unistd.h>
#include <cmocka.h>

#include "test_local_dgram.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LOCAL_DGRAM_PAIRBUF 16

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Make the pair r2, r3 */

static void local_dgram_pair(FAR struct nettest_local_dgram_state_s *st)
{
  int sv[2];
  int ret;

  ret = socketpair(AF_UNIX, SOCK_DGRAM, 0, sv);
  assert_return_code(ret, errno);
  st->r2 = sv[0];
  st->r3 = sv[1];
}

/* recv() one datagram without a wait into size bytes, and check its length
 * and data.
 */

static void local_dgram_pairrecv(int fd, FAR const char *want, size_t size)
{
  char buf[LOCAL_DGRAM_PAIRBUF];
  ssize_t ret;

  memset(buf, 0, sizeof(buf));
  ret = recv(fd, buf, size, MSG_DONTWAIT);
  assert_int_equal(ret, strlen(want));
  assert_memory_equal(buf, want, ret);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_local_dgram_pair_send
 *
 * Description:
 *   send() on a datagram socket pair keeps the boundaries of the datagrams,
 *   also of an empty one, in both directions.  A short buffer gets the
 *   start of a datagram and the rest is lost.
 *
 ****************************************************************************/

void test_local_dgram_pair_send(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  char buf[16];

  local_dgram_pair(st);

  assert_int_equal(send(st->r2, "one", 3, 0), 3);
  assert_int_equal(send(st->r2, "", 0, 0), 0);
  assert_int_equal(send(st->r2, "three", 5, 0), 5);
  assert_int_equal(send(st->r2, "abcdef", 6, 0), 6);
  assert_int_equal(send(st->r2, "x", 1, 0), 1);

  local_dgram_pairrecv(st->r3, "one", LOCAL_DGRAM_PAIRBUF);
  local_dgram_pairrecv(st->r3, "", LOCAL_DGRAM_PAIRBUF);
  local_dgram_pairrecv(st->r3, "three", LOCAL_DGRAM_PAIRBUF);
  local_dgram_pairrecv(st->r3, "abc", 3);
  local_dgram_pairrecv(st->r3, "x", LOCAL_DGRAM_PAIRBUF);

  errno = 0;
  assert_int_equal(recv(st->r3, buf, sizeof(buf), MSG_DONTWAIT), -1);
  assert_true(errno == EAGAIN || errno == EWOULDBLOCK);

  assert_int_equal(write(st->r3, "back", 4), 4);
  local_dgram_pairrecv(st->r2, "back", LOCAL_DGRAM_PAIRBUF);
}

/****************************************************************************
 * Name: test_local_dgram_pair_full
 *
 * Description:
 *   A send that must not wait fails with EAGAIN when the peer is full, and
 *   leaves no part of a datagram: each datagram comes out whole and in
 *   order.  poll() gives POLLOUT only when the peer has space again.
 *
 ****************************************************************************/

void test_local_dgram_pair_full(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  char buf[LOCAL_DGRAM_PAIRBUF];
  int count;

  local_dgram_pair(st);

  count = local_dgram_fill(st->r2, NULL);
  assert_int_equal(local_dgram_revents(st->r2, POLLOUT, 0), 0);
  local_dgram_unfill(st->r3, count);

  errno = 0;
  assert_int_equal(recv(st->r3, buf, sizeof(buf), MSG_DONTWAIT), -1);
  assert_true(errno == EAGAIN || errno == EWOULDBLOCK);
  assert_int_equal(local_dgram_revents(st->r2, POLLOUT, 0), POLLOUT);

  assert_int_equal(send(st->r2, "next", 4, MSG_DONTWAIT), 4);
  local_dgram_pairrecv(st->r3, "next", LOCAL_DGRAM_PAIRBUF);
}

/****************************************************************************
 * Name: test_local_dgram_pair_close
 *
 * Description:
 *   A datagram sent before the peer closes can still be read.  A send to a
 *   closed peer fails with ECONNREFUSED.
 *
 ****************************************************************************/

void test_local_dgram_pair_close(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;

  local_dgram_pair(st);

  assert_int_equal(send(st->r2, "last", 4, 0), 4);
  assert_int_equal(close(st->r2), 0);
  st->r2 = -1;
  local_dgram_pairrecv(st->r3, "last", LOCAL_DGRAM_PAIRBUF);

  errno = 0;
  assert_int_equal(send(st->r3, "gone", 4, 0), -1);
  assert_int_equal(errno, ECONNREFUSED);
}

/****************************************************************************
 * Name: test_local_dgram_poll_pair
 *
 * Description:
 *   poll() of a socketpair(SOCK_DGRAM).
 *
 ****************************************************************************/

void test_local_dgram_poll_pair(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;

  local_dgram_pair(st);

  assert_int_equal(local_dgram_revents(st->r2, POLLOUT, 0), POLLOUT);
  assert_int_equal(local_dgram_revents(st->r3, POLLOUT, 0), POLLOUT);
  assert_int_equal(local_dgram_revents(st->r3, POLLIN, 0), 0);

  assert_int_equal(send(st->r2, "pa", 2, 0), 2);
  assert_int_equal(local_dgram_revents(st->r3, POLLIN, 1000), POLLIN);
  assert_int_equal(local_dgram_revents(st->r2, POLLIN, 0), 0);
  local_dgram_expect(st->r3, "pa");
  assert_int_equal(local_dgram_revents(st->r3, POLLIN, 0), 0);

  assert_int_equal(send(st->r3, "ir", 2, 0), 2);
  assert_int_equal(local_dgram_revents(st->r2, POLLIN, 1000), POLLIN);
  local_dgram_expect(st->r2, "ir");
}
