/****************************************************************************
 * apps/testing/ostest/spinlock.c
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

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <nuttx/spinlock.h>

#include "ostest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static spinlock_t lock = SP_UNLOCKED;

static pthread_t g_thread1;
static pthread_t g_thread2;

static int g_result = 0;

static FAR void thread_spinlock(FAR void *parameter)
{
  int pid = *(int *)parameter;

  for (int i = 0; i < 10; i++)
    {
      printf("pid %d get lock g_result:%d\n", pid, g_result);
      spin_lock(&lock);
      g_result++;
      spin_unlock(&lock);
      printf("pid %d release lock g_result:%d\n", pid, g_result);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void spinlock_test(void)
{
  int status;
  g_result = 0;

  status = pthread_create(&g_thread1, NULL,
                          (void *)thread_spinlock, &g_thread1);
  if (status != 0)
    {
      printf("spinlock_test: ERROR pthread_create failed, status=%d\n",
              status);
      ASSERT(false);
    }

  status = pthread_create(&g_thread2, NULL,
                          (void *)thread_spinlock, &g_thread2);
  if (status != 0)
    {
      printf("spinlock_test: ERROR pthread_create failed, status=%d\n",
              status);
      ASSERT(false);
    }

  pthread_join(g_thread1, NULL);
  pthread_join(g_thread2, NULL);

  assert(g_result == 20);
}
