/****************************************************************************
 * apps/testing/testsuites/kernel/socket/include/SocketTest.h
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

#ifndef __TEST_H
#define __TEST_H

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/config.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define CLIENT_NUM 8

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* test case function */

/* cases/net_socket_test_005.c
 * ***********************************************/

void test_nuttx_net_socket_test05(FAR void **state);

/* cases/net_socket_test_006.c
 * ************************************************/

void test_nuttx_net_socket_test06(FAR void **state);

/* cases/net_socket_test_008.c
 * ************************************************/

void test_nuttx_net_socket_test08(FAR void **state);

/* cases/net_socket_test_009.c
 * ************************************************/

void test_nuttx_net_socket_test09(FAR void **state);

/* cases/net_socket_test_010.c
 * ************************************************/

void test_nuttx_net_socket_test10(FAR void **state);

/* cases/net_socket_test_011.c
 * ************************************************/

void test_nuttx_net_socket_test11(FAR void **state);

#endif
