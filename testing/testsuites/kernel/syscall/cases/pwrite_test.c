/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/pwrite_test.c
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
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "SyscallTest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define K1 32
#define K2 (K1 * 2)
#define K3 (K1 * 3)
#define K4 (K1 * 4)
#define K5 (K1 * 5)

/****************************************************************************
 * Private data Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

int pwrite02_init_buffers(char *[]);
int pwrite02_l_seek(int, off_t, int, off_t);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pwrite02_init_buffers
 ****************************************************************************/

int pwrite02_init_buffers(char *wbuf[])
{
  int i;

  for (i = 0; i < 4; i++)
    {
      wbuf[i] = (char *)malloc(K1);
      if (wbuf[i] == NULL)
        {
          syslog(LOG_ERR, "ib: malloc failed: errno=%d\n", errno);
          return -1;
        }

      memset(wbuf[i], i, K1);
    }

  return 0;
}

void pwrite02_free_buffers(char *wbuf[])
{
  int i;

  for (i = 0; i < 4; i++)
    {
      if (wbuf[i] != NULL)
        {
          free(wbuf[i]);
        }
    }
}

/****************************************************************************
 * Name: pwrite02_l_seek
 ****************************************************************************/

/* l_seek() is a local front end to lseek().
 * "checkoff" is the offset at which we believe we should be at.
 * Used to validate pwrite doesn't move the offset.
 */

