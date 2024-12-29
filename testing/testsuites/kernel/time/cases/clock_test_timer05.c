/****************************************************************************
 * apps/testing/testsuites/kernel/time/cases/clock_test_timer05.c
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
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <syslog.h>
#include <errno.h>
#include <stdlib.h>
#include <inttypes.h>
#include "TimeTest.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int test_timer05_g_sighdlcnt01;
static int test_timer05_g_sighdlcnt02;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tempsighandler
 ****************************************************************************/

static void tempsighandler(union sigval v)
{
  syslog(LOG_INFO, "This is tempsighandler ...\r\n");
  (*(void (*)(void))(v.sival_ptr))();
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tempsighandler01
 ****************************************************************************/

static void tempsighandler01(void)
{
  test_timer05_g_sighdlcnt01++;
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tempsighandler02
 ****************************************************************************/

static void tempsighandler02(void)
{
  test_timer05_g_sighdlcnt02++;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_clock_test_timer05
 ****************************************************************************/

void test_nuttx_clock_test_timer05(FAR void **state)
{
  timer_t timerid01;
  timer_t timerid02;
  struct sigevent sev;
  struct itimerspec its;
  int ret;
  char *p = NULL;

  p = memset(&sev, 0, sizeof(struct sigevent));
  assert_non_null(p);
  sev.sigev_notify = SIGEV_THREAD;
  sev.sigev_notify_function = tempsighandler;
  sev.sigev_value.sival_ptr = (void *)tempsighandler01;

  /* Start the timer */

  its.it_value.tv_sec = 3; /* 3, timer time 3 seconds. */
  its.it_value.tv_nsec = 0;
  its.it_interval.tv_sec = its.it_value.tv_sec;
  its.it_interval.tv_nsec = its.it_value.tv_nsec;
  ret = timer_create(CLOCK_REALTIME, &sev, &timerid01);
  syslog(LOG_INFO, "timer_settime %p: %d", timerid01, ret);
  assert_int_equal(ret, 0);

  ret = timer_settime(timerid01, 0, &its, NULL);
  syslog(LOG_INFO, "timer_create %p: %d", timerid01, ret);
  assert_int_equal(ret, 0);

  its.it_value.tv_sec = 4; /* 4, timer time 4 seconds. */
  its.it_value.tv_nsec = 0;
  its.it_interval.tv_sec = its.it_value.tv_sec;
  its.it_interval.tv_nsec = its.it_value.tv_nsec;

  sev.sigev_value.sival_ptr = (void *)tempsighandler02;
  ret = timer_create(CLOCK_REALTIME, &sev, &timerid02);
  syslog(LOG_INFO, "timer_settime %p: %d", timerid02, ret);
  assert_int_equal(ret, 0);

  ret = timer_settime(timerid02, 0, &its, NULL);
  syslog(LOG_INFO, "timer_settime %p: %d", timerid02, ret);
  assert_int_equal(ret, 0);

  its.it_value.tv_sec = 5; /* 5, timer time 5 seconds. */
  its.it_value.tv_nsec = 0;
  its.it_interval.tv_sec = its.it_value.tv_sec;
  its.it_interval.tv_nsec = its.it_value.tv_nsec;

  sleep(20); /* 20, sleep seconds for timer. */
  ret = timer_delete(timerid01);
  syslog(LOG_INFO, "timer_delete %p %d", timerid01, ret);
  assert_int_equal(ret, 0);

  ret = timer_delete(timerid02);
  syslog(LOG_INFO, "timer_delete %p %d", timerid02, ret);
  assert_int_equal(ret, 0);

  assert_int_not_equal(test_timer05_g_sighdlcnt01, 0);
  assert_int_not_equal(test_timer05_g_sighdlcnt02, 0);
}
