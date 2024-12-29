/****************************************************************************
 * apps/testing/testsuites/kernel/mutex/include/MutexTest.h
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
#include "semaphore.h"
#include "unistd.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define MUTEX_TEST_NUM 100
#define ENOERR 0
typedef unsigned int UINT32;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* test case function */

/* cases/posix_mutex_test_001.c
 * ************************************************/

void test_nuttx_mutex_test01(FAR void **state);

/* cases/posix_mutex_test_019.c
 * ************************************************/

void test_nuttx_mutex_test19(FAR void **state);

/* cases/posix_mutex_test_020.c
 * ************************************************/

void test_nuttx_mutex_test20(FAR void **state);
#endif
