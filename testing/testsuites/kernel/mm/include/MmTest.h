/****************************************************************************
 * apps/testing/testsuites/kernel/mm/include/MmTest.h
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

#ifndef __SYSCALLTEST_H
#define __SYSCALLTEST_H

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

int mmtest_get_rand_size(int min, int max);
void mmtest_showmallinfo(void);
unsigned long mmtest_get_memsize(void);

int test_nuttx_mmsetup(void **state);
int test_nuttx_mmteardown(void **state);

/* test case function */

/* cases/mm_test_001.c ******************************************************/

void test_nuttx_mm01(FAR void **state);

/* cases/mm_test_002.c ******************************************************/

void test_nuttx_mm02(FAR void **state);

/* cases/mm_test_003.c ******************************************************/

void test_nuttx_mm03(FAR void **state);

/* cases/mm_test_004.c ******************************************************/

void test_nuttx_mm04(FAR void **state);

/* cases/mm_test_005.c ******************************************************/

void test_nuttx_mm05(FAR void **state);

/* cases/mm_test_006.c ******************************************************/

void test_nuttx_mm06(FAR void **state);

/* cases/mm_test_007.c ******************************************************/

void test_nuttx_mm07(FAR void **state);

/* cases/mm_test_008.c ******************************************************/

void test_nuttx_mm08(FAR void **state);

#endif