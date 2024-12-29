/****************************************************************************
 * apps/testing/testsuites/kernel/pthread/cases/posix_pthread_test_021.c
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

/****************************************************************************
 * Name: test_nuttx_pthread_test21
 ****************************************************************************/

void test_nuttx_pthread_test21(FAR void **state)
{
  UINT32 ret;
  int oldstate;
  int oldstype;

  ret =
      pthread_setcancelstate(2, &oldstate); /* 2, test for invalid param. */
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, EINVAL);

  ret =
      pthread_setcancelstate(3, &oldstate); /* 3, test for invalid param. */
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, EINVAL);

  ret = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, 0);

  ret = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, 0);
  syslog(LOG_INFO, "oldstate: %d \n", oldstate);
  assert_int_equal(oldstate, PTHREAD_CANCEL_ENABLE);

  ret =
      pthread_setcanceltype(2, &oldstype); /* 2, test for invalid param. */
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, EINVAL);

  ret =
      pthread_setcanceltype(3, &oldstype); /* 3, test for invalid param. */
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, EINVAL);

  ret = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  syslog(LOG_INFO, "ret: %d \n", ret);
  assert_int_equal(ret, 0);

#ifdef CONFIG_CANCELLATION_POINTS
  ret = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldstate);
  syslog(LOG_INFO, "ret: %d \n", ret);
  syslog(LOG_INFO, "oldstate: %d \n", oldstate);
  assert_int_equal(ret, 0);
  assert_int_equal(oldstate, PTHREAD_CANCEL_ASYNCHRONOUS);
#endif
}
