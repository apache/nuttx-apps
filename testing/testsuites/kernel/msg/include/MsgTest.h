/****************************************************************************
 * apps/testing/testsuites/kernel/msg/include/MsgTest.h
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

#ifndef __APPS_TESTING_TESTSUITES_KERNEL_MSG_INCLUDE_MSGTEST_H
#define __APPS_TESTING_TESTSUITES_KERNEL_MSG_INCLUDE_MSGTEST_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The largest message that msgsnd() accepts */

#define MSG_TEST_MAXSIZE   CONFIG_MQ_MAXMSGSIZE

/* The key of the queue in the msgget() test */

#define MSG_TEST_KEY       ((key_t)0x4d534754)

/* The number of queues in the test of many queues */

#define MSG_TEST_NMANY     260

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct msg_test_buf_s
{
  long mtype;
  char mtext[MSG_TEST_MAXSIZE + 1];
};

/* A thread that blocks in msgsnd() or msgrcv() */

struct msg_test_waiter_s
{
  pthread_t thread;
  int msqid;
  bool snd;
  long mtype;
  volatile bool done;
  bool joined;
  ssize_t ret;
  int err;
  struct msg_test_buf_s buf;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* common/test_msg_common.c *************************************************/

int msg_test_create(key_t key, int msgflg);
int msg_test_remove(int msqid);
int msg_test_send(int msqid, long mtype, FAR const char *text, size_t len,
                  int msgflg);
ssize_t msg_test_recv(int msqid, FAR struct msg_test_buf_s *buf,
                      size_t len, long mtype, int msgflg);
long msg_test_qnum(int msqid);
int msg_test_set_qbytes(int msqid, size_t qbytes);
void msg_test_start(FAR struct msg_test_waiter_s *w, int msqid, bool snd,
                    long mtype);
bool msg_test_finish(FAR struct msg_test_waiter_s *w);
int msg_test_teardown(FAR void **state);

/* cases/msg_test_get.c *****************************************************/

void test_nuttx_msg_get_private(FAR void **state);
void test_nuttx_msg_get_key(FAR void **state);
void test_nuttx_msg_get_many(FAR void **state);

/* cases/msg_test_type.c ****************************************************/

void test_nuttx_msg_type_zero(FAR void **state);
void test_nuttx_msg_type_positive(FAR void **state);
void test_nuttx_msg_type_negative(FAR void **state);

/* cases/msg_test_size.c ****************************************************/

void test_nuttx_msg_noerror(FAR void **state);
void test_nuttx_msg_e2big(FAR void **state);
void test_nuttx_msg_nowait(FAR void **state);
void test_nuttx_msg_einval(FAR void **state);

/* cases/msg_test_ctl.c *****************************************************/

void test_nuttx_msg_ctl_stat(FAR void **state);
void test_nuttx_msg_ctl_fields(FAR void **state);
void test_nuttx_msg_ctl_set(FAR void **state);
void test_nuttx_msg_ctl_rmid(FAR void **state);

/* cases/msg_test_wait.c ****************************************************/

void test_nuttx_msg_wait_rcv(FAR void **state);
void test_nuttx_msg_wait_types(FAR void **state);
void test_nuttx_msg_wait_snd(FAR void **state);
void test_nuttx_msg_eidrm_rcv(FAR void **state);
void test_nuttx_msg_eidrm_snd(FAR void **state);
void test_nuttx_msg_eintr_rcv(FAR void **state);
void test_nuttx_msg_eintr_snd(FAR void **state);

/* cases/msg_test_pool.c ****************************************************/

void test_nuttx_msg_pool_nowait(FAR void **state);
void test_nuttx_msg_pool_wait(FAR void **state);
void test_nuttx_msg_pool_eintr(FAR void **state);
void test_nuttx_msg_pool_eidrm(FAR void **state);

#endif /* __APPS_TESTING_TESTSUITES_KERNEL_MSG_INCLUDE_MSGTEST_H */
