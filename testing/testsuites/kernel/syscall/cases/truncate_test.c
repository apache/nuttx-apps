/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/truncate_test.c
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
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
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
 * Name: test_nuttx_syscall_truncate01
 ****************************************************************************/

void test_nuttx_syscall_truncate01(FAR void **state)
{
  int BUF_SIZE = 256;   /* buffer size */
  int FILE_SIZE = 1024; /* test file size */
  int TRUNC_LEN = 256;  /* truncation length */
  struct stat stat_buf; /* stat(2) struct contents */
  int lc;
  int ret;
  off_t file_length; /* test file length */
  char truncate01_fileneme[128] = "";

  /* setup */

  int fd;
  int i; /* file handler for testfile */
  int c;
  int c_total = 0;         /* no. bytes to be written to file */
  char tst_buff[BUF_SIZE]; /* buffer to hold data */

  snprintf(truncate01_fileneme, sizeof(truncate01_fileneme), "%s_file",
           __func__);

  /* Fill the test buffer with the known data */

  for (i = 0; i < BUF_SIZE; i++)
    {
      tst_buff[i] = 'a';
    }

  /* Creat a testfile under temporary directory */

  fd = open(truncate01_fileneme, O_RDWR | O_CREAT,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd < 0)
    {
      syslog(LOG_ERR, "open test file fail !\n");
      fail_msg("test fail !");
    }

  /* Write to the file 1k data from the buffer */

  while (c_total < FILE_SIZE)
    {
      c = write(fd, tst_buff, sizeof(tst_buff));
      if (c > 0)
        {
          c_total += c;
        }
    }

  /* Close the testfile after writing data into it */

  close(fd);

  /* do test */

  for (lc = 0; lc < 2; lc++)
    {
      ret = truncate(truncate01_fileneme, TRUNC_LEN);

      if (ret == -1)
        {
          syslog(LOG_ERR,
                 "FAIL, truncate(%s, %d) Failed, errno=%d : %s\n",
                 truncate01_fileneme, TRUNC_LEN, errno, strerror(errno));
          fail_msg("test fail !");
        }
      else
        {
          /* Get the testfile information using
           * stat(2).
           */

          if (stat(truncate01_fileneme, &stat_buf) < 0)
            {
              syslog(LOG_ERR,
                     "FAIL, stat(2) of "
                     "%s failed, error:%d\n",
                     truncate01_fileneme, errno);
              fail_msg("test fail !");
            }

          stat_buf.st_mode &= ~S_IFREG;
          file_length = stat_buf.st_size;

          /* Check for expected size of testfile after
           * truncate(2) on it.
           */

          if (file_length != TRUNC_LEN)
            {
              syslog(LOG_ERR,
                     "FAIL, %s: Incorrect file "
                     "size %" PRId64 " , Expected %d\n",
                     truncate01_fileneme, (int64_t)file_length,
                     TRUNC_LEN);
              fail_msg("test fail !");
            }
        }
    }

  assert_int_equal(unlink(truncate01_fileneme), 0);
}
