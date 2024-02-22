/****************************************************************************
 * apps/testing/testsuites/kernel/time/include/TimeTest.h
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

#ifndef TIME_CLOCK_LT_CLOCK_TEST_H_
#define TIME_CLOCK_LT_CLOCK_TEST_H_

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/times.h>
#include <sys/time.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CLOCK_RES_SEC 0
#define CLOCK_RES_NSEC 1000
#define CLOCK_COARSE_RES_SEC 0
#define CLOCK_COARSE_RES_NSEC 1000000
#define CLOCK_GET_CPU_CLOCKID(pid) ((-(pid) - 1) * 8U + 2)

/* cases/clock_test_smoke.c *************************************************/

void test_nuttx_clock_test_smoke01(FAR void **state);

/* cases/clock_test_timer01.c ***********************************************/

void test_nuttx_clock_test_timer01(FAR void **state);

/* cases/clock_test_timer03.c ***********************************************/

void test_nuttx_clock_test_timer03(FAR void **state);

/* cases/clock_test_timer04.c ***********************************************/

void test_nuttx_clock_test_timer04(FAR void **state);

/* cases/clock_test_timer05.c ***********************************************/

void test_nuttx_clock_test_timer05(FAR void **state);

/* cases/clock_test_clock01.c ***********************************************/

void test_nuttx_clock_test_clock01(FAR void **state);

/* cases/clock_test_clock02.c ***********************************************/

void test_nuttx_clock_test_clock02(FAR void **state);

#endif /* TIME_CLOCK_LT_CLOCK_TEST_H_ */
