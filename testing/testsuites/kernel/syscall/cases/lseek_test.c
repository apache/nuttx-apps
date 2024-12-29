/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/lseek_test.c
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
#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "SyscallTest.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_lseek01
 ****************************************************************************/

void test_nuttx_syscall_lseek01(FAR void **state)
{
  int fd;
  char filename[32] =
  {
    0
  };

  ssize_t rval;
  char read_buf[64] =
  {
    0
  };

  off_t test_ret;
  struct tcase
  {
    off_t off;
    int whence;
    char wname[64];
    off_t exp_off;
    ssize_t exp_size;
    char *exp_data;
  }

  tcases[] =
    {
        {
          4, SEEK_SET, "SEEK_SET", 4, 3, "efg"
        },

        {
          -2, SEEK_CUR, "SEEK_CUR", 5, 2, "fg"
        },

        {
          -4, SEEK_END, "SEEK_END", 3, 4, "defg"
        },

        {
          0, SEEK_END, "SEEK_END", 7, 0, NULL
        },
    };

  snprintf(filename, sizeof(filename), "%s_file", __func__);

  fd = open(filename, O_RDWR | O_CREAT);

  if (fd > 0)
    {
      rval = write(fd, "abcdefg", 7);
      assert_int_in_range(rval, 0, 7);
    }

  else
    {
      fail_msg("creat test file fail !\n");
    }

  for (int i = 0; i < 4; i++)
    {
      /* reset the offset to end of file */

      rval = read(fd, read_buf, sizeof(read_buf));
      assert_int_in_range(rval, 0, 64);
      memset(read_buf, 0, sizeof(read_buf));
      test_ret = lseek(fd, tcases[i].off, tcases[i].whence);
      if (test_ret == (off_t)-1)
        {
          syslog(LOG_ERR, "lseek() failed");
          fail_msg("test fail !\n");
        }

      if (test_ret != tcases[i].exp_off)
        {
          syslog(LOG_ERR, "lseek() returned not expected\n");
          fail_msg("test fail !\n");
        }

      rval = read(fd, read_buf, tcases[i].exp_size);
      assert_int_in_range(rval, 0, 64);

      if (tcases[i].exp_data && strcmp(read_buf, tcases[i].exp_data))
        {
          syslog(LOG_ERR, "lseek() read incorrect data\n");
          fail_msg("test fail !\n");
        }
    }

  if (fd > 0)
    {
      close(fd);
    }

  unlink(filename);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_lseek07
 ****************************************************************************/

void test_nuttx_syscall_lseek07(FAR void **state)
{
  int fd1;
  int fd2;
  int flag = 1;
  int ret;
  char read_buf[64] =
  {
    0
  };

  struct tcase
  {
    int *fd;
    char fname[32];
    off_t off;
    off_t exp_off;
    int exp_size;
    char exp_data[16];
  }

  tcases[] =
    {
        {
          NULL, "", 7, 7, 10, "abcdefgijk"
        },

        {
          NULL, "", 2, 2, 7, "abijkfg"
        },
    };

  tcases[0].fd = &fd1;
  tcases[1].fd = &fd2;
  snprintf(tcases[0].fname, sizeof(tcases[0].fname), "%s_file",
           __func__);
  snprintf(tcases[1].fname, sizeof(tcases[1].fname), "%s_file",
           __func__);

  fd1 = open(tcases[0].fname, O_RDWR | O_CREAT, 0644);
  fd2 = open(tcases[1].fname, O_RDWR | O_CREAT, 0644);

  write(fd1, "abcdefg", sizeof("abcdefg") - 1);
  write(fd2, "abcdefg", sizeof("abcdefg") - 1);

  for (int i = 0; i < 2; i++)
    {
      memset(read_buf, 0, sizeof(read_buf));

      ret = lseek(*(tcases[i].fd), tcases[i].off, SEEK_SET);
      if (ret == (off_t)-1)
        flag = 0;

      if (ret != tcases[i].exp_off)
        flag = 0;

      write(*(tcases[i].fd), "ijk", sizeof("ijk") - 1);

      close(*(tcases[i].fd));

      *(tcases[i].fd) = open(tcases[i].fname, O_RDWR);

      read(*(tcases[i].fd), read_buf, tcases[i].exp_size);

      if (strcmp(read_buf, tcases[i].exp_data))
        {
          flag = 0;
          syslog(LOG_ERR, "FAIL, lseek() wrote incorrect data\n");
        }
    }

  if (fd1 > 0)
    close(fd1);

  if (fd2 > 0)
    close(fd2);
  unlink(tcases[0].fname);
  unlink(tcases[1].fname);
  assert_true(flag);
}
