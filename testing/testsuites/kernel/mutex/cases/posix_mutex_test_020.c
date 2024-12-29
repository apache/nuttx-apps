/****************************************************************************
 * apps/testing/testsuites/kernel/mutex/cases/posix_mutex_test_020.c
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
#include "MutexTest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_mutex_test20
 ****************************************************************************/

void test_nuttx_mutex_test20(FAR void **state)
{
  pthread_mutex_t mutex;
  int rc;

  /* Initialize a mutex object */

  rc = pthread_mutex_init(&mutex, NULL);
  syslog(LOG_INFO, "rc : %d\n", rc);
  assert_int_equal(rc, ENOERR);

  /* Acquire the mutex object using pthread_mutex_lock */

  if ((rc = pthread_mutex_lock(&mutex)))
    {
      syslog(LOG_INFO, "rc: %d \n", rc);
      goto EXIT1;
    }

  sleep(1);

  /* Release the mutex object using pthread_mutex_unlock */

  if ((rc = pthread_mutex_unlock(&mutex)))
    {
      syslog(LOG_INFO, "rc: %d \n", rc);
      goto EXIT2;
    }

  /* Destroy the mutex object */

  if ((rc = pthread_mutex_destroy(&mutex)))
    {
      syslog(LOG_INFO, "rc: %d \n", rc);
      goto EXIT1;
    }

  return;

EXIT2:
  pthread_mutex_unlock(&mutex);
  assert_int_equal(1, 0);

EXIT1:
  pthread_mutex_destroy(&mutex);
  assert_int_equal(1, 0);
  return;
}
