/****************************************************************************
 * apps/testing/testsuites/kernel/pthread/cases/posix_pthread_test_003.c
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
  pthread_exit(NULL);
  return NULL;
}

/****************************************************************************
 * Name: test_nuttx_pthread_test03
 ****************************************************************************/

void test_nuttx_pthread_test03(FAR void **state)
{
  pthread_t athread;
  pthread_t ptid;
  pthread_t a = 0;
  pthread_t b = 0;
  int tmp;
  pthread_attr_t aa =
    {
      0
    };

  int detachstate;
  UINT32 ret;

  ptid = pthread_self();
  syslog(LOG_INFO, "ptid: %d \n", ptid);
  assert_int_not_equal(ptid, 0);
  pthread_create(&athread, NULL, threadf01, NULL);

  tmp = pthread_equal(a, b);
  syslog(LOG_INFO, "ret: %d\n", tmp);
  assert_int_not_equal(tmp, 0);

  pthread_attr_init(&aa);

  ret = pthread_attr_getdetachstate(&aa, &detachstate);
  syslog(LOG_INFO, "ret of getdetachstate: %d\n", ret);
  assert_int_equal(ret, 0);

  ret = pthread_attr_setdetachstate(&aa, PTHREAD_CREATE_DETACHED);
  syslog(LOG_INFO, "ret of setdetachstate: %d\n", ret);
  assert_int_equal(ret, 0);

  pthread_attr_destroy(&aa);

  ret = pthread_join(athread, NULL);
  syslog(LOG_INFO, "ret of pthread_join: %d\n", ret);
  assert_int_equal(ret, 0);
}
