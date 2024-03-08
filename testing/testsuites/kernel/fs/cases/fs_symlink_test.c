/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_symlink_test.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
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

#define TEST_FILE "test_file"
#define PATH_MAX_SIZE 64

/****************************************************************************
 * Private Data
 ****************************************************************************/

static char *path;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_run_test
 ****************************************************************************/

static int test_nuttx_fs_run_test(void)
{
  int ret;

  /* test symlink */

  char buf[64] = {
    0
  };

  ret = symlink(path, "/file_link");
  if (ret != 0)
    {
      return ERROR;
    }

  else
    {
      ret = readlink("/file_link", buf, PATH_MAX_SIZE);
      unlink("/file_link");
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_symlink01
 ****************************************************************************/

void test_nuttx_fs_symlink01(FAR void **state)
{
  int fd;
  int ret;
  char *buf;
  struct fs_testsuites_state_s *test_state;

  test_state = (struct fs_testsuites_state_s *)*state;

  /* malloc memory */

  path = malloc(PATH_MAX_SIZE * sizeof(char));
  assert_non_null(path);
  test_state->ptr = path;

  /* set memory */

  buf = getcwd(NULL, 0);
  memset(path, '\0', PATH_MAX_SIZE);
  sprintf(path, "%s/symlink_test_file", buf);
  free(buf);

  /* open file */

  fd = open(path, O_WRONLY | O_CREAT, 0700);
  assert_true(fd > 0);
  test_state->fd1 = fd;

  /* do run test */

  ret = test_nuttx_fs_run_test();

  /* do remove */

  unlink(path);

  assert_int_equal(ret, OK);
}
