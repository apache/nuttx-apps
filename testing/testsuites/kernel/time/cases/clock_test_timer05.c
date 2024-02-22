/****************************************************************************
 * apps/testing/testsuites/kernel/time/cases/clock_test_timer05.c
 * Copyright (c) 2013-2019 Huawei Technologies Co., Ltd. All rights reserved.
 * Copyright (c) 2020-2022 Huawei Device Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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

static int test_timer05_g_sig_hdl_cnt01;
static int test_timer05_g_sig_hdl_cnt02;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: temp_sig_handler
 ****************************************************************************/

static void temp_sig_handler(union sigval v)
{
  syslog(LOG_INFO, "This is temp_sig_handler ...\r\n");
  (*(void (*)(void))(v.sival_ptr))();
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: temp_sig_handler01
 ****************************************************************************/

static void temp_sig_handler01(void)
{
  test_timer05_g_sig_hdl_cnt01++;
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: temp_sig_handler02
 ****************************************************************************/

static void temp_sig_handler02(void)
{
  test_timer05_g_sig_hdl_cnt02++;
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
  sev.sigev_notify_function = temp_sig_handler;
  sev.sigev_value.sival_ptr = (void *)temp_sig_handler01;

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

  sev.sigev_value.sival_ptr = (void *)temp_sig_handler02;
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

  assert_int_not_equal(test_timer05_g_sig_hdl_cnt01, 0);
  assert_int_not_equal(test_timer05_g_sig_hdl_cnt02, 0);
}
