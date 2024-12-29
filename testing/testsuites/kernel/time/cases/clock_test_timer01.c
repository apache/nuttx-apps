/****************************************************************************
 * apps/testing/testsuites/kernel/time/cases/clock_test_timer01.c
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
#include "TimeTest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SIG SIGALRM
#define CLOCKID CLOCK_REALTIME

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int test_timer01_g_sighdlcnt = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sighandler01
 ****************************************************************************/

static void sighandler01(int sig)
{
  test_timer01_g_sighdlcnt++;
  syslog(LOG_INFO, "signo = %d test_timer01_g_sighdlcnt = %d\n", sig,
         test_timer01_g_sighdlcnt);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_clock_test_timer01
 ****************************************************************************/

void test_nuttx_clock_test_timer01(FAR void **state)
{
  int interval = 3; /* 3, seconds */
  timer_t timerid01;
  timer_t timerid02;
  struct sigevent sev;
  struct itimerspec its;
  sigset_t mask;
  struct sigaction sa;
  int ret;

  sa.sa_flags = 0;
  sa.sa_handler = sighandler01;
  sigemptyset(&sa.sa_mask);
  ret = sigaction(SIG, &sa, NULL);
  syslog(LOG_INFO, "sigaction %d: %d", SIG, ret);
  assert_int_equal(ret, 0);

  /* Block timer signal */

  sigemptyset(&mask);
  sigaddset(&mask, SIG);
  ret = sigprocmask(SIG_BLOCK, &mask, NULL);
  syslog(LOG_INFO, "sigprocmask setmask %d: %d", SIG, ret);
  assert_int_equal(ret, 0);

  /* Create the timer */

  sev.sigev_notify = SIGEV_SIGNAL;
  sev.sigev_signo = SIG;
  sev.sigev_value.sival_ptr = &timerid01;
  ret = timer_create(CLOCKID, &sev, &timerid01);
  syslog(LOG_INFO, "timer_create %p: %d", timerid01, ret);
  assert_int_equal(ret, 0);

  /* Start the timer */

  its.it_value.tv_sec = 0;
  its.it_value.tv_nsec = 900000000; /* 900000000, 0.9s */
  its.it_interval.tv_sec = its.it_value.tv_sec;
  its.it_interval.tv_nsec = its.it_value.tv_nsec;

  ret = timer_settime(timerid01, 0, &its, NULL);
  syslog(LOG_INFO, "timer_settime %p: %d", timerid01, ret);
  assert_int_equal(ret, 0);

  /* Test of evp is NULL */

  ret = timer_create(CLOCKID, NULL, &timerid02);
  syslog(LOG_INFO, "timer_settime %p: %d", timerid02, ret);
  assert_int_equal(ret, 0);

  its.it_value.tv_sec = 1;
  its.it_value.tv_nsec = 0;
  its.it_interval.tv_sec = its.it_value.tv_sec;
  its.it_interval.tv_nsec = its.it_value.tv_nsec;

  ret = timer_settime(timerid02, 0, &its, NULL);
  syslog(LOG_INFO, "timer_settime %p: %d", timerid02, ret);
  assert_int_equal(ret, 0);

  sleep(6);

  /* Sleep for a while */

  syslog(LOG_INFO, "sleep %ds", interval);

  /* timer signal is blocked,
   * this sleep should not be interrupted
   */

  sleep(interval);

  /* Get the timer's time */

  ret = timer_gettime(timerid01, &its);
  syslog(LOG_INFO, "timer_gettime %p: %d", timerid01, ret);
  assert_int_equal(ret, 0); /* get time success */

  syslog(LOG_INFO, "unblock signal %d", SIG);

  /* Unlock the timer signal */

  ret = sigprocmask(SIG_UNBLOCK, &mask, NULL);
  syslog(LOG_INFO, "sigprocmask unblock %d: %d", SIG, ret);
  assert_int_equal(ret, 0);

  interval = 1;
  syslog(LOG_INFO, "sleep another %ds", interval);
  sleep(interval); /* this sleep may be interrupted by the timer */
  syslog(LOG_INFO, "sleep time over, test_timer01_g_sighdlcnt = %d",
         test_timer01_g_sighdlcnt);

  syslog(LOG_INFO, "sleep another %ds", interval);
  sleep(interval); /* this sleep may be interrupted by the timer */
  syslog(LOG_INFO, "sleep time over, test_timer01_g_sighdlcnt = %d",
         test_timer01_g_sighdlcnt);

  ret = timer_delete(timerid01);
  syslog(LOG_INFO, "timer_delete %p %d", timerid01, ret);
  assert_int_equal(ret, 0);

  ret = timer_delete(timerid02);
  syslog(LOG_INFO, "timer_delete %p %d", timerid02, ret);
  assert_int_equal(ret, 0);

  assert_int_not_equal(test_timer01_g_sighdlcnt, 0);
}
