/****************************************************************************
 * apps/testing/testsuites/kernel/sched/cases/api_task_test_005.c
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
#include <stdio.h>
#include <syslog.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "SchedTest.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: schedtask05routine
 ****************************************************************************/

static int schedtask05routine(int argc, char *argv[])
{
  int ret;
  int i;
  for (i = 1; i <= 10; i++)
    {
      if (i >= 4 && i <= 7)
        {
          ret = sched_yield();
          assert_int_equal(ret, OK);
        }
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_sched_task05
 ****************************************************************************/

void test_nuttx_sched_task05(FAR void **state)
{
  pid_t pid;
  int status;

  pid = task_create("schedtask05routine", TASK_PRIORITY,
                    DEFAULT_STACKSIZE, schedtask05routine, NULL);
  assert_true(pid > 0);
  waitpid(pid, &status, 0);
}
