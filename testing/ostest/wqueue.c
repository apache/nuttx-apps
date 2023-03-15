/****************************************************************************
 * apps/testing/ostest/wqueue.c
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

#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>

#include <nuttx/wqueue.h>

#if defined(CONFIG_SCHED_LPWORK) || defined(CONFIG_SCHED_HPWORK)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SLEEP_TIME   (100 * 1000)
#define TEST_COUNT   (100)
#define VERIFY_COUNT (100)

#ifdef CONFIG_SCHED_LPWORK
#  define TEST_QUEUE           LPWORK
#  define TEST_QUEUE_PRIORITY  CONFIG_SCHED_LPWORKPRIORITY
#else
#  define TEST_QUEUE           HPWORK
#  define TEST_QUEUE_PRIORITY  CONFIG_SCHED_HPWORKPRIORITY
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void empty_worker(FAR void *arg)
{
}

static void sleep_worker(FAR void *arg)
{
  FAR sem_t *sem = arg;
  usleep(SLEEP_TIME);
  sem_post(sem);
}

static void count_worker(FAR void *arg)
{
  FAR sem_t *sem = arg;
  sem_post(sem);
}

static FAR void *tester(FAR void *arg)
{
  int interval = (intptr_t)arg;
  int i;
  struct work_s work;

  memset(&work, 0, sizeof(work));
  for (i = 0; i < TEST_COUNT; i++)
    {
      work_queue(TEST_QUEUE, &work, empty_worker, NULL, 0);
      work_cancel(TEST_QUEUE, &work);
      usleep(interval);
    }

  usleep(SLEEP_TIME); /* Wait for workers to run. */
  return NULL;
}

static FAR void *verifier(FAR void *arg)
{
  sem_t sem;
  sem_t call_sem;
  int call_count;
  int i;
  struct work_s work[VERIFY_COUNT + 1];

  sem_init(&sem, 0, 0);
  sem_init(&call_sem, 0, 0);
  memset(&work, 0, sizeof(work));

  /* Queue sleep worker. */

  work_queue(TEST_QUEUE, &work[0], sleep_worker, &sem, 0);

  /* Queue count workers when TEST_QUEUE is busy. */

  for (i = 1; i <= VERIFY_COUNT; i++)
    {
      work_queue(TEST_QUEUE, &work[i], count_worker, &call_sem, 0);
    }

  /* Wait for sleep worker to run. */

  sem_wait(&sem);

  /* Wait for count workers to run. */

  usleep(SLEEP_TIME);

  sem_getvalue(&call_sem, &call_count);
  printf("wqueue_test: call = %d, expect = %d\n", call_count, VERIFY_COUNT);

  for (i = 0; i < VERIFY_COUNT; i++)
    {
      ASSERT(work[i].worker == NULL);
    }

  ASSERT(call_count == VERIFY_COUNT);
  return NULL;
}

static void run_once(int interval, int priority_test, int priority_verify)
{
  pthread_t thread;
  pthread_attr_t attr;
  struct sched_param sparam;

  pthread_attr_init(&attr);
  memset(&sparam, 0, sizeof(sparam));

  /* Tester: try race conditions. */

  sparam.sched_priority = priority_test;
  pthread_attr_setschedparam(&attr, &sparam);

  pthread_create(&thread, &attr, tester, (FAR void *)(intptr_t)interval);
  pthread_join(thread, NULL);

  /* Verifier: make sure queue is still working properly. */

  sparam.sched_priority = priority_verify;
  pthread_attr_setschedparam(&attr, &sparam);
  pthread_attr_setstacksize(&attr,
        VERIFY_COUNT * sizeof(struct work_s) + CONFIG_PTHREAD_STACK_DEFAULT);

  pthread_create(&thread, &attr, verifier, NULL);
  pthread_join(thread, NULL);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void wqueue_test(void)
{
  int interval;
  int priority_test;
  int priority_verify;

  for (interval = 0; interval <= 1; interval++)
    {
      for (priority_test  = TEST_QUEUE_PRIORITY - 1;
           priority_test <= TEST_QUEUE_PRIORITY + 1;
           priority_test++)
        {
          for (priority_verify  = TEST_QUEUE_PRIORITY - 1;
               priority_verify <= TEST_QUEUE_PRIORITY + 1;
               priority_verify++)
            {
              run_once(interval, priority_test, priority_verify);
            }
        }
    }
}

#endif /* CONFIG_SCHED_LPWORK || CONFIG_SCHED_HPWORK */
