/****************************************************************************
 * apps/testing/testsuites/kernel/msg/common/test_msg_common.c
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
#include <pthread.h>
#include <sched.h>
#include <string.h>
#include <unistd.h>

#include "MsgTest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MSG_TEST_NQUEUES  8
#define MSG_TEST_NWAITERS 4

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_msg_test_ids[MSG_TEST_NQUEUES];
static int g_msg_test_nids;
static FAR struct msg_test_waiter_s *g_msg_test_waiters[MSG_TEST_NWAITERS];
static int g_msg_test_nwaiters;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static FAR void *msg_test_waiter(FAR void *arg)
{
  FAR struct msg_test_waiter_s *w = arg;

  errno = 0;
  if (w->snd)
    {
      w->buf.mtype = w->mtype;
      w->ret = msgsnd(w->msqid, &w->buf, MSG_TEST_MAXSIZE, 0);
    }
  else
    {
      w->ret = msgrcv(w->msqid, &w->buf, MSG_TEST_MAXSIZE, w->mtype, 0);
    }

  w->err  = w->ret < 0 ? errno : 0;
  w->done = true;
  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* msgget() that remembers the queue, so that the teardown removes it */

int msg_test_create(key_t key, int msgflg)
{
  int msqid;

  msqid = msgget(key, msgflg);
  if (msqid >= 0 && g_msg_test_nids < MSG_TEST_NQUEUES)
    {
      g_msg_test_ids[g_msg_test_nids++] = msqid;
    }

  return msqid;
}

int msg_test_remove(int msqid)
{
  int i;

  for (i = 0; i < g_msg_test_nids; i++)
    {
      if (g_msg_test_ids[i] == msqid)
        {
          g_msg_test_ids[i] = g_msg_test_ids[--g_msg_test_nids];
          break;
        }
    }

  return msgctl(msqid, IPC_RMID, NULL);
}

int msg_test_send(int msqid, long mtype, FAR const char *text, size_t len,
                  int msgflg)
{
  struct msg_test_buf_s buf;

  memset(&buf, 0, sizeof(buf));
  buf.mtype = mtype;
  memcpy(buf.mtext, text, len);
  return msgsnd(msqid, &buf, len, msgflg);
}

ssize_t msg_test_recv(int msqid, FAR struct msg_test_buf_s *buf,
                      size_t len, long mtype, int msgflg)
{
  memset(buf, 0, sizeof(*buf));
  return msgrcv(msqid, buf, len, mtype, msgflg);
}

long msg_test_qnum(int msqid)
{
  struct msqid_ds ds;

  memset(&ds, 0, sizeof(ds));
  if (msgctl(msqid, IPC_STAT, &ds) < 0)
    {
      return -1;
    }

  return ds.msg_qnum;
}

int msg_test_set_qbytes(int msqid, size_t qbytes)
{
  struct msqid_ds ds;
  int ret;

  memset(&ds, 0, sizeof(ds));
  ret = msgctl(msqid, IPC_STAT, &ds);
  if (ret == 0)
    {
      ds.msg_qbytes = qbytes;
      ret = msgctl(msqid, IPC_SET, &ds);
    }

  return ret;
}

/* Start a thread with a higher priority than the caller that blocks in
 * msgsnd() (snd true) or msgrcv() of msqid, and give it time to block.
 */

void msg_test_start(FAR struct msg_test_waiter_s *w, int msqid, bool snd,
                    long mtype)
{
  struct sched_param param;
  pthread_attr_t attr;
  int policy;
  int ret;

  memset(w, 0, sizeof(*w));
  w->msqid = msqid;
  w->snd   = snd;
  w->mtype = mtype;
  w->ret   = -2;

  ret = pthread_getschedparam(pthread_self(), &policy, &param);
  assert_int_equal(ret, 0);
  if (param.sched_priority < sched_get_priority_max(SCHED_FIFO))
    {
      param.sched_priority++;
    }

  pthread_attr_init(&attr);
  pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
  pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
  pthread_attr_setschedparam(&attr, &param);
  ret = pthread_create(&w->thread, &attr, msg_test_waiter, w);
  pthread_attr_destroy(&attr);
  assert_int_equal(ret, 0);

  if (g_msg_test_nwaiters < MSG_TEST_NWAITERS)
    {
      g_msg_test_waiters[g_msg_test_nwaiters++] = w;
    }

  usleep(100000);
}

/* Wait up to 3 seconds for the waiter to return and join it; false if it
 * is still blocked.
 */

bool msg_test_finish(FAR struct msg_test_waiter_s *w)
{
  int i;

  for (i = 0; i < 300 && !w->done; i++)
    {
      usleep(10000);
    }

  if (w->done && !w->joined)
    {
      pthread_join(w->thread, NULL);
      w->joined = true;
    }

  return w->done;
}

/* Remove the queues of the test case; this also wakes up its waiters */

int msg_test_teardown(FAR void **state)
{
  int i;

  while (g_msg_test_nids > 0)
    {
      msg_test_remove(g_msg_test_ids[0]);
    }

  for (i = 0; i < g_msg_test_nwaiters; i++)
    {
      msg_test_finish(g_msg_test_waiters[i]);
    }

  g_msg_test_nwaiters = 0;
  return 0;
}
