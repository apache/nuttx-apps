/****************************************************************************
 * apps/testing/testsuites/kernel/pthread/cases/posix_pthread_test_009.c
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

static void *pthread_f01(void *arg)
{
  g_test_pthread_count++;

  pthread_exit(NULL);

  return (void *)9; /* 9, here set value about the return status. */
}

/****************************************************************************
 * Name: TestNuttxPthreadTest09
 ****************************************************************************/

void test_nuttx_pthread_test09(FAR void **state)
{
  pthread_t new_th;
  UINT32 ret;
  UINTPTR temp = 1;

  /* _pthread_data *joinee = NULL; */

  g_test_pthread_count = 0;
  g_test_pthread_task_max_num = 128;

  ret = pthread_create(&new_th, NULL, pthread_f01, NULL);
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, 0);
  usleep(1000);

  /* LOS_TaskDelay(1); */

  syslog(LOG_INFO, "g_testPthreadCount: %d \n", g_test_pthread_count);
  assert_int_equal(g_test_pthread_count, 1);

  ret = pthread_join(g_test_pthread_task_max_num, (void *)&temp);
  syslog(LOG_INFO, "ret: %d \n", ret);
  syslog(LOG_INFO, "temp: %ld \n", temp);
  assert_int_equal(ret, ESRCH);
  assert_int_equal(temp, 1);

  ret = pthread_join(LOSCFG_BASE_CORE_TSK_CONFIG + 1, (void *)&temp);
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, ESRCH);
  syslog(LOG_INFO, "temp: %ld \n", temp);
  assert_int_equal(temp, 1);

  ret = pthread_detach(g_test_pthread_task_max_num);
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, ESRCH);

  ret = pthread_join(new_th, NULL);
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, 0);

  sleep(1);
  ret = pthread_detach(new_th);
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, ESRCH);

  ret = pthread_join(new_th, NULL);
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, ESRCH);
}
