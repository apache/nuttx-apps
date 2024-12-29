/****************************************************************************
 * apps/testing/testsuites/kernel/pthread/cases/posix_pthread_test_006.c
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
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

  sleep(1);

  pthread_exit((void *)0);
  return NULL;
}

/****************************************************************************
 * Name: test_nuttx_pthread_test06
 ****************************************************************************/

void test_nuttx_pthread_test06(FAR void **state)
{
  UINT32 ret;
  void *temp = NULL;
  pthread_t a;

  /* SIGALRM will be sent in 5 seconds. */

  /* Create a new thread. */

  if (pthread_create(&a, NULL, threadf01, NULL) != 0)
    {
      printf("Error creating thread\n");
      assert_int_equal(1, 0);
    }

  usleep(1000);

  /* los_taskdelay(1); */

  pthread_cancel(a);

  /* If 'main' has reached here, then the test passed because it means
   * that the thread is truly asynchronise, and main isn't waiting for
   * it to return in order to move on.
   */

  ret = pthread_join(a, &temp);
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, 0);
}
