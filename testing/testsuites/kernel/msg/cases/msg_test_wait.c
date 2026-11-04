/****************************************************************************
 * apps/testing/testsuites/kernel/msg/cases/msg_test_wait.c
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
#include <unistd.h>

#include "MsgTest.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct msg_test_waiter_s g_msg_test_waiter;
static struct msg_test_waiter_s g_msg_test_waiter2;
static volatile int g_msg_test_nsig;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void msg_test_sighandler(int signo)
{
  g_msg_test_nsig++;
}

/* A new queue with room for one message of the maximum size; full is true:
 * the queue has one message of type 1.
 */

static int msg_test_queue(bool full)
{
  char text[MSG_TEST_MAXSIZE];
  int id;

  id = msg_test_create(IPC_PRIVATE, 0600);
  assert_true(id >= 0);
  assert_int_equal(msg_test_set_qbytes(id, MSG_TEST_MAXSIZE), 0);

  if (full)
    {
      memset(text, 'x', sizeof(text));
      assert_int_equal(msg_test_send(id, 1, text, sizeof(text),
                                     IPC_NOWAIT), 0);
      assert_int_equal(msg_test_send(id, 1, text, sizeof(text),
                                     IPC_NOWAIT), -1);
      assert_int_equal(errno, EAGAIN);
    }

  return id;
}

/* A signal to a thread blocked in msgsnd() or msgrcv(): EINTR */

static void msg_test_eintr(bool snd)
{
  FAR struct msg_test_waiter_s *w = &g_msg_test_waiter;
  struct sigaction act;
  struct sigaction old;
  int id;

  memset(&act, 0, sizeof(act));
  act.sa_handler = msg_test_sighandler;
  sigemptyset(&act.sa_mask);
  assert_int_equal(sigaction(SIGUSR1, &act, &old), 0);

  id = msg_test_queue(snd);
  g_msg_test_nsig = 0;
  msg_test_start(w, id, snd, 1);
  assert_false(w->done);

  assert_int_equal(pthread_kill(w->thread, SIGUSR1), 0);
  assert_true(msg_test_finish(w));
  sigaction(SIGUSR1, &old, NULL);

  assert_int_equal(g_msg_test_nsig, 1);
  assert_int_equal(w->ret, -1);
  assert_int_equal(w->err, EINTR);
  assert_int_equal(msg_test_qnum(id), snd ? 1 : 0);
}

/* IPC_RMID of the queue of a thread blocked in msgsnd() or msgrcv() */

