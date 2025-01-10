/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/dup_test.c
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
#include <syslog.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
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
 * Name: test_nuttx_syscall_dup01
 ****************************************************************************/

void test_nuttx_syscall_dup01(FAR void **state)
{
  char filename[64];
  int fd = -1;
  int ret;

  sprintf(filename, "%s_file", __func__);

  fd = open(filename, O_RDWR | O_CREAT, 0700);
  if (fd < 0)
    {
      fail_msg("open test file fail !\n");
    }

  ret = dup(fd);

  assert_true(ret > 0);
  if (fd > 0)
    {
      close(fd);
    }

  if (ret > 0)
    close(ret);

  assert_int_equal(unlink(filename), 0);
}

/****************************************************************************
 * Name: test_nuttx_syscall_dup02
 ****************************************************************************/

void test_nuttx_syscall_dup02(FAR void **state)
{
#ifndef CONFIG_FDSAN
  int fds[] =
  {
    -1, 1500
  };

  int ret;
  int nfds;
  int ind;

  nfds = sizeof(fds) / sizeof(int);

  for (ind = 0; ind < nfds; ind++)
    {
      ret = dup(fds[ind]);
      assert_int_equal(ret, -1);
      if (ret == -1)
        {
          assert_int_equal(errno, EBADF);
        }
    }
#endif
}

/****************************************************************************
 * Name: test_nuttx_syscall_dup03
 ****************************************************************************/

void test_nuttx_syscall_dup03(FAR void **state)
{
  int fd[2];
  int ret;

  fd[0] = -1;
  pipe(fd);

  ret = dup(fd[0]);
  assert_true(ret > 0);
  if (ret > 0)
    close(ret);

  ret = dup(fd[1]);
  assert_true(ret > 0);
  if (ret > 0)
    close(ret);

  if (fd[0] > 0)
    close(fd[0]);
  if (fd[1] > 0)
    close(fd[1]);
}

/****************************************************************************
 * Name: test_nuttx_syscall_dup04
 ****************************************************************************/

void test_nuttx_syscall_dup04(FAR void **state)
{
  char fname[64];
  int fd = -1;
  int ret;

  sprintf(fname, "%s_file", __func__);

  fd = open(fname, O_RDWR | O_CREAT, 0700);
  assert_true(fd > 0);

  ret = dup(fd);
  assert_true(ret > 0);

  if (fd > 0)
    {
      close(fd);
    }

  if (ret > 0)
    close(ret);

  assert_int_equal(unlink(fname), 0);
}

/****************************************************************************
 * Name: test_nuttx_syscall_dup05
 ****************************************************************************/

void test_nuttx_syscall_dup05(FAR void **state)
{
  struct stat retbuf;
  struct stat dupbuf;
  int rdoret;
  int wroret;
  int rdwret;
  int duprdo;
  int dupwro;
  int duprdwr;
  char testfile[64] = "";

  sprintf(testfile, "%s_file", __func__);

  rdoret = creat(testfile, 0444);
  if (rdoret > 0)
    {
      duprdo = dup(rdoret);
      if (duprdo > 0)
        {
          assert_int_equal(fstat(rdoret, &retbuf), 0);
          assert_int_equal(fstat(duprdo, &dupbuf), 0);
          assert_int_equal(retbuf.st_mode, dupbuf.st_mode);

          assert_int_equal(close(duprdo), 0);
          assert_int_equal(close(rdoret), 0);
        }
    }

  else
    {
      fail_msg("create test fail fail !\n");
    }

  assert_int_equal(unlink(testfile), 0);

  wroret = creat(testfile, 0222);
  if (wroret > 0)
    {
      dupwro = dup(wroret);
      if (dupwro)
        {
          assert_int_equal(fstat(wroret, &retbuf), 0);
          assert_int_equal(fstat(dupwro, &dupbuf), 0);
          assert_int_equal(retbuf.st_mode, dupbuf.st_mode);

          assert_int_equal(close(dupwro), 0);
          assert_int_equal(close(wroret), 0);
        }
    }

  else
    {
      fail_msg("create test fail fail !\n");
    }

  assert_int_equal(unlink(testfile), 0);

  rdwret = creat(testfile, 0666);
  if (rdwret > 0)
    {
      duprdwr = dup(rdwret);
      if (duprdwr > 0)
        {
          assert_int_equal(fstat(rdwret, &retbuf), 0);
          assert_int_equal(fstat(duprdwr, &dupbuf), 0);
          assert_int_equal(retbuf.st_mode, dupbuf.st_mode);

          assert_int_equal(close(duprdwr), 0);
          assert_int_equal(close(rdwret), 0);
        }
    }

  else
    {
      fail_msg("create test fail fail !\n");
    }

  assert_int_equal(unlink(testfile), 0);
}
