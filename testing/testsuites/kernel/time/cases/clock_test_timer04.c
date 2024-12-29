/****************************************************************************
 * apps/testing/testsuites/kernel/time/cases/clock_test_timer04.c
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
 * Pre-processor Definitions
 ****************************************************************************/

/* signo should be in the range SIGRTMIN to SIGRTMAX when SA_SIGINFO flag
 * is set.
 */

#define SIG SIGRTMIN
#define CLOCKID CLOCK_REALTIME

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int test_timer04_g_handlerflag;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sighandler
 ****************************************************************************/

static void sighandler(int sig, siginfo_t *si, void *uc)
{
  if (si == NULL)
    {
      syslog(LOG_ERR, "sig %d, si %p, uc %p\n", sig, si, uc);
      return;
    }

  test_timer04_g_handlerflag++;
  syslog(LOG_INFO, "sig %d, si %p, uc %p\n", sig, si, uc);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_clock_test_timer04
 ****************************************************************************/

void test_nuttx_clock_test_timer04(FAR void **state)
{
  int interval = 3; /* 3 seconds */
  timer_t timerid;
  struct sigevent sev;
  struct itimerspec its;
  sigset_t mask;
  struct sigaction sa;
  int ret;

  /* Install handler for timer signal. */

  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = sighandler;
  sigemptyset(&sa.sa_mask);
  ret = sigaction(SIG, &sa, NULL);
  syslog(LOG_INFO, "sigaction %d: %d", SIG, ret);
  assert_int_equal(ret, 0);

  /* Block timer signal */

  sigemptyset(&mask);
  sigaddset(&mask, SIG);
  ret = sigprocmask(SIG_BLOCK, &mask, NULL);
  syslog(LOG_INFO, "sigaction %d: %d", SIG, ret);
  assert_int_equal(ret, 0);

  /* Create the timer */

  sev.sigev_notify = SIGEV_SIGNAL;
  sev.sigev_signo = SIG;
  sev.sigev_value.sival_ptr = &timerid;
  ret = timer_create(CLOCKID, &sev, &timerid);
  syslog(LOG_INFO, "timer_create %p: %d", timerid, ret);
  assert_int_equal(ret, 0);

  /* Start the timer */

  its.it_value.tv_sec = 0;
  its.it_value.tv_nsec = 990000000; /* 990000000, 0.99s */
  its.it_interval.tv_sec = its.it_value.tv_sec;
  its.it_interval.tv_nsec = its.it_value.tv_nsec;

  ret = timer_settime(timerid, 0, &its, NULL);
  syslog(LOG_INFO, "timer_create %p: %d", timerid, ret);
  assert_int_equal(ret, 0);

  /* Sleep for a while */

  syslog(LOG_INFO, "sleep %ds", interval);
  sleep(interval); /* should not be interrupted */

  /* Get the timer's time */

  ret = timer_gettime(timerid, &its);
  syslog(LOG_INFO, "timer_gettime %p: %d", timerid, ret);
  assert_int_equal(ret, 0);

  /* Unlock the timer signal */

  ret = sigprocmask(SIG_UNBLOCK, &mask, NULL);
  syslog(LOG_INFO, "sigprocmask unblock %d: %d", SIG, ret);
  assert_int_equal(ret, 0);

  syslog(LOG_INFO, "sleep another %ds", interval);
  sleep(interval); /* should be interrupted */
  syslog(LOG_INFO, "sleep time over, g_handlerflag = %d",
         test_timer04_g_handlerflag);

  ret = timer_delete(timerid);
  syslog(LOG_INFO, "timer_delete %p %d", timerid, ret);
  assert_int_equal(ret, 0);

  assert_int_not_equal(test_timer04_g_handlerflag, 0);
}
