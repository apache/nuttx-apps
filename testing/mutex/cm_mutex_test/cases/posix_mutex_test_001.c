/****************************************************************************
 * apps/testing/testsuites/kernel/mutex/cases/posix_mutex_test_001.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 *The ASF licenses this file to you under the Apache License, Version 2.0
 *(the "License"); you may not use this file except in compliance with
 *the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 *WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 *implied.  See the License for the specific language governing
 *permissions and limitations under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/config.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdint.h>
#include "MutexTest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_mutex_test01
 ****************************************************************************/

void test_nuttx_mutex_test01(FAR void **state)
{
  pthread_mutexattr_t mta;
  int rc;

  /* Initialize a mutex attributes object */

  rc = pthread_mutexattr_init(&mta);
  syslog(LOG_INFO, "rc : %d\n", rc);
  assert_int_equal(rc, 0);

  rc = pthread_mutexattr_destroy(&mta);
  syslog(LOG_INFO, "rc : %d\n", rc);
  assert_int_equal(rc, 0);
}
