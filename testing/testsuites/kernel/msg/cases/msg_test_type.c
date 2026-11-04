/****************************************************************************
 * apps/testing/testsuites/kernel/msg/cases/msg_test_type.c
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

#include "MsgTest.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Receive one message and check its type and its one-byte text */

static void msg_test_expect(int msqid, long msgtyp, long mtype, char text)
{
  struct msg_test_buf_s buf;

  assert_int_equal(msg_test_recv(msqid, &buf, MSG_TEST_MAXSIZE, msgtyp,
                                 IPC_NOWAIT), 1);
  assert_int_equal(buf.mtype, mtype);
  assert_int_equal(buf.mtext[0], text);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_msg_type_zero
 *
 * Description:
 *   msgtyp 0 receives the messages in the order of msgsnd().
 *
 ****************************************************************************/

void test_nuttx_msg_type_zero(FAR void **state)
{
  int id;

  id = msg_test_create(IPC_PRIVATE, 0600);
  assert_true(id >= 0);

  assert_int_equal(msg_test_send(id, 3, "a", 1, 0), 0);
  assert_int_equal(msg_test_send(id, 1, "b", 1, 0), 0);
  assert_int_equal(msg_test_send(id, 2, "c", 1, 0), 0);
  assert_int_equal(msg_test_qnum(id), 3);

  msg_test_expect(id, 0, 3, 'a');
  msg_test_expect(id, 0, 1, 'b');
  msg_test_expect(id, 0, 2, 'c');
  assert_int_equal(msg_test_qnum(id), 0);
}

/****************************************************************************
 * Name: test_nuttx_msg_type_positive
 *
 * Description:
 *   msgtyp > 0 receives the first message of that type.
 *
 ****************************************************************************/

void test_nuttx_msg_type_positive(FAR void **state)
{
  struct msg_test_buf_s buf;
  int id;

  id = msg_test_create(IPC_PRIVATE, 0600);
  assert_true(id >= 0);

  assert_int_equal(msg_test_send(id, 1, "a", 1, 0), 0);
  assert_int_equal(msg_test_send(id, 2, "b", 1, 0), 0);
  assert_int_equal(msg_test_send(id, 1, "c", 1, 0), 0);

  msg_test_expect(id, 2, 2, 'b');
  msg_test_expect(id, 1, 1, 'a');
  msg_test_expect(id, 1, 1, 'c');

  assert_int_equal(msg_test_recv(id, &buf, MSG_TEST_MAXSIZE, 1,
                                 IPC_NOWAIT), -1);
  assert_int_equal(errno, ENOMSG);
}

/****************************************************************************
 * Name: test_nuttx_msg_type_negative
 *
 * Description:
 *   msgtyp < 0 receives the first message of the lowest type that is less
 *   than or equal to the absolute value of msgtyp.
 *
 ****************************************************************************/

void test_nuttx_msg_type_negative(FAR void **state)
{
  struct msg_test_buf_s buf;
  int id;

  id = msg_test_create(IPC_PRIVATE, 0600);
  assert_true(id >= 0);

  assert_int_equal(msg_test_send(id, 3, "a", 1, 0), 0);
  assert_int_equal(msg_test_send(id, 2, "b", 1, 0), 0);
  assert_int_equal(msg_test_send(id, 1, "c", 1, 0), 0);
  assert_int_equal(msg_test_send(id, 1, "d", 1, 0), 0);

  msg_test_expect(id, -2, 1, 'c');
  msg_test_expect(id, -2, 1, 'd');
  msg_test_expect(id, -2, 2, 'b');

  /* Only type 3 is left: no message of type 2 or less */

  assert_int_equal(msg_test_recv(id, &buf, MSG_TEST_MAXSIZE, -2,
                                 IPC_NOWAIT), -1);
  assert_int_equal(errno, ENOMSG);

  msg_test_expect(id, -3, 3, 'a');
}
