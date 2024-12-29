/****************************************************************************
 * apps/testing/testsuites/kernel/fs/common/test_fs_common.c
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
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "fstest.h"

#define CM_FSTESTDIR "CM_fs_testdir"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

unsigned long long cm_get_partition_available_size(void)
{
  unsigned long long size = 0;
  int ret;
  struct statfs statinfo;

  /* call statfs() */

  ret = statfs(MOUNT_DIR, &statinfo);
  if (ret == 0)
    {
      size = (unsigned long long)statinfo.f_bsize *
             (unsigned long long)statinfo.f_bfree;
    }
  else
    {
      size = (unsigned long long)-1;
    }
  return size;
}

int get_mem_info(struct mallinfo *mem_info)
{
  *mem_info = mallinfo();
  return 0;
}

int cm_unlink_recursive(FAR char *path)
{
  struct dirent *d;
  struct stat stat;
  size_t len;
  int ret;
  DIR *dp;

  ret = lstat(path, &stat);
  if (ret < 0)
    {
      return ret;
    }

  if (!S_ISDIR(stat.st_mode))
    {
      return unlink(path);
    }

  dp = opendir(path);
  if (dp == NULL)
    {
      return -1;
    }

  len = strlen(path);
  if (len > 0 && path[len - 1] == '/')
    {
      path[--len] = '\0';
    }

  while ((d = readdir(dp)) != NULL)
    {
      if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0)
        {
          continue;
        }

      snprintf(&path[len], PATH_MAX - len, "/%s", d->d_name);
      ret = cm_unlink_recursive(path);
      if (ret < 0)
        {
          closedir(dp);
          return ret;
        }
    }

  ret = closedir(dp);
  if (ret >= 0)
    {
      path[len] = '\0';
      ret = rmdir(path);
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_test_group_setup
 ****************************************************************************/

int test_nuttx_fs_test_group_setup(void **state)
{
  int res;
  struct stat buf;
  struct fs_testsuites_state_s *test_state;

  res = chdir(MOUNT_DIR);
  if (res != 0)
    {
      syslog(LOG_INFO, "ERROR: Failed to switch the mount dir\n");
      exit(1);
    }

  res = stat(CM_FSTESTDIR, &buf);
  if (res == 0 && buf.st_mode == S_IFDIR)
    {
      res = chdir(CM_FSTESTDIR);
    }
  else
    {
      char testdir[PATH_MAX] =
      {
        0
      };

      sprintf(testdir, "%s/%s", MOUNT_DIR, CM_FSTESTDIR);

      /* Delete the existing test directory */

      cm_unlink_recursive(testdir);
      res = mkdir(CM_FSTESTDIR, 0777);
      if (res != 0)
        {
          syslog(LOG_INFO,
                 "ERROR: Failed to creat the test directory\n");
          exit(1);
        }

      chdir(CM_FSTESTDIR);
    }

  test_state = zalloc(sizeof(struct fs_testsuites_state_s));
  assert_false(test_state == NULL);
  test_state->ptr = NULL;
  test_state->fd1 = -1;
  test_state->fd2 = -1;
  memset(test_state->filename, 0, PATH_SIZE);
  *state = test_state;
  return res;
}

/****************************************************************************
 * Name: test_nuttx_fs_testgroupteardown
 ****************************************************************************/

int test_nuttx_fs_test_group_teardown(void **state)
{
  int res;
  char testdir[PATH_MAX] =
  {
    0
  };

  struct fs_testsuites_state_s *test_state;

  test_state = (struct fs_testsuites_state_s *)*state;

  if (test_state->fd1 > 0)
    close(test_state->fd1);
  if (test_state->fd2 > 0)
    close(test_state->fd2);
  if (test_state->ptr != NULL)
    free(test_state->ptr);
  free(test_state);

  sprintf(testdir, "%s/%s", MOUNT_DIR, CM_FSTESTDIR);
  res = chdir(MOUNT_DIR);
  if (res != 0)
    {
      syslog(LOG_INFO, "ERROR: Failed to switch the mount dir\n");
      exit(1);
    }

  /* Delete the existing test directory */

  cm_unlink_recursive(testdir);
  return 0;
}
