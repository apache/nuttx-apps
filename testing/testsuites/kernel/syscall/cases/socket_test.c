/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/socket_test.c
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
#include <sys/stat.h>
#include <errno.h>
#include <setjmp.h>
#include <cmocka.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include "SyscallTest.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_verifysocket01
 ****************************************************************************/

void test_nuttx_syscall_verifysocket01(int testno, int domain, int type,
                                    int proto, int retval, int experrno)
{
  int fd;
  int ret;
  errno = 0;
  fd = socket(domain, type, proto);
  ret = fd;
  if (fd > 0)
    {
      ret = 0;
      close(fd);
    }

  else
    {
      ret = -1;
    }

  if (errno != experrno || ret != retval)
    {
      syslog(LOG_WARNING,
             "NO.%d do socket() test, ret=%d  erron=%d experrno=%d\n",
             testno, fd, errno, experrno);
    }

  assert_int_equal(ret, retval);
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_verifysocket02
 ****************************************************************************/

void test_nuttx_syscall_verifysocket02(int type, int flag, int fl_flag)
{
  int fd;
  int res;
  int tmp;

  fd = socket(PF_INET, type, 0);
  assert_int_not_equal(fd, -1);

  res = fcntl(fd, fl_flag);
  tmp = (res & flag);

  if (flag != 0)
    {
      assert_int_not_equal(tmp, 0);
    }

  else
    {
      assert_int_equal(tmp, 0);
    }

  assert_int_equal(close(fd), 0);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_sockettest01
 ****************************************************************************/

void test_nuttx_syscall_sockettest01(FAR void **state)
{
  struct test_case_t
  {
    int domain;
    int type;
    int proto;
    int retval;
    int experrno;
  }

  tdat[] =
    {
        {
          0, SOCK_STREAM, 0, -1, EAFNOSUPPORT
        },

        {
          PF_INET, 75, 0, -1, EAFNOSUPPORT
        },

        {
          PF_INET, SOCK_RAW, 0, -1, EAFNOSUPPORT
        },

        {
          PF_INET, SOCK_STREAM, 17, -1, EAFNOSUPPORT
        },

        {
          PF_INET, SOCK_DGRAM, 6, -1, EAFNOSUPPORT
        },

        {
          PF_INET, SOCK_STREAM, 6, 0, 0
        },

        {
          PF_INET, SOCK_STREAM, 1, -1, EAFNOSUPPORT
        }
    };

  for (int i = 0; i < 7; i++)
    {
      test_nuttx_syscall_verifysocket01(i, tdat[i].domain, tdat[i].type,
                                     tdat[i].proto, tdat[i].retval,
                                     tdat[i].experrno);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_sockettest02
 ****************************************************************************/

void test_nuttx_syscall_sockettest02(FAR void **state)
{
  static struct tcase
  {
    int type;
    int flag;
    int fl_flag;
  }

  tcases[] =
    {
        {
          SOCK_STREAM, 0, F_GETFD
        },

        {
          SOCK_STREAM | SOCK_CLOEXEC, FD_CLOEXEC, F_GETFD
        },

        {
          SOCK_STREAM, 0, F_GETFL
        },

        {
          SOCK_STREAM | SOCK_NONBLOCK, O_NONBLOCK, F_GETFL
        }
    };

  for (int i = 0; i < 4; i++)
    {
      test_nuttx_syscall_verifysocket02(tcases[i].type, tcases[i].flag,
                                     tcases[i].fl_flag);
    }
}
