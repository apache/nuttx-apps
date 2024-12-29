/****************************************************************************
 * apps/testing/testsuites/kernel/pthread/cases/posix_pthread_test_004.c
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

#include "PthreadTest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

static void *threadf01(void *arg)
{
  pthread_exit((void *)2); /* 2, here set value of the exit status. */
  return NULL;
}

/****************************************************************************
 * Name: test_nuttx_pthread_test04
 ****************************************************************************/

void test_nuttx_pthread_test04(FAR void **state)
{
  pthread_t mainth;
  pthread_t newth;
  UINT32 ret;
  UINTPTR temp;

  if (pthread_create(&newth, NULL, threadf01, NULL) != 0)
    {
      printf("Error creating thread\n");
      assert_int_equal(1, 0);
    }

  usleep(1000);

  /* los_taskdelay(1); */

  /* Obtain the thread ID of this main function */

  mainth = testpthreadself();

  /* Compare the thread ID of the new thread to the main thread.
   * They should be different.  If not, the test fails.
   */

  if (pthread_equal(newth, mainth) != 0)
    {
      printf("Test FAILED: A new thread wasn't created\n");
      assert_int_equal(1, 0);
    }

  usleep(1000);

  /* testextrataskdelay(1); */

  ret = pthread_join(newth, (void *)&temp);
  syslog(LOG_INFO, "ret: %d \n", ret);
  syslog(LOG_INFO, "temp: %ld \n", temp);
  assert_int_equal(ret, 0);
  assert_int_equal(temp, 2); /* 2, here assert the result. */
}
