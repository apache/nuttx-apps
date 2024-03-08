/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_fsync_test.c
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <time.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "fstest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TESTFILE "FsyncTestFile"
#define BUF "testData123#$%*-=/ sdafasd37575sasdfasdf3563456345" \
            "63456ADSFASDFASDFQWREdf4as5df4as5dfsd ###"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_fsync01
 ****************************************************************************/

void test_nuttx_fs_fsync01(FAR void **state)
{
  int fd;
  int rval;
  int ret;
  struct fs_testsuites_state_s *test_state;

  test_state = (struct fs_testsuites_state_s *)*state;

  /* open file */

  fd = open(TESTFILE, O_RDWR | O_CREAT, 0700);
  assert_true(fd > 0);
  test_state->fd1 = fd;

  for (int i = 0; i < 20; i++)
    {
      /* do write */

      rval = write(fd, BUF, sizeof(BUF));
      assert_int_equal(rval, sizeof(BUF));

      /* refresh to storage */

      ret = fsync(fd);
      assert_int_equal(ret, 0);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_fsync02
 ****************************************************************************/

void test_nuttx_fs_fsync02(FAR void **state)
{
  int fd;
  int ret;
  char *buf = NULL;
  int bufsize = 4096;
  ssize_t writen = 0;
  struct statfs statfsbuf;
  struct fs_testsuites_state_s *test_state;

  test_state = (struct fs_testsuites_state_s *)*state;

  /* delete test file */

  unlink(TESTFILE);

  /* open file */

  fd = open(TESTFILE, O_CREAT | O_RDWR, 0777);
  assert_true(fd > 0);
  test_state->fd1 = fd;

  /* call fstatfs() */

  ret = fstatfs(fd, &statfsbuf);
  assert_int_equal(ret, 0);

  bufsize = statfsbuf.f_bsize;

  syslog(LOG_INFO, "the bsize = %d\n", statfsbuf.f_bsize);

  /* malloc memory */

  buf = malloc(bufsize * sizeof(char));
  assert_non_null(buf);
  test_state->ptr = buf;

  /* set memory */

  memset(buf, 0x66, bufsize);

  /* do write */

  writen = write(fd, buf, bufsize);
  assert_int_in_range(writen, 1, bufsize);

  /* refresh to storage */

  fsync(fd);

  /* call fstatfs() */

  ret = fstatfs(fd, &statfsbuf);
  assert_int_equal(ret, 0);
}
