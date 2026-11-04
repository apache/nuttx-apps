/****************************************************************************
 * apps/testing/testsuites/kernel/msg/cases/msg_test_pool.c
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

#include <nuttx/config.h>

#include <errno.h>
#include <signal.h>
#include <string.h>

#include "MsgTest.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct msg_test_waiter_s g_msg_test_poolwaiter;
static volatile int g_msg_test_poolsig;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void msg_test_poolhandler(int signo)
{
  g_msg_test_poolsig++;
}

/* Take all CONFIG_PREALLOC_MQ_MSGS messages of the system-wide pool: they
 * stay in the queue *full.  Return the identifier of a second, empty queue.
 */

static int msg_test_fill_pool(FAR int *full)
{
  int id;
  int i;

  *full = msg_test_create(IPC_PRIVATE, 0600);
  assert_true(*full >= 0);
  assert_int_equal(msg_test_set_qbytes(*full, CONFIG_PREALLOC_MQ_MSGS *
                                              MSG_TEST_MAXSIZE), 0);

  for (i = 0; i < CONFIG_PREALLOC_MQ_MSGS; i++)
    {
      assert_int_equal(msg_test_send(*full, 1, "p", 1, IPC_NOWAIT), 0);
    }

  id = msg_test_create(IPC_PRIVATE, 0600);
  assert_true(id >= 0);
  return id;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_msg_pool_nowait
 *
 * Description:
 *   No free message in the system: msgsnd() with IPC_NOWAIT gives EAGAIN.
 *
 ****************************************************************************/

void test_nuttx_msg_pool_nowait(FAR void **state)
{
  int full;
  int id;

  id = msg_test_fill_pool(&full);
  assert_int_equal(msg_test_send(id, 1, "a", 1, IPC_NOWAIT), -1);
  assert_int_equal(errno, EAGAIN);
  assert_int_equal(msg_test_qnum(id), 0);
}

/****************************************************************************
 * Name: test_nuttx_msg_pool_wait
 *
 * Description:
 *   No free message in the system: msgsnd() blocks until msgrcv() of
 *   another queue frees a message.
 *
 ****************************************************************************/

void test_nuttx_msg_pool_wait(FAR void **state)
{
  FAR struct msg_test_waiter_s *w = &g_msg_test_poolwaiter;
  struct msg_test_buf_s buf;
  int full;
  int id;

  id = msg_test_fill_pool(&full);
  assert_int_equal(msg_test_qnum(full), CONFIG_PREALLOC_MQ_MSGS);

  msg_test_start(w, id, true, 1);
  assert_false(w->done);

  assert_int_equal(msg_test_recv(full, &buf, 1, 0, 0), 1);
  assert_true(msg_test_finish(w));
  assert_int_equal(w->ret, 0);
  assert_int_equal(msg_test_qnum(id), 1);
}

/****************************************************************************
 * Name: test_nuttx_msg_pool_eintr
 *
 * Description:
 *   A signal interrupts a thread that waits in msgsnd() for a free
 *   message: EINTR.
 *
 ****************************************************************************/

void test_nuttx_msg_pool_eintr(FAR void **state)
{
  FAR struct msg_test_waiter_s *w = &g_msg_test_poolwaiter;
  struct sigaction act;
  struct sigaction old;
  int full;
  int id;

  memset(&act, 0, sizeof(act));
  act.sa_handler = msg_test_poolhandler;
  sigemptyset(&act.sa_mask);
  assert_int_equal(sigaction(SIGUSR1, &act, &old), 0);

  id = msg_test_fill_pool(&full);
  g_msg_test_poolsig = 0;
  msg_test_start(w, id, true, 1);
  assert_false(w->done);

  assert_int_equal(pthread_kill(w->thread, SIGUSR1), 0);
  assert_true(msg_test_finish(w));
  sigaction(SIGUSR1, &old, NULL);

  assert_int_equal(g_msg_test_poolsig, 1);
  assert_int_equal(w->ret, -1);
  assert_int_equal(w->err, EINTR);
  assert_int_equal(msg_test_qnum(id), 0);
}

/****************************************************************************
 * Name: test_nuttx_msg_pool_eidrm
 *
 * Description:
 *   IPC_RMID of the queue of a thread that waits in msgsnd() for a free
 *   message: EIDRM, while the pool stays empty.
 *
 ****************************************************************************/

void test_nuttx_msg_pool_eidrm(FAR void **state)
{
  FAR struct msg_test_waiter_s *w = &g_msg_test_poolwaiter;
  int full;
  int id;

  id = msg_test_fill_pool(&full);
  msg_test_start(w, id, true, 1);
  assert_false(w->done);

  assert_int_equal(msg_test_remove(id), 0);
  assert_true(msg_test_finish(w));
  assert_int_equal(w->ret, -1);
  assert_int_equal(w->err, EIDRM);
}
