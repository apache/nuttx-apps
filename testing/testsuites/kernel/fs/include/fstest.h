/****************************************************************************
 * apps/testing/testsuites/kernel/fs/include/fstest.h
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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>
#include <dirent.h>
#include <sys/stat.h>
#include <syslog.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define TEST_PASS 0
#define TEST_FAIL -1

#define MAX_PATH 300

/* The test files generated during the 'fs-test' are stored in this
 * directory
 */

#define FS_TEST_DIR "fs_test_dir"
#define MOUNT_DIR CONFIG_TESTS_TESTSUITES_MOUNT_DIR

#define PATH_SIZE 128

struct fs_testsuites_state_s
{
  char filename[PATH_SIZE];
  char *ptr;
  int fd1;
  int fd2;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

unsigned long long cm_get_partition_available_size(void);
int cm_unlink_recursive(FAR char *path);
int test_nuttx_fs_test_group_setup(FAR void **state);
int test_nuttx_fs_test_group_teardown(FAR void **state);
int get_mem_info(struct mallinfo *mem_info);

/* cases/fs_creat_test.c
 * ************************************************/

void test_nuttx_fs_creat01(FAR void **state);

/* cases/fs_dup_test.c
 * ************************************************/

void test_nuttx_fs_dup01(FAR void **state);

/* cases/fs_dup2_test.c
 * ************************************************/

void test_nuttx_fs_dup201(FAR void **state);

/* cases/fs_fcntl_test.c
 * ************************************************/

void test_nuttx_fs_fcntl01(FAR void **state);
void test_nuttx_fs_fcntl02(FAR void **state);
void test_nuttx_fs_fcntl03(FAR void **state);

/* cases/fs_fstat_test.c
 * ************************************************/

void test_nuttx_fs_fstat01(FAR void **state);
void test_nuttx_fs_fstat02(FAR void **state);

/* cases/fs_fstatfs_test.c
 * ************************************************/

void test_nuttx_fs_fstatfs01(FAR void **state);

/* cases/fs_fsync_test.c
 * ************************************************/

void test_nuttx_fs_fsync01(FAR void **state);
void test_nuttx_fs_fsync02(FAR void **state);

/* cases/fs_file_get_test.c
 * ************************************************/

void test_nuttx_fs_file_get01(FAR void **state);

/* cases/fs_mkdir_test.c
 * ************************************************/

void test_nuttx_fs_mkdir01(FAR void **state);

/* cases/fs_open_test.c
 * ************************************************/

void test_nuttx_fs_open01(FAR void **state);
void test_nuttx_fs_open02(FAR void **state);

/* cases/fs_opendir_test.c
 * ************************************************/

void test_nuttx_fs_opendir01(FAR void **state);
void test_nuttx_fs_opendir02(FAR void **state);

/* cases/fs_pread_test.c
 * ************************************************/

void test_nuttx_fs_pread01(FAR void **state);

/* cases/fs_pwrite_test.c
 * ************************************************/

void test_nuttx_fs_pwrite01(FAR void **state);

/* cases/fs_read_test.c
 * ************************************************/

void test_nuttx_fs_read01(FAR void **state);

/* cases/fs_readdir_test.c
 * ************************************************/

void test_nuttx_fs_readdir01(FAR void **state);

/* cases/fs_readlink_test.c
 * ************************************************/

void test_nuttx_fs_readlink01(FAR void **state);

/* cases/fs_rename_test.c
 * ************************************************/

void test_nuttx_fs_rename01(FAR void **state);
void test_nuttx_fs_rename02(FAR void **state);

/* cases/fs_rewinddir_test.c
 * ************************************************/

void test_nuttx_fs_rewinddir01(FAR void **state);

/* cases/fs_rmdir_test.c
 * ************************************************/

void test_nuttx_fs_rmdir01(FAR void **state);
void test_nuttx_fs_rmdir02(FAR void **state);
void test_nuttx_fs_rmdir03(FAR void **state);

/* cases/fs_seek_test.c
 * ************************************************/

void test_nuttx_fs_seek01(FAR void **state);
void test_nuttx_fs_seek02(FAR void **state);

/* cases/fs_stat_test.c
 * ************************************************/

void test_nuttx_fs_stat01(FAR void **state);

/* cases/fs_statfs_test.c
 * ************************************************/

void test_nuttx_fs_statfs01(FAR void **state);

/* cases/fs_symlink_test.c
 * ************************************************/

void test_nuttx_fs_symlink01(FAR void **state);

/* cases/fs_truncate_test.c
 * ************************************************/

void test_nuttx_fs_truncate01(FAR void **state);

/* cases/fs_unlink_test.c
 * ************************************************/

void test_nuttx_fs_unlink01(FAR void **state);

/* cases/fs_write_test.c
 * ************************************************/

void test_nuttx_fs_write01(FAR void **state);
void test_nuttx_fs_write02(FAR void **state);
void test_nuttx_fs_write03(FAR void **state);

/* cases/fs_append_test.c
 * ************************************************/

void test_nuttx_fs_append01(FAR void **state);

/* cases/fs_sendfile_test.c
 * ************************************************/

void test_nuttx_fs_sendfile01(FAR void **state);
void test_nuttx_fs_sendfile02(FAR void **state);

/* cases/fs_stream_test.c
 * ************************************************/

void test_nuttx_fs_stream01(FAR void **state);
void test_nuttx_fs_stream02(FAR void **state);
void test_nuttx_fs_stream03(FAR void **state);
void test_nuttx_fs_stream04(FAR void **state);

/* cases/fs_eventfd_test.c
 * ************************************************/

void test_nuttx_fs_eventfd(FAR void **state);

/* fs_poll_test.c
 * ************************************************/

void test_nuttx_fs_poll01(FAR void **state);

#endif
