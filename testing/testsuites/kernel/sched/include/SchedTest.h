/*
 * Copyright (C) 2020 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
/* common/common_test.c *****************************************************/
int TestNuttxSchedTestGroupSetup(void **state);
int TestNuttxSchedTestGroupTearDown(void **state);

/* testcase function */
/* cases/api_pthread_test_001.c *********************************************/
void TestNuttxSchedPthread01(FAR void **state);

/* cases/api_pthread_test_002.c *********************************************/
void TestNuttxSchedPthread02(FAR void **state);

/* cases/api_pthread_test_003.c *********************************************/
void TestNuttxSchedPthread03(FAR void **state);

/* cases/api_pthread_test_004.c *********************************************/
void TestNuttxSchedPthread04(FAR void **state);

/* cases/api_pthread_test_005.c *********************************************/
void TestNuttxSchedPthread05(FAR void **state);

/* cases/api_pthread_test_006.c *********************************************/
void TestNuttxSchedPthread06(FAR void **state);

/* cases/api_pthread_test_007.c *********************************************/
void TestNuttxSchedPthread07(FAR void **state);

/* cases/api_pthread_test_008.c *********************************************/
void TestNuttxSchedPthread08(FAR void **state);

/* cases/api_pthread_test_009.c *********************************************/
void TestNuttxSchedPthread09(FAR void **state);

/* cases/api_task_test_001.c ************************************************/
void TestNuttxSchedTask01(FAR void **state);

/* cases/api_task_test_002.c ************************************************/
void TestNuttxSchedTask02(FAR void **state);

/* cases/api_task_test_003.c ************************************************/
void TestNuttxSchedTask03(FAR void **state);

/* cases/api_task_test_004.c ************************************************/
void TestNuttxSchedTask04(FAR void **state);

/* cases/api_task_test_005.c ************************************************/
void TestNuttxSchedTask05(FAR void **state);

/* cases/api_task_test_006.c ************************************************/
void TestNuttxSchedTask06(FAR void **state);

/* cases/api_task_test_007.c ************************************************/
void TestNuttxSchedTask07(FAR void **state);

#endif