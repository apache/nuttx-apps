/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_fstatfs_test.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/config.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <stdio.h>
#include <fcntl.h>
#include <syslog.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "fstest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_FILE "fstat_test_file"
#define BUF_SIZE 512

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_printstatfs
 ****************************************************************************/

__attribute__((unused)) static void
test_nuttx_fs_printstatfs(struct statfs *buf)
{
  syslog(LOG_INFO, "statfs buffer:\n");
  syslog(LOG_INFO, "  f_type:     %lu\n", (unsigned long)buf->f_type);
  syslog(LOG_INFO, "  f_namelen:  %lu\n", (unsigned long)buf->f_namelen);
  syslog(LOG_INFO, "  f_bsize:    %lu\n", (unsigned long)buf->f_bsize);
  syslog(LOG_INFO, "  f_blocks:   %llu\n",
         (unsigned long long)buf->f_blocks);
  syslog(LOG_INFO, "  f_bfree:    %llu\n",
         (unsigned long long)buf->f_bfree);
  syslog(LOG_INFO, "  f_bavail:   %llu\n",
         (unsigned long long)buf->f_bavail);
  syslog(LOG_INFO, "  f_files:    %llu\n",
         (unsigned long long)buf->f_files);
  syslog(LOG_INFO, "  f_ffree:    %llu\n",
         (unsigned long long)buf->f_ffree);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_fstatfs01
 ****************************************************************************/

void test_nuttx_fs_fstatfs01(FAR void **state)
{
  struct statfs statfsbuf;
  int ret;
  int fd;
  char *buf;
  struct fs_testsuites_state_s *test_state;

  test_state = (struct fs_testsuites_state_s *)*state;

  /* malloc memory */

  buf = malloc(BUF_SIZE);
  assert_non_null(buf);
  test_state->ptr = buf;

  /* set memory */

  memset(buf, 'B', BUF_SIZE);

  /* open file */

  fd = open(TEST_FILE, O_RDWR | O_CREAT, 0777);
  assert_true(fd > 0);
  test_state->fd1 = fd;

  /* call fstatfs() */

  ret = fstatfs(fd, &statfsbuf);
  assert_int_equal(ret, 0);
}
