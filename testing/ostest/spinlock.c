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
#include <stdatomic.h>

#include "ostest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define READER_FLAG (1 << 0)
#define WRITER_FLAG (1 << 1)

/****************************************************************************
 * Private Data
 ****************************************************************************/

static spinlock_t lock = SP_UNLOCKED;

static pthread_t g_thread1;
static pthread_t g_thread2;

#ifdef CONFIG_RW_SPINLOCK
static pthread_t g_thread3;
static rwlock_t rw_lock = RW_SP_UNLOCKED;

/* To distinguish to write or read (can be 0, 1, 2) */

static int test_rw_flag = 0;
#endif

static int g_result = 0;

static FAR void *thread_native_spinlock(FAR FAR void *parameter)
{
  int pid = *(FAR int *)parameter;

  for (int i = 0; i < 10; i++)
    {
      printf("pid %d try to get lock g_result:%d\n", pid, g_result);
      spin_lock(&lock);
      g_result++;
      spin_unlock(&lock);
      printf("pid %d release lock g_result:%d\n", pid, g_result);
    }

  return NULL;
}

static FAR void test_native_spinlock(void)
{
  int status;
  g_result = 0;
  lock = SP_UNLOCKED;
  spin_initialize(&lock, SP_UNLOCKED);

  status = pthread_create(&g_thread1, NULL,
                          thread_native_spinlock, &g_thread1);
  if (status != 0)
    {
      printf("spinlock_test: ERROR pthread_create failed, status=%d\n",
              status);
      ASSERT(false);
    }

  status = pthread_create(&g_thread2, NULL,
                          thread_native_spinlock, &g_thread2);
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

#if defined(CONFIG_RW_SPINLOCK)
static void FAR *thread_read_spinlock(FAR void *parameter)
{
  static atomic_int reader_counter = 0;
  int pid = *(FAR int *)parameter;
  int test;

  for (int i = 0; i < 10; ++i)
    {
      printf("pid %d try to get read lock g_result:%d\n", pid, g_result);
      atomic_fetch_add(&reader_counter, 1);
      read_lock(&rw_lock);
      test = g_result + 1;
      test_rw_flag |= READER_FLAG;

      /* Only reader can in critical section */

      ASSERT(test_rw_flag == READER_FLAG);
      read_unlock(&rw_lock);
      atomic_fetch_sub(&reader_counter, 1);
      if (reader_counter == 0)
        {
          test_rw_flag &= ~READER_FLAG;
        }
      printf("pid %d release read lock g_result+1:%d\n", pid, test);
    }

  return NULL;
}

static void FAR *thread_wrt_spinlock(FAR void *parameter)
{
  static atomic_int writer_counter = 0;
  int pid = *(FAR int *)parameter;

  for (int i = 0; i < 10; ++i)
    {
      printf("pid %d try to get write lock g_result:%d\n", pid, g_result);
      atomic_fetch_add(&writer_counter, 1);
      write_lock(&rw_lock);

      g_result++;
      test_rw_flag |= WRITER_FLAG;

      /* Only wrtier can in critical section */

      ASSERT(test_rw_flag == WRITER_FLAG && writer_counter == 1);
      test_rw_flag &= ~WRITER_FLAG;
      write_unlock(&rw_lock);
      atomic_fetch_sub(&writer_counter, 1);
      printf("pid %d release write lock g_result:%d\n", pid, g_result);
    }
  write_trylock(&rw_lock);
  return NULL;
}

static FAR void test_rw_spinlock(void)
{
  int status;
  g_result = 0;
  rwlock_init(&rw_lock);

  status = pthread_create(&g_thread1, NULL,
                          thread_read_spinlock, &g_thread1);
  if (status != 0)
    {
      printf("spinlock_test: ERROR pthread_create failed, status=%d\n",
              status);
      ASSERT(false);
    }

  status = pthread_create(&g_thread2, NULL,
                          thread_read_spinlock, &g_thread2);
  if (status != 0)
    {
      printf("spinlock_test: ERROR pthread_create failed, status=%d\n",
              status);
      ASSERT(false);
    }

  status = pthread_create(&g_thread3, NULL,
                          thread_wrt_spinlock, &g_thread3);
  if (status != 0)
    {
      printf("spinlock_test: ERROR pthread_create failed, status=%d\n",
              status);
      ASSERT(false);
    }

  pthread_join(g_thread1, NULL);
  pthread_join(g_thread2, NULL);
  pthread_join(g_thread3, NULL);

  assert(g_result == 10);
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void spinlock_test(void)
{
  test_native_spinlock();

#if defined(CONFIG_RW_SPINLOCK)
  test_rw_spinlock();
#endif
}
