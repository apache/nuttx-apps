/****************************************************************************
 * apps/testing/testsuites/kernel/msg/cases/msg_test_size.c
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
#include <string.h>

#include "MsgTest.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_msg_noerror
 *
 * Description:
 *   MSG_NOERROR truncates a message that is longer than msgsz.
 *
 ****************************************************************************/

void test_nuttx_msg_noerror(FAR void **state)
{
  struct msg_test_buf_s buf;
  int id;

  id = msg_test_create(IPC_PRIVATE, 0600);
  assert_true(id >= 0);

  assert_int_equal(msg_test_send(id, 1, "abcdefgh", 8, 0), 0);
  assert_int_equal(msg_test_recv(id, &buf, 4, 0, MSG_NOERROR), 4);
  assert_int_equal(buf.mtype, 1);
  assert_memory_equal(buf.mtext, "abcd", 4);
  assert_int_equal(buf.mtext[4], 0);
  assert_int_equal(msg_test_qnum(id), 0);
}

/****************************************************************************
 * Name: test_nuttx_msg_e2big
 *
 * Description:
 *   Without MSG_NOERROR, a message longer than msgsz gives E2BIG and stays
 *   in the queue.  A message that fits is received, also with a small
 *   msgsz.
 *
 ****************************************************************************/

void test_nuttx_msg_e2big(FAR void **state)
{
  struct msg_test_buf_s buf;
  int id;

  id = msg_test_create(IPC_PRIVATE, 0600);
  assert_true(id >= 0);

  assert_int_equal(msg_test_send(id, 1, "abcdefgh", 8, 0), 0);

  assert_int_equal(msg_test_recv(id, &buf, 4, 0, 0), -1);
  assert_int_equal(errno, E2BIG);
  assert_int_equal(msg_test_recv(id, &buf, 7, 0, IPC_NOWAIT), -1);
  assert_int_equal(errno, E2BIG);
  assert_int_equal(msg_test_qnum(id), 1);

  assert_int_equal(msg_test_recv(id, &buf, 8, 0, 0), 8);
  assert_memory_equal(buf.mtext, "abcdefgh", 8);

  assert_int_equal(msg_test_send(id, 1, "ab", 2, 0), 0);
  assert_int_equal(msg_test_recv(id, &buf, 2, 0, 0), 2);
  assert_memory_equal(buf.mtext, "ab", 2);
}

/****************************************************************************
 * Name: test_nuttx_msg_nowait
 *
 * Description:
 *   IPC_NOWAIT: msgrcv() of an empty queue gives ENOMSG, and msgsnd() to a
 *   full queue gives EAGAIN.
 *
 ****************************************************************************/

void test_nuttx_msg_nowait(FAR void **state)
{
  char text[MSG_TEST_MAXSIZE];
  struct msg_test_buf_s buf;
  int id;

  id = msg_test_create(IPC_PRIVATE, 0600);
  assert_true(id >= 0);

  assert_int_equal(msg_test_recv(id, &buf, MSG_TEST_MAXSIZE, 0,
                                 IPC_NOWAIT), -1);
  assert_int_equal(errno, ENOMSG);

  /* Room for two messages of the maximum size */

  assert_int_equal(msg_test_set_qbytes(id, 2 * MSG_TEST_MAXSIZE), 0);

  memset(text, 'x', sizeof(text));
  assert_int_equal(msg_test_send(id, 1, text, sizeof(text), IPC_NOWAIT), 0);
  assert_int_equal(msg_test_send(id, 1, text, sizeof(text), IPC_NOWAIT), 0);
  assert_int_equal(msg_test_send(id, 1, text, sizeof(text), IPC_NOWAIT), -1);
  assert_int_equal(errno, EAGAIN);
  assert_int_equal(msg_test_qnum(id), 2);
}

/****************************************************************************
 * Name: test_nuttx_msg_einval
 *
 * Description:
 *   EINVAL: a type less than 1, a message that is too long, and a queue
 *   identifier that is not valid.
 *
 ****************************************************************************/

void test_nuttx_msg_einval(FAR void **state)
{
  char text[MSG_TEST_MAXSIZE + 1];
  struct msg_test_buf_s buf;
  int id;

  id = msg_test_create(IPC_PRIVATE, 0600);
  assert_true(id >= 0);

  assert_int_equal(msg_test_send(id, 0, "a", 1, IPC_NOWAIT), -1);
  assert_int_equal(errno, EINVAL);
  assert_int_equal(msg_test_send(id, -1, "a", 1, IPC_NOWAIT), -1);
  assert_int_equal(errno, EINVAL);
  memset(text, 'x', sizeof(text));
  assert_int_equal(msg_test_send(id, 1, text, sizeof(text), IPC_NOWAIT), -1);
  assert_int_equal(errno, EINVAL);
  assert_int_equal(msg_test_qnum(id), 0);

  assert_int_equal(msg_test_send(-1, 1, "a", 1, IPC_NOWAIT), -1);
  assert_int_equal(errno, EINVAL);
  assert_int_equal(msg_test_recv(-1, &buf, 1, 0, IPC_NOWAIT), -1);
  assert_int_equal(errno, EINVAL);
}
