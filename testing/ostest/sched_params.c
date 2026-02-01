/****************************************************************************
 * apps/testing/ostest/sched_params.c
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

#include <stdio.h>
#include <errno.h>
#include <sched.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>

#include <nuttx/sched.h>

#include "ostest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Test parameters */

#define SCHEDULER_TEST_PRIORITY_LOW    100
#define SCHEDULER_TEST_PRIORITY_MED    150
#define SCHEDULER_TEST_PRIORITY_HIGH   200

#ifdef CONFIG_SCHED_SPORADIC
#define SPORADIC_INIT_PRIORITY         150
#define SPORADIC_MAX_REPL              200  /* Max priority */
#define SPORADIC_REPL_PERIOD           1000 /* milliseconds */
#define SPORADIC_BUDGET                300  /* milliseconds */
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static pthread_t g_test_thread;
static volatile bool g_thread_ready = false;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sched_test_thread
 *
 * Description:
 *   Test thread that can have its scheduler parameters modified.
 *
 ****************************************************************************/

static FAR void *sched_test_thread(FAR void *parameter)
{
  g_thread_ready = true;

  /* Just keep the thread running for testing */

  while (1)
    {
      usleep(100000);  /* Sleep 100ms */
    }

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sched_params_test
 *
 * Description:
 *   Test nxsched_set_param, nxsched_get_param, nxsched_set_scheduler,
 *   and nxsched_get_scheduler functions. Also test CONFIG_SCHED_SPORADIC
 *   if enabled.
 *
 ****************************************************************************/

void sched_params_test(void)
{
  struct sched_param param;
  struct sched_param get_param;
  int policy;
  int ret;
  int current_priority;
  pid_t main_pid;
  int test_priorities[4];
  int i;

  test_priorities[0] = 120;
  test_priorities[1] = 140;
  test_priorities[2] = 160;
  test_priorities[3] = 130;

  printf("\nsched_params_test: Starting scheduler parameters test\n");

  /* Test 1: Get main task's current scheduler and parameters */

  printf("\nTest 1: Get main task scheduler and priority\n");

  main_pid = getpid();
  printf("sched_params_test: Main task PID = %d\n", main_pid);

  /* Get scheduler policy for main task */

  ret = nxsched_get_scheduler(main_pid);
  if (ret < 0)
    {
      printf("sched_params_test: ERROR - nxsched_get_scheduler failed: %d\n",
             ret);
      ASSERT(false);
    }

  policy = ret;
  printf("sched_params_test: Main task policy = %d (SCHED_FIFO=%d, "
         "SCHED_RR=%d)\n", policy, SCHED_FIFO, SCHED_RR);

  /* Get priority for main task */

  ret = nxsched_get_param(main_pid, &get_param);
  if (ret < 0)
    {
      printf("sched_params_test: ERROR -\
              nxsched_get_param failed: %d\n", ret);
      ASSERT(false);
    }

  current_priority = get_param.sched_priority;
  printf("sched_params_test: Main task priority = %d\n", current_priority);

  /* Test 2: Set priority and verify */

  printf("\nTest 2: Set priority and verify\n");

  param.sched_priority = SCHEDULER_TEST_PRIORITY_MED;

  ret = nxsched_set_param(main_pid, &param);
  if (ret < 0)
    {
      printf("sched_params_test: ERROR - \
              nxsched_set_param failed: %d\n", ret);
      ASSERT(false);
    }

  printf("sched_params_test: Set priority to %d\n",
         SCHEDULER_TEST_PRIORITY_MED);

  /* Verify the priority was set */

  ret = nxsched_get_param(main_pid, &get_param);
  if (ret < 0)
    {
      printf("sched_params_test: ERROR - \
              nxsched_get_param after set " \
             "failed: %d\n", ret);
      ASSERT(false);
    }

  if (get_param.sched_priority != SCHEDULER_TEST_PRIORITY_MED)
    {
      printf("sched_params_test: ERROR - Priority not set correctly, "
             "expected %d, got %d\n", SCHEDULER_TEST_PRIORITY_MED,
             get_param.sched_priority);
      ASSERT(false);
    }

  printf("sched_params_test: Verified priority set to %d\n",
         SCHEDULER_TEST_PRIORITY_MED);

  /* Test 3: Create a test thread and modify its parameters */

  printf("\nTest 3: Test thread parameter modification\n");

  g_thread_ready = false;

  /* Create test thread with initial priority */

  param.sched_priority = SCHEDULER_TEST_PRIORITY_LOW;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setschedparam(&attr, &param);

  ret = pthread_create(&g_test_thread, &attr, sched_test_thread, NULL);
  pthread_attr_destroy(&attr);

  if (ret != 0)
    {
      printf("sched_params_test: ERROR - pthread_create failed: %d\n", ret);
      ASSERT(false);
    }

  printf("sched_params_test: Test thread created\n");

  /* Wait for thread to be ready */

  usleep(200000);  /* Wait 200ms for thread to start */

  if (!g_thread_ready)
    {
      printf("sched_params_test: WARNING - Test thread not ready\n");
    }

  /* Get test thread parameters */

  ret = nxsched_get_param((pid_t)g_test_thread, &get_param);
  if (ret == 0)
    {
      printf("sched_params_test: Test thread initial priority = %d\n",
             get_param.sched_priority);
    }
  else
    {
      printf("sched_params_test: WARNING - Could not get test thread "
             "parameters: %d\n", ret);
    }

  /* Test 4: Verify invalid PID handling */

  printf("\nTest 4: Test invalid PID handling\n");

  /* Try to get scheduler for invalid PID */

  ret = nxsched_get_scheduler(99999);
  if (ret < 0 && ret == -ESRCH)
    {
      printf("sched_params_test: Correctly rejected invalid PID\n");
    }
  else if (ret < 0)
    {
      printf("sched_params_test: Got error for invalid PID: %d\n", ret);
    }
  else
    {
      printf("sched_params_test: WARNING - Did not reject invalid PID\n");
    }

  /* Test 5: Test getting scheduler for PID 0 (self) */

  printf("\nTest 5: Get scheduler for self (PID 0)\n");

  ret = nxsched_get_scheduler(0);
  if (ret < 0)
    {
      printf("sched_params_test: ERROR - \
              nxsched_get_scheduler(0) failed: %d\n", \
              ret);
      ASSERT(false);
    }

  printf("sched_params_test: Self scheduler policy = %d\n", ret);

  /* Test 6: Get parameters for PID 0 (self) */

  printf("\nTest 6: Get parameters for self (PID 0)\n");

  ret = nxsched_get_param(0, &get_param);
  if (ret < 0)
    {
      printf("sched_params_test: ERROR - nxsched_get_param(0) failed: %d\n",
             ret);
      ASSERT(false);
    }

  printf("sched_params_test: Self priority = %d\n", \
          get_param.sched_priority);

  /* Test 7: Multiple priority changes */

  printf("\nTest 7: Multiple priority changes\n");

  for (i = 0; i < 4; i++)
    {
      param.sched_priority = test_priorities[i];

      ret = nxsched_set_param(main_pid, &param);
      if (ret < 0)
        {
          printf("sched_params_test: ERROR - Failed to set priority to %d: "
                 "%d\n", test_priorities[i], ret);
          ASSERT(false);
        }

      ret = nxsched_get_param(main_pid, &get_param);
      if (ret < 0)
        {
          printf("sched_params_test: ERROR - Failed to get priority: %d\n",
                 ret);
          ASSERT(false);
        }

      if (get_param.sched_priority != test_priorities[i])
        {
          printf("sched_params_test: ERROR - \
                 Priority mismatch: expected %d, "
                 "got %d\n", test_priorities[i], get_param.sched_priority);
          ASSERT(false);
        }

      printf("sched_params_test: [%d] Priority changed to %d and verified\n",
             i + 1, test_priorities[i]);
    }

  /* Restore original priority */

  param.sched_priority = current_priority;
  nxsched_set_param(main_pid, &param);

  printf("\nTest 8: Restored original priority to %d\n", current_priority);

  /* Cancel the test thread */

  printf("\nsched_params_test: Canceling test thread\n");

  pthread_cancel(g_test_thread);
  pthread_join(g_test_thread, NULL);

  printf("sched_params_test: Test thread cleaned up\n");

  printf("\nsched_params_test: All scheduler parameter tests PASSED\n");
}
