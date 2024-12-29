/****************************************************************************
 * apps/testing/testsuites/kernel/pthread/include/PthreadTest.h
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

#ifndef PTHREAD_TEST_H
#define PTHREAD_TEST_H

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/config.h>
#include <errno.h>
#include <syslog.h>
#include <stdio.h>
#include <pthread.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "sched.h"
#include "signal.h"
#include "semaphore.h"
#include "sched.h"
#include "pthread.h"
#include "limits.h"
#include "unistd.h"
#include "mqueue.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

typedef unsigned int UINT32;
typedef unsigned long UINTPTR;

#define PTHREAD_NO_ERROR 0
#define PTHREAD_IS_ERROR (-1)
#define PTHREAD_SIGNAL_SUPPORT 0 /* 0 means that not support the signal */
#define PTHREAD_PRIORITY_TEST 20
#define PTHREAD_DEFAULT_STACK_SIZE (LOSCFG_BASE_CORE_TSK_DEFAULT_STACK_SIZE)
#define PTHREAD_KEY_NUM 10
#define THREAD_NUM 3
#define PTHREAD_TIMEOUT (THREAD_NUM * 2)
#define PTHREAD_INTHREAD_TEST 0 /* Control going to or is already for Thread */

/* Control going to or is already for Main */

#define PTHREAD_INMAIN_TEST 1
#define INVALID_PSHARED_VALUE (-100)
#define NUM_OF_CONDATTR 10
#define RUNTIME 5
#define PTHREAD_THREADS_NUM 3
#define TCOUNT 5      // Number of single-threaded polling
#define COUNT_LIMIT 7 // The number of times the signal is sent
#define HIGH_PRIORITY 5
#define LOW_PRIORITY 10
#define PTHREAD_EXIT_VALUE ((void *)100) /* The return code of the thread when using pthread_exit(). */

#define PTHREAD_EXISTED_NUM TASK_EXISTED_NUM
#define PTHREAD_EXISTED_SEM_NUM SEM_EXISTED_NUM

/* We are testing conformance to IEEE Std 1003.1, 2003 Edition */

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif

#define PRIORITY_OTHER (-1)
#define PRIORITY_FIFO 20
#define PRIORITY_RR 20
#define LOSCFG_BASE_CORE_TSK_CONFIG 1024

extern UINT32 g_testpthreadcount;
extern UINT32 g_testpthreadtaskmaxnum;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

pthread_t testpthreadself(void);

/* test case function */

/* cases/posix_pthread_test_003.c
 * ************************************************/

void test_nuttx_pthread_test03(FAR void **state);

/* cases/posix_pthread_test_004.c
 * ************************************************/

void test_nuttx_pthread_test04(FAR void **state);

/* cases/posix_pthread_test_005.c
 * ************************************************/

void test_nuttx_pthread_test05(FAR void **state);

/* cases/posix_pthread_test_006.c
 * ************************************************/

void test_nuttx_pthread_test06(FAR void **state);

/* cases/posix_pthread_test_009.c
 * ************************************************/

void test_nuttx_pthread_test09(FAR void **state);

/* cases/posix_pthread_test_018.c
 * ************************************************/

void test_nuttx_pthread_test18(FAR void **state);

/* cases/posix_pthread_test_019.c
 * ************************************************/

void test_nuttx_pthread_test19(FAR void **state);

/* cases/posix_pthread_test_021.c
 * ************************************************/

void test_nuttx_pthread_test21(FAR void **state);
#endif
