/****************************************************************************
 * apps/testing/testsuites/kernel/pthread/cases/posix_pthread_test_005.c
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
#include<time.h>
#include "PthreadTest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

static void *threadf01(void *arg)
{
  sleep(500);

  /* Shouldn't reach here.  If we do, then the pthread_cancel()
   * function did not succeed.
   */

  pthread_exit((void *)6);
  return NULL;
}

/****************************************************************************
 * Name: test_nuttx_pthread_test05
 ****************************************************************************/

void test_nuttx_pthread_test05(FAR void **state)
{
  pthread_t newth;
  UINT32 ret;
  UINTPTR temp;
  clock_t start, finish;
  double duration;

  start = clock();
  if (pthread_create(&newth, NULL, threadf01, NULL) < 0)
    {
      syslog(LOG_INFO, "Error creating thread\n");
      assert_int_equal(1, 0);
    }

  usleep(1000);

  /* los_taskdelay(1); */

  /* Try to cancel the newly created thread.  If an error is returned,
   * then the thread wasn't created successfully.
   */

  if (pthread_cancel(newth) != 0)
    {
      syslog(LOG_INFO, "Test FAILED: A new thread wasn't created\n");
      assert_int_equal(1, 0);
    }

  ret = pthread_join(newth, (void *)&temp);
  finish = clock();
  duration = (double)(finish - start) / CLOCKS_PER_SEC * 1000;
  syslog(LOG_INFO, "duration: %f \n", duration);
  assert_int_equal(duration < 500, 1);
  syslog(LOG_INFO, "ret: %d \n", ret);
  syslog(LOG_INFO, "temp: %ld \n", temp);
  assert_int_equal(ret, 0);
  assert_int_not_equal(temp, 6);
}
