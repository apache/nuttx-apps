/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_statfs_test.c
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <dirent.h>
#include <syslog.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "fstest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define _1k 1024

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
  syslog(LOG_INFO, "  f_type:     %u \n", (unsigned)buf->f_type);
  syslog(LOG_INFO, "  f_bsize:    %zuk\n", (size_t)buf->f_bsize / _1k);
  syslog(LOG_INFO, "  f_blocks:   %llu \n",
         (unsigned long long)buf->f_blocks);
  syslog(LOG_INFO, "  f_bfree:    %llu \n",
         (unsigned long long)buf->f_bfree);
  syslog(LOG_INFO, "  f_bavail:   %llu \n",
         (unsigned long long)buf->f_bavail);
  syslog(LOG_INFO, "  f_files:    %llu \n",
         (unsigned long long)buf->f_files);
  syslog(LOG_INFO, "  f_ffree:    %llu \n",
         (unsigned long long)buf->f_ffree);
  syslog(LOG_INFO, "  f_namelen:  %zu \n", (size_t)buf->f_namelen);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_statfs01
 ****************************************************************************/

void test_nuttx_fs_statfs01(FAR void **state)
{
  struct statfs diskinfo;

  /* call statfs() */

  char *buf = getcwd(NULL, 0);
  int ret = statfs(buf, &diskinfo);
  free(buf);
  assert_int_equal(ret, 0);
}
