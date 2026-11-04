/****************************************************************************
 * apps/testing/testsuites/kernel/msg/cases/msg_test_ctl.c
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
#include <unistd.h>

#include "MsgTest.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_msg_ctl_stat
 *
 * Description:
 *   IPC_STAT gives the number of messages and the size limit of the queue.
 *
 ****************************************************************************/

void test_nuttx_msg_ctl_stat(FAR void **state)
{
  struct msg_test_buf_s buf;
  struct msqid_ds ds;
  int id;

  id = msg_test_create(IPC_PRIVATE, 0600);
  assert_true(id >= 0);

  memset(&ds, 0xff, sizeof(ds));
  assert_int_equal(msgctl(id, IPC_STAT, &ds), 0);
  assert_int_equal(ds.msg_qnum, 0);
  assert_true(ds.msg_qbytes >= MSG_TEST_MAXSIZE);

  assert_int_equal(msg_test_send(id, 1, "a", 1, 0), 0);
  assert_int_equal(msg_test_send(id, 2, "b", 1, 0), 0);
  assert_int_equal(msg_test_qnum(id), 2);

  assert_int_equal(msg_test_recv(id, &buf, 1, 0, 0), 1);
  assert_int_equal(msg_test_qnum(id), 1);
}

/****************************************************************************
 * Name: test_nuttx_msg_ctl_fields
 *
 * Description:
 *   IPC_STAT gives the permissions, the process IDs and the times of the
 *   last msgsnd() and msgrcv().
 *
 ****************************************************************************/

void test_nuttx_msg_ctl_fields(FAR void **state)
{
  struct msg_test_buf_s buf;
  struct msqid_ds ds;
  int id;

  id = msg_test_create(IPC_PRIVATE, 0640);
  assert_true(id >= 0);

  memset(&ds, 0xff, sizeof(ds));
  assert_int_equal(msgctl(id, IPC_STAT, &ds), 0);
  assert_int_equal(ds.msg_perm.mode & 0777, 0640);
  assert_int_equal(ds.msg_lspid, 0);
  assert_int_equal(ds.msg_lrpid, 0);
  assert_int_equal(ds.msg_stime, 0);
  assert_int_equal(ds.msg_rtime, 0);
  assert_true(ds.msg_ctime > 0);

  assert_int_equal(msg_test_send(id, 1, "a", 1, 0), 0);
  memset(&ds, 0xff, sizeof(ds));
  assert_int_equal(msgctl(id, IPC_STAT, &ds), 0);
  assert_int_equal(ds.msg_lspid, getpid());
  assert_true(ds.msg_stime > 0);

  assert_int_equal(msg_test_recv(id, &buf, 1, 0, 0), 1);
  memset(&ds, 0xff, sizeof(ds));
  assert_int_equal(msgctl(id, IPC_STAT, &ds), 0);
  assert_int_equal(ds.msg_lrpid, getpid());
  assert_true(ds.msg_rtime > 0);
}

/****************************************************************************
 * Name: test_nuttx_msg_ctl_set
 *
 * Description:
 *   IPC_SET changes msg_qbytes, and IPC_STAT gives the new value.
 *
 ****************************************************************************/

void test_nuttx_msg_ctl_set(FAR void **state)
{
  struct msqid_ds ds;
  int id;

  id = msg_test_create(IPC_PRIVATE, 0600);
  assert_true(id >= 0);

  assert_int_equal(msg_test_set_qbytes(id, 3 * MSG_TEST_MAXSIZE), 0);
  memset(&ds, 0, sizeof(ds));
  assert_int_equal(msgctl(id, IPC_STAT, &ds), 0);
  assert_int_equal(ds.msg_qbytes, 3 * MSG_TEST_MAXSIZE);

  assert_int_equal(msg_test_set_qbytes(id, MSG_TEST_MAXSIZE + 1), 0);
  memset(&ds, 0, sizeof(ds));
  assert_int_equal(msgctl(id, IPC_STAT, &ds), 0);
  assert_int_equal(ds.msg_qbytes, MSG_TEST_MAXSIZE + 1);

  assert_int_equal(msgctl(id, IPC_SET, NULL), -1);
  assert_int_equal(errno, EFAULT);
}

/****************************************************************************
 * Name: test_nuttx_msg_ctl_rmid
 *
 * Description:
 *   IPC_RMID removes the queue and its messages; the identifier is then
 *   not valid.
 *
 ****************************************************************************/

void test_nuttx_msg_ctl_rmid(FAR void **state)
{
  struct msg_test_buf_s buf;
  struct msqid_ds ds;
  int id;

  id = msg_test_create(IPC_PRIVATE, 0600);
  assert_true(id >= 0);

  assert_int_equal(msg_test_send(id, 1, "a", 1, 0), 0);
  assert_int_equal(msg_test_remove(id), 0);

  assert_int_equal(msgctl(id, IPC_STAT, &ds), -1);
  assert_int_equal(errno, EINVAL);
  assert_int_equal(msgctl(id, IPC_RMID, NULL), -1);
  assert_int_equal(errno, EINVAL);
  assert_int_equal(msg_test_send(id, 1, "a", 1, IPC_NOWAIT), -1);
  assert_int_equal(errno, EINVAL);
  assert_int_equal(msg_test_recv(id, &buf, 1, 0, IPC_NOWAIT), -1);
  assert_int_equal(errno, EINVAL);
}
