/****************************************************************************
 * apps/testing/testsuites/kernel/time/include/TimeTest.h
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
