/****************************************************************************
 * apps/testing/nettest/local/test_local_dgram_wait.c
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

/* A recv() of a local socket that waits for data: a signal ends it with
 * EINTR.  Another thread sends SIGUSR1 (a handler without SA_RESTART) after
 * 200 ms.  If the signal does not end the call, data sent later ends the
 * wait, so that the case fails and does not hang.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <cmocka.h>

#include "test_local_dgram.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void local_dgram_intr_handler(int signo)
{
}

static int local_dgram_do_intr(FAR struct local_dgram_job_s *job)
{
  return pthread_kill(job->target, SIGUSR1);
}

/* Install the handler of SIGUSR1, without SA_RESTART */

static void local_dgram_intr_setup(FAR struct sigaction *old)
{
  struct sigaction act;
  int ret;

  memset(&act, 0, sizeof(act));
  act.sa_handler = local_dgram_intr_handler;
  sigemptyset(&act.sa_mask);
  ret = sigaction(SIGUSR1, &act, old);
  assert_return_code(ret, errno);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_local_dgram_recv_intr
 *
 * Description:
 *   A signal ends recv() of a bound datagram socket that waits for a
 *   datagram.  The datagram that comes later is not lost.
 *
 ****************************************************************************/

void test_local_dgram_recv_intr(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_job_s intr;
  struct local_dgram_job_s later;
  struct sigaction old;
  char buf[16];
  ssize_t ret;
  int err;

  local_dgram_intr_setup(&old);
  local_dgram_job_start(&intr, local_dgram_do_intr, -1, NULL, 200);
  local_dgram_job_start(&later, local_dgram_do_sendto, st->s, st->p1, 1500);
  ret = recv(st->r1, buf, sizeof(buf), 0);
  err = errno;
  local_dgram_job_join(&intr);
  local_dgram_job_join(&later);
  sigaction(SIGUSR1, &old, NULL);

  assert_int_equal(ret, -1);
  assert_int_equal(err, EINTR);
  assert_int_equal(later.ret, 5);
  local_dgram_expect(st->r1, "later");
}

/****************************************************************************
 * Name: test_local_dgram_stream_intr
 *
 * Description:
 *   A signal ends recv() of a stream socket that waits for data.
 *
 ****************************************************************************/

void test_local_dgram_stream_intr(FAR void **state)
{
#ifdef CONFIG_NET_LOCAL_STREAM
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_job_s intr;
  struct local_dgram_job_s later;
  struct sigaction old;
  char buf[16];
  ssize_t ret;
  int sv[2];
  int err;

  ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
  assert_return_code(ret, errno);
  st->r2 = sv[0];
  st->r3 = sv[1];

  local_dgram_intr_setup(&old);
  local_dgram_job_start(&intr, local_dgram_do_intr, -1, NULL, 200);
  local_dgram_job_start(&later, local_dgram_do_send, st->r3, NULL, 1500);
  ret = recv(st->r2, buf, sizeof(buf), 0);
  err = errno;
  local_dgram_job_join(&intr);
  local_dgram_job_join(&later);
  sigaction(SIGUSR1, &old, NULL);

  assert_int_equal(ret, -1);
  assert_int_equal(err, EINTR);
  assert_int_equal(later.ret, 5);
  assert_int_equal(recv(st->r2, buf, sizeof(buf), 0), 5);
#else
  skip();
#endif
}