static void msg_test_eidrm(bool snd)
{
  FAR struct msg_test_waiter_s *w = &g_msg_test_waiter;
  int id;

  id = msg_test_queue(snd);
  msg_test_start(w, id, snd, 1);
  assert_false(w->done);

  assert_int_equal(msg_test_remove(id), 0);
  assert_true(msg_test_finish(w));
  assert_int_equal(w->ret, -1);
  assert_int_equal(w->err, EIDRM);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_msg_wait_rcv
 *
 * Description:
 *   A thread blocked in msgrcv() of type 2 stays blocked after a message of
 *   type 1, and receives the next message of type 2.
 *
 ****************************************************************************/

void test_nuttx_msg_wait_rcv(FAR void **state)
{
  FAR struct msg_test_waiter_s *w = &g_msg_test_waiter;
  int id;

  id = msg_test_queue(false);
  assert_int_equal(msg_test_set_qbytes(id, 2 * MSG_TEST_MAXSIZE), 0);
  msg_test_start(w, id, false, 2);
  assert_false(w->done);

  assert_int_equal(msg_test_send(id, 1, "a", 1, 0), 0);
  usleep(100000);
  assert_false(w->done);

  assert_int_equal(msg_test_send(id, 2, "b", 1, 0), 0);
  assert_true(msg_test_finish(w));
  assert_int_equal(w->ret, 1);
  assert_int_equal(w->buf.mtype, 2);
  assert_int_equal(w->buf.mtext[0], 'b');
  assert_int_equal(msg_test_qnum(id), 1);
}

/****************************************************************************
 * Name: test_nuttx_msg_wait_types
 *
 * Description:
 *   Two threads block in msgrcv(): the first for type 2, the second for
 *   type 1.  A message of type 1 goes to the second thread.
 *
 ****************************************************************************/

void test_nuttx_msg_wait_types(FAR void **state)
{
  FAR struct msg_test_waiter_s *w1 = &g_msg_test_waiter;
  FAR struct msg_test_waiter_s *w2 = &g_msg_test_waiter2;
  int id;

  id = msg_test_queue(false);
  assert_int_equal(msg_test_set_qbytes(id, 2 * MSG_TEST_MAXSIZE), 0);
  msg_test_start(w1, id, false, 2);
  msg_test_start(w2, id, false, 1);
  assert_false(w1->done);
  assert_false(w2->done);

  assert_int_equal(msg_test_send(id, 1, "a", 1, 0), 0);
  assert_true(msg_test_finish(w2));
  assert_int_equal(w2->ret, 1);
  assert_int_equal(w2->buf.mtype, 1);
  assert_false(w1->done);

  assert_int_equal(msg_test_send(id, 2, "b", 1, 0), 0);
  assert_true(msg_test_finish(w1));
  assert_int_equal(w1->ret, 1);
  assert_int_equal(w1->buf.mtype, 2);
  assert_int_equal(msg_test_qnum(id), 0);
}

/****************************************************************************
 * Name: test_nuttx_msg_wait_snd
 *
 * Description:
 *   A thread blocked in msgsnd() of a full queue sends its message after
 *   msgrcv() makes room.
 *
 ****************************************************************************/

void test_nuttx_msg_wait_snd(FAR void **state)
{
  FAR struct msg_test_waiter_s *w = &g_msg_test_waiter;
  struct msg_test_buf_s buf;
  int id;

  id = msg_test_queue(true);
  msg_test_start(w, id, true, 2);
  assert_false(w->done);

  assert_int_equal(msg_test_recv(id, &buf, MSG_TEST_MAXSIZE, 0, 0),
                   MSG_TEST_MAXSIZE);
  assert_int_equal(buf.mtype, 1);
  assert_true(msg_test_finish(w));
  assert_int_equal(w->ret, 0);

  assert_int_equal(msg_test_recv(id, &buf, MSG_TEST_MAXSIZE, 0,
                                 IPC_NOWAIT), MSG_TEST_MAXSIZE);
  assert_int_equal(buf.mtype, 2);
}

/****************************************************************************
 * Name: test_nuttx_msg_eidrm_rcv
 *
 * Description:
 *   IPC_RMID wakes up a thread blocked in msgrcv(): EIDRM.
 *
 ****************************************************************************/

void test_nuttx_msg_eidrm_rcv(FAR void **state)
{
  msg_test_eidrm(false);
}

/****************************************************************************
 * Name: test_nuttx_msg_eidrm_snd
 *
 * Description:
 *   IPC_RMID wakes up a thread blocked in msgsnd(): EIDRM.
 *
 ****************************************************************************/

void test_nuttx_msg_eidrm_snd(FAR void **state)
{
  msg_test_eidrm(true);
}

/****************************************************************************
 * Name: test_nuttx_msg_eintr_rcv
 *
 * Description:
 *   A signal interrupts a thread blocked in msgrcv(): EINTR.
 *
 ****************************************************************************/

void test_nuttx_msg_eintr_rcv(FAR void **state)
{
  msg_test_eintr(false);
}

/****************************************************************************
 * Name: test_nuttx_msg_eintr_snd
 *
 * Description:
 *   A signal interrupts a thread blocked in msgsnd(): EINTR.
 *
 ****************************************************************************/

void test_nuttx_msg_eintr_snd(FAR void **state)
{
  msg_test_eintr(true);
}
