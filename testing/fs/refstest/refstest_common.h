/****************************************************************************
 * apps/testing/fs/refstest/refstest_common.h
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

#ifndef __APPS_TESTING_FS_REFSTEST_REFSTEST_COMMON_H
#define __APPS_TESTING_FS_REFSTEST_REFSTEST_COMMON_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int refstest_setup(FAR void **state);
int refstest_teardown(FAR void **state);

void test_nuttx_fs_refs01(FAR void **state);
void test_nuttx_fs_refs02(FAR void **state);
void test_nuttx_fs_refs03(FAR void **state);
void test_nuttx_fs_refs04(FAR void **state);
void test_nuttx_fs_refs05(FAR void **state);
void test_nuttx_fs_refs06(FAR void **state);
#ifdef CONFIG_FS_RAMMAP
void test_nuttx_fs_refs07(FAR void **state);
#endif
void test_nuttx_fs_refs08(FAR void **state);
void test_nuttx_fs_refs09(FAR void **state);

#endif /* __APPS_TESTING_FS_REFSTEST_REFSTEST_COMMON_H */
