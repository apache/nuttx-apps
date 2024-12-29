/****************************************************************************
 * apps/testing/testsuites/kernel/mutex/cases/posix_mutex_test_019.c
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
#include <sys/syscall.h>
#include <unistd.h>
#include <stdint.h>
#include "MutexTest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

static pthread_mutex_t *g_mtx;
static sem_t g_sema;
static sem_t g_semb;
static pthread_mutex_t g_mtxnull;
static pthread_mutex_t g_mtxdef;
static int g_testmutexretval;
static UINT32 g_testmutexcount;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

static void *taskf01(void *arg)
{
  int ret;

  /* testbusytaskdelay(20); */

  usleep(50000);
  g_testmutexcount++;

  if ((ret = pthread_mutex_lock(g_mtx)))
    {
      syslog(LOG_INFO, "ret: %d \n", ret);
      goto EXIT;
    }

  if ((ret = sem_post(&g_sema)))
    {
      syslog(LOG_INFO, "ret: %d \n", ret);
      goto EXIT;
    }

  if ((ret = sem_wait(&g_semb)))
    {
      syslog(LOG_INFO, "ret: %d \n", ret);
      goto EXIT;
    }

  if (g_testmutexretval != 0)
    {
      /* parent thread failed to unlock the mutex) */

      if ((ret = pthread_mutex_unlock(g_mtx)))
        {
          syslog(LOG_INFO, "ret: %d \n", ret);
          goto EXIT;
        }
    }

  g_testmutexcount++;
  return NULL;

EXIT:
  assert_int_equal(1, 0);
  return NULL;
}

/****************************************************************************
 * Name: test_nuttx_mutex_test19
 ****************************************************************************/

void test_nuttx_mutex_test19(FAR void **state)
{
  pthread_mutexattr_t mattr;
  pthread_t thr;

  pthread_mutex_t *tabmutex[2];
  int tabres[2][3] =
  {
    {
      0, 0, 0
    },
    {
      0, 0, 0
    }
  };

  int ret;
  void *thret = NULL;
  int i;
  g_testmutexretval = 0;
  g_testmutexcount = 0;

  /* We first initialize the two mutexes. */

  if ((ret = pthread_mutex_init(&g_mtxnull, NULL)))
    {
      syslog(LOG_INFO, "ret: %d \n", ret);
      assert_int_equal(1, 0);
    }

  if ((ret = pthread_mutexattr_init(&mattr)))
    {
      syslog(LOG_INFO, "ret: %d \n", ret);
      assert_int_equal(1, 0);
    }

  if ((ret = pthread_mutex_init(&g_mtxdef, &mattr)))
    {
      syslog(LOG_INFO, "ret: %d \n", ret);
      assert_int_equal(1, 0);
    }

  if ((ret = pthread_mutexattr_destroy(&mattr)))
    {
      syslog(LOG_INFO, "ret: %d \n", ret);
      assert_int_equal(1, 0);
    }

  tabmutex[0] = &g_mtxnull;
  tabmutex[1] = &g_mtxdef;

  if ((ret = sem_init(&g_sema, 0, 0)))
    {
      syslog(LOG_INFO, "ret: %d \n", ret);
      assert_int_equal(1, 0);
    }

  if ((ret = sem_init(&g_semb, 0, 0)))
    {
      syslog(LOG_INFO, "ret: %d \n", ret);
      assert_int_equal(1, 0);
    }

  /* OK let's go for the first part of the test : abnormals unlocking */

  /* We first check if unlocking an unlocked mutex returns an uwErr. */

  ret = pthread_mutex_unlock(tabmutex[0]);
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_not_equal(ret, ENOERR);

  ret = pthread_mutex_unlock(tabmutex[1]);
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_not_equal(ret, ENOERR);

  /* Now we focus on unlocking a mutex lock by another thread */

  for (i = 0; i < 2; i++)
    {
      /* 2, Set the timeout of runtime */

      g_mtx = tabmutex[i];
      tabres[i][0] = 0;
      tabres[i][1] = 0;
      tabres[i][2] = 0; /* 2, buffer index. */

      if ((ret = pthread_create(&thr, NULL, taskf01, NULL)))
        {
          syslog(LOG_INFO, "ret: %d \n", ret);
          assert_int_equal(1, 0);
        }

      if (i == 0)
        {
          syslog(LOG_INFO, "g_testmutexcount: %d \n", g_testmutexcount);
          assert_int_equal(g_testmutexcount, 0);
        }

      if (i == 1)
        {
          syslog(LOG_INFO, "g_testmutexcount: %d \n", g_testmutexcount);
          assert_int_equal(g_testmutexcount,
                           2); /* 2, Here, assert the g_testmutexcount. */
        }

      if ((ret = sem_wait(&g_sema)))
        {
          syslog(LOG_INFO, "ret: %d \n", ret);
          assert_int_equal(1, 0);
        }

      g_testmutexretval = pthread_mutex_unlock(g_mtx);
      syslog(LOG_INFO, "g_testmutexretval: %d \n", g_testmutexretval);
      assert_int_equal(g_testmutexretval, EPERM);

      if (i == 0)
        {
          syslog(LOG_INFO, "g_testmutexcount: %d \n", g_testmutexcount);
          assert_int_equal(g_testmutexcount, 1);
        }

      if (i == 1)
        {
          syslog(LOG_INFO, "g_testmutexcount: %d \n", g_testmutexcount);
          assert_int_equal(g_testmutexcount,
                           3); /* 3, Here, assert the g_testmutexcount. */
        }

      if ((ret = sem_post(&g_semb)))
        {
          syslog(LOG_INFO, "ret: %d \n", ret);
          assert_int_equal(1, 0);
        }

      if ((ret = pthread_join(thr, &thret)))
        {
          syslog(LOG_INFO, "ret: %d \n", ret);
          assert_int_equal(1, 0);
        }

      if (i == 0)
        {
          syslog(LOG_INFO, "g_testmutexcount: %d \n", g_testmutexcount);
          assert_int_equal(g_testmutexcount,
                           2); /* 2, Here, assert the g_testmutexcount. */
        }

      if (i == 1)
        {
          syslog(LOG_INFO, "g_testmutexcount: %d \n", g_testmutexcount);
          assert_int_equal(g_testmutexcount,
                           4); /* 4, Here, assert the g_testmutexcount. */
        }

      tabres[i][0] = g_testmutexretval;
    }

  if (tabres[0][0] != tabres[1][0])
    {
      assert_int_equal(1, 0);
    }

  /* We start with testing the NULL mutex features */

  (void)pthread_mutexattr_destroy(&mattr);
  ret = pthread_mutex_destroy(&g_mtxnull);
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, 0);

  ret = pthread_mutex_destroy(&g_mtxdef);
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, 0);

  ret = sem_destroy(&g_sema);
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, 0);

  ret = sem_destroy(&g_semb);
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, 0);
}
