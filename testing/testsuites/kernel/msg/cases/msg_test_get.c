/****************************************************************************
 * apps/testing/testsuites/kernel/msg/cases/msg_test_get.c
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
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_msg_get_private
 *
 * Description:
 *   IPC_PRIVATE always creates a new queue, also without IPC_CREAT.
 *
 ****************************************************************************/

void test_nuttx_msg_get_private(FAR void **state)
{
  struct msqid_ds ds;
  int id1;
  int id2;

  id1 = msg_test_create(IPC_PRIVATE, IPC_CREAT | 0600);
  assert_true(id1 >= 0);

  id2 = msg_test_create(IPC_PRIVATE, 0600);
  assert_true(id2 >= 0);
  assert_int_not_equal(id1, id2);

  assert_int_equal(msg_test_remove(id1), 0);
  assert_int_equal(msgctl(id1, IPC_STAT, &ds), -1);
  assert_int_equal(errno, EINVAL);
  assert_int_equal(msg_test_qnum(id2), 0);
}

/****************************************************************************
 * Name: test_nuttx_msg_get_key
 *
 * Description:
 *   msgget() of a key: ENOENT before IPC_CREAT, then the same identifier,
 *   and EEXIST with IPC_CREAT | IPC_EXCL.
 *
 ****************************************************************************/

void test_nuttx_msg_get_key(FAR void **state)
{
  int id;

  assert_int_equal(msgget(MSG_TEST_KEY, 0600), -1);
  assert_int_equal(errno, ENOENT);

  id = msg_test_create(MSG_TEST_KEY, IPC_CREAT | 0600);
  assert_true(id >= 0);

  assert_int_equal(msgget(MSG_TEST_KEY, 0600), id);
  assert_int_equal(msg_test_create(MSG_TEST_KEY, IPC_CREAT | 0600), id);

  assert_int_equal(msg_test_create(MSG_TEST_KEY,
                                   IPC_CREAT | IPC_EXCL | 0600), -1);
  assert_int_equal(errno, EEXIST);

  assert_int_equal(msg_test_remove(id), 0);
  assert_int_equal(msgget(MSG_TEST_KEY, 0600), -1);
  assert_int_equal(errno, ENOENT);
}

/****************************************************************************
 * Name: test_nuttx_msg_get_many
 *
 * Description:
 *   More than 250 queues at the same time: each one has its own
 *   identifier, and msgctl() finds each one.  Skipped when the kernel heap
 *   is too small for them.
 *
 ****************************************************************************/

void test_nuttx_msg_get_many(FAR void **state)
{
  static int ids[MSG_TEST_NMANY];
  struct msqid_ds ds;
  int nfound = 0;
  int ndup = 0;
  int n;
  int i;
  int j;

  for (n = 0; n < MSG_TEST_NMANY; n++)
    {
      ids[n] = msgget(IPC_PRIVATE, 0600);
      if (ids[n] < 0)
        {
          break;
        }
    }

  for (i = 0; i < n; i++)
    {
      if (msgctl(ids[i], IPC_STAT, &ds) == 0)
        {
          nfound++;
        }

      for (j = 0; j < i; j++)
        {
          if (ids[j] == ids[i])
            {
              ndup++;
            }
        }
    }

  for (i = 0; i < n; i++)
    {
      msgctl(ids[i], IPC_RMID, NULL);
    }

  if (n < MSG_TEST_NMANY)
    {
      skip();
    }

  assert_int_equal(nfound, MSG_TEST_NMANY);
  assert_int_equal(ndup, 0);
}
