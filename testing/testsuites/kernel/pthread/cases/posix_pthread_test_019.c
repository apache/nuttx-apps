/****************************************************************************
 * apps/testing/testsuites/kernel/pthread/cases/posix_pthread_test_019.c
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

static void pthreadoncef01(void)
{
  g_testpthreadcount++;
}

/****************************************************************************
 * Name: test_nuttx_pthread_test19
 ****************************************************************************/

void test_nuttx_pthread_test19(FAR void **state)
{
  UINT32 ret;
  pthread_once_t onceblock = PTHREAD_ONCE_INIT;

  g_testpthreadcount = 0;

  ret = pthread_once(NULL, pthreadoncef01);
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, EINVAL);

  ret = pthread_once(&onceblock, NULL);
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, EINVAL);

  ret = pthread_once(&onceblock, pthreadoncef01);
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, 0);
  syslog(LOG_INFO, "g_testpthreadcount: %d \n", g_testpthreadcount);
  assert_int_equal(g_testpthreadcount, 1);

  ret = pthread_once(&onceblock, pthreadoncef01);
  syslog(LOG_INFO, "ret: %d \n", ret);
  syslog(LOG_INFO, "g_testpthreadcount: %d \n", g_testpthreadcount);
  assert_int_equal(ret, 0);
  assert_int_equal(g_testpthreadcount, 1);
}
