/****************************************************************************
 * apps/testing/testsuites/kernel/time/cases/clock_test_smoke.c
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
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "TimeTest.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_clock_test_smoke01
 ****************************************************************************/

void test_nuttx_clock_test_smoke01(FAR void **state)
{
  clockid_t clk = CLOCK_REALTIME;
  struct timespec res = {
      0,
      0,
  };

  struct timespec  setts = {
      0,
      0,
  };

  struct timespec oldtp = {
      0,
      0
  };

  struct timespec  ts = {
      0,
      0,
  };

  int ret;
  int passflag = 0;

  /* get clock resolution */

  ret = clock_getres(clk, &res);
  assert_int_equal(ret, 0);

  /* get clock realtime */

  ret = clock_gettime(clk, &oldtp);
  syslog(LOG_INFO, "the clock current time: %lld second, %ld nanosecond\n",
         (long long)oldtp.tv_sec, oldtp.tv_nsec);
  assert_int_equal(ret, 0);

  /* set clock realtime */

  setts.tv_sec = oldtp.tv_sec + 1;
  setts.tv_nsec = oldtp.tv_nsec;
  syslog(LOG_INFO, "the clock setting time: %lld second, %ld nanosecond\n",
         (long long)setts.tv_sec, setts.tv_nsec);
  ret = clock_settime(CLOCK_REALTIME, &setts);
  assert_int_equal(ret, 0);

  ret = clock_gettime(clk, &ts);
  syslog(LOG_INFO,
         "obtaining the current time after "
         "setting: %lld second, %ld nanosecond\n",
         (long long)ts.tv_sec,
         ts.tv_nsec);

  /* 1, means obtaining time's errno is 1 second. */

  passflag = (ts.tv_sec >= setts.tv_sec) && (ts.tv_sec <= setts.tv_sec + 1);
  assert_int_equal(ret, 0);
  assert_int_equal(passflag, 1);
}
