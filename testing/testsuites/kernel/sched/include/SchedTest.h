/****************************************************************************
 * apps/testing/testsuites/kernel/sched/include/SchedTest.h
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

#ifndef __SCHEDTEST_H
#define __SCHEDTEST_H

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/config.h>
#include <setjmp.h>
#include <cmocka.h>
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define PTHREAD_STACK_SIZE CONFIG_DEFAULT_TASK_STACKSIZE
#define DEFAULT_STACKSIZE CONFIG_DEFAULT_TASK_STACKSIZE
#define TASK_PRIORITY SCHED_PRIORITY_DEFAULT

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* common/common_test.c
 * *****************************************************/

int test_nuttx_sched_test_group_setup(void **state);
int test_nuttx_sched_test_group_teardown(void **state);

/* testcase function */

/* cases/api_pthread_test_001.c
 * *********************************************/

void test_nuttx_sched_pthread01(FAR void **state);

/* cases/api_pthread_test_002.c
 * *********************************************/

void test_nuttx_sched_pthread02(FAR void **state);

/* cases/api_pthread_test_003.c
 * *********************************************/

void test_nuttx_sched_pthread03(FAR void **state);

/* cases/api_pthread_test_004.c
 * *********************************************/

void test_nuttx_sched_pthread04(FAR void **state);

/* cases/api_pthread_test_005.c
 * *********************************************/

void test_nuttx_sched_pthread05(FAR void **state);

/* cases/api_pthread_test_006.c
 * *********************************************/

void test_nuttx_sched_pthread06(FAR void **state);

/* cases/api_pthread_test_007.c
 * *********************************************/

void test_nuttx_sched_pthread07(FAR void **state);

/* cases/api_pthread_test_008.c
 * *********************************************/

void test_nuttx_sched_pthread08(FAR void **state);

/* cases/api_pthread_test_009.c
 * *********************************************/

void test_nuttx_sched_pthread09(FAR void **state);

/* cases/api_task_test_001.c
 * ************************************************/

void test_nuttx_sched_task01(FAR void **state);

/* cases/api_task_test_002.c
 * ************************************************/

void test_nuttx_sched_task02(FAR void **state);

/* cases/api_task_test_003.c
 * ************************************************/

void test_nuttx_sched_task03(FAR void **state);

/* cases/api_task_test_004.c
 * ************************************************/

void test_nuttx_sched_task04(FAR void **state);

/* cases/api_task_test_005.c
 * ************************************************/

void test_nuttx_sched_task05(FAR void **state);

/* cases/api_task_test_006.c
 * ************************************************/

void test_nuttx_sched_task06(FAR void **state);

/* cases/api_task_test_007.c
 * ************************************************/

void test_nuttx_sched_task07(FAR void **state);

#endif
