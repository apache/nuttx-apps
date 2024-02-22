/****************************************************************************
 * apps/testing/testsuites/kernel/pthread/cases/posix_pthread_test_018.c
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
#include <semaphore.h>
#include "PthreadTest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

static sem_t re_pthread_f01;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

static void *pthread_f01(void *argument)
{
  g_test_pthread_count++;
  sem_wait(&re_pthread_f01);

  return argument;
}

/****************************************************************************
 * Name: TestNuttxPthreadTest18
 ****************************************************************************/

void test_nuttx_pthread_test18(FAR void **state)
{
  pthread_attr_t attr;
  pthread_t new_th;
  UINT32 ret;
  UINTPTR temp;
  int policy;
  struct sched_param param;
  struct sched_param param2 =
    {
      2
    }; /* 2, init */

  g_test_pthread_count = 0;

  ret = sem_init(&re_pthread_f01, 0, 0);
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, 0);

  ret = pthread_attr_init(&attr);
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, 0);

  ret = pthread_create(&new_th, NULL, pthread_f01, (void *)9); /* 9, test param of the function. */
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, 0);
  usleep(1000);

  /* LOS_TaskDelay(1); */

  syslog(LOG_INFO, "g_testPthreadCount: %d \n", g_test_pthread_count);
  assert_int_equal(g_test_pthread_count, 1);

  ret = pthread_setschedparam(new_th, -1, &param);
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, EINVAL);

  ret = pthread_setschedparam(new_th, 100, &param); /* 4, test for invalid param. */
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, EINVAL);

#if CONFIG_RR_INTERVAL > 0
  param.sched_priority = 31; /* 31, init */
  ret = pthread_setschedparam(new_th, SCHED_RR, &param);
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, 0);
#endif

  ret = pthread_getschedparam(new_th, NULL, &param2);
  syslog(LOG_INFO, "ret: %d \n", ret);
  syslog(LOG_INFO, "param2.sched_priority: %d \n", param2.sched_priority);
  assert_int_equal(ret, EINVAL);
  assert_int_equal(param2.sched_priority, 2); /* 2, here assert the result. */

  ret = pthread_getschedparam(new_th, &policy, NULL);
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, EINVAL);

  ret = pthread_getschedparam(new_th, &policy, &param2);
  syslog(LOG_INFO, "ret: %d \n", ret);
  syslog(LOG_INFO, "param2.sched_priority: %d \n", param2.sched_priority);
  assert_int_equal(ret, 0);
#if CONFIG_RR_INTERVAL > 0
  assert_int_equal(param2.sched_priority, 31); /* 31, here assert the result. */
#endif

  ret = sem_post(&re_pthread_f01);
  syslog(LOG_INFO, "ret of sem_post: %d \n", ret);
  assert_int_equal(ret, 0);
  ret = pthread_join(new_th, (void *)&temp);
  syslog(LOG_INFO, "ret: %d \n", ret);
  syslog(LOG_INFO, "temp: %ld \n", temp);
  assert_int_equal(ret, 0);
  assert_int_equal(temp, 9); /* 9, here assert the result. */

#if CONFIG_RR_INTERVAL > 0
  ret = pthread_setschedparam(new_th + 9, SCHED_RR, &param); /* 9, test for invalid param. */
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, ESRCH);
#endif

  ret = pthread_getschedparam(new_th + 8, &policy, &param2); /* 8, test for invalid param. */
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, ESRCH);

  ret = pthread_attr_destroy(&attr);
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, 0);

  ret = sem_destroy(&re_pthread_f01);
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, 0);
}