int pwrite02_l_seek(int fdesc, off_t offset, int whence, off_t checkoff)
{
  off_t offloc;

  if ((offloc = lseek(fdesc, offset, whence)) != checkoff)
    {
      syslog(LOG_ERR,
             "FAIL, (%ld = lseek(%d, %ld, %d)) != %ld) errno = %d\n",
             (long int)offloc, fdesc, (long int)offset, whence,
             (long int)checkoff, errno);
      return -1;
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_pwrite01
 ****************************************************************************/

void test_nuttx_syscall_pwrite01(FAR void **state)
{
  int fd;
  int ret;
  char filename[64] = "";

  sprintf(filename, "%s_file", __func__);

  fd = open(filename, O_RDWR | O_CREAT, 0700);
  assert_true(fd > 0);
  ret = pwrite(fd, NULL, 0, 0);
  assert_int_equal(ret, 0);

  if (fd > 0)
    assert_int_equal(close(fd), 0);

  assert_int_equal(unlink(filename), 0);
}

/****************************************************************************
 * Name: test_nuttx_syscall_pwrite02
 ****************************************************************************/

void test_nuttx_syscall_pwrite02(FAR void **state)
{
  char fname[256] = "";
  char *wbuf[4];
  int fd;
  int nbytes;
  int lc;
  struct stat statbuf;

  sprintf(fname, "%s_file", __func__);

  for (lc = 0; lc < 2; lc++)
    {
      assert_int_equal(pwrite02_init_buffers(wbuf), 0);

      fd = open(fname, O_RDWR | O_CREAT, 0666);
      assert_true(fd > 0);

      /* pwrite() K1 of data (0's) at offset 0.
       */

      nbytes = pwrite(fd, wbuf[0], K1, 0);
      assert_int_equal(nbytes, K1);
      if (nbytes != K1)
        {
          syslog(LOG_ERR,
                 "FAIL, pwrite at 0 failed: nbytes=%d, errno=%d\n",
                 nbytes, errno);
          assert_int_equal(unlink(fname), 0);
        }

      /* We should still be at offset 0.
       */

      assert_int_equal(pwrite02_l_seek(fd, 0, SEEK_CUR, 0), 0);

      /* lseek() to a non K boundary, just to be different.
       */

      assert_int_equal(pwrite02_l_seek(fd, K1 / 2, SEEK_SET, K1 / 2), 0);

      /* pwrite() K1 of data (2's) at offset K2.
       */

      nbytes = pwrite(fd, wbuf[2], K1, K2);
      assert_int_equal(nbytes, K1);
      if (nbytes != K1)
        {
          syslog(LOG_ERR,
                 "FAIL, pwrite at K2 failed: nbytes=%d, errno=%d\n",
                 nbytes, errno);
          assert_int_equal(unlink(fname), 0);
        }

      /* We should still be at our non K boundary.
       */

      assert_int_equal(pwrite02_l_seek(fd, 0, SEEK_CUR, K1 / 2), 0);

      /* lseek() to an offset of K3.
       */

      assert_int_equal(pwrite02_l_seek(fd, K3, SEEK_SET, K3), 0);

      /* This time use a normal write() of K1 of data (3's) which should
       * take place at an offset of K3, moving the file pointer to K4.
       */

      nbytes = write(fd, wbuf[3], K1);
      assert_int_equal(nbytes, K1);
      if (nbytes != K1)
        {
          syslog(LOG_ERR, "FAIL, write failed: nbytes=%d, errno=%d\n",
                 nbytes, errno);
          assert_int_equal(unlink(fname), 0);
        }

      /* We should be at offset K4.
       */

      assert_int_equal(pwrite02_l_seek(fd, 0, SEEK_CUR, K4), 0);

      /* pwrite() K1 of data (1's) at offset K1.
       */

      nbytes = pwrite(fd, wbuf[1], K1, K1);
      assert_int_equal(nbytes, K1);
      if (nbytes != K1)
        {
          syslog(LOG_ERR, "FAIL, pwrite failed: nbytes=%d, errno=%d\n",
                 nbytes, errno);
          assert_int_equal(unlink(fname), 0);
        }

      /* -------------------------------------------------------------- */

      /* Now test that O_APPEND takes precedence over any
       * offset specified by pwrite(), but that the file
       * pointer remains unchanged.  First, close then reopen
       * the file and ensure it is already K4 in length and
       * set the file pointer to it's midpoint, K2.
       */

      close(fd);
      fd = open(fname, O_RDWR | O_APPEND, 0666);
      assert_true(fd > 0);
      if (fd < 0)
        {
          fail_msg("TEST FAIL !\n");
          syslog(LOG_ERR, "FAIL, open failed: fname = %s, errno = %d\n",
                 fname, errno);
          assert_int_equal(unlink(fname), 0);
        }

      if (fstat(fd, &statbuf) == -1)
        {
          fail_msg("TEST FAIL !\n");
          syslog(LOG_ERR, "FAIl, fstat failed: errno = %d\n", errno);
          assert_int_equal(unlink(fname), 0);
        }

      if (statbuf.st_size != K4)
        {
          fail_msg("TEST FAIL !\n");
          syslog(LOG_ERR, "FAIl, file size is %ld != K4\n",
                 (long int)statbuf.st_size);
          assert_int_equal(unlink(fname), 0);
        }

      assert_int_equal(pwrite02_l_seek(fd, K2, SEEK_SET, K2), 0);

      /* Finally, pwrite() some K1 of data at offset 0.
       * What we should end up with is:
       *      -The file pointer should still be at K2.
       *      -The data should have been written to the end
       *       of the file (O_APPEND) and should be K5 in size.
       */

      if ((nbytes = pwrite(fd, wbuf[0], K1, 0)) != K1)
        {
          fail_msg("TEST FAIL !\n");
          syslog(LOG_ERR,
                 "FAIl, pwrite at 0 failed: nbytes=%d, errno=%d\n",
                 nbytes, errno);
        }

      assert_int_equal(pwrite02_l_seek(fd, 0, SEEK_CUR, K2), 0);
      if (fstat(fd, &statbuf) == -1)
        {
          fail_msg("TEST FAIL !\n");
          syslog(LOG_ERR, "FAIl, fstat failed: errno = %d\n", errno);
        }

      if (statbuf.st_size != K5)
        {
          fail_msg("TEST FAIL !\n");
          syslog(LOG_ERR, "FAIl, file size is %ld != K4\n",
                 (long int)statbuf.st_size);
        }

      close(fd);
      assert_int_equal(unlink(fname), 0);
      pwrite02_free_buffers(wbuf);
    }
}
