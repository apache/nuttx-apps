/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/getpeername_test.c
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
#ifndef UCLINUX
#define UCLINUX

#include <nuttx/config.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "SyscallTest.h"

__attribute__((unused)) static struct sockaddr_in server_addr;
__attribute__((unused)) static struct sockaddr_in fsin1;
__attribute__((unused)) static socklen_t sinlen;

/* static socklen_t invalid_sinlen = -1; */

__attribute__((unused)) static void setup(void);
__attribute__((unused)) static void setup2(int);
__attribute__((unused)) static void setup3(int);

/* static void setup4(int); */

__attribute__((unused)) static void cleanup2(int);

/* static void cleanup4(int); */

__attribute__((unused)) static struct test_case_t
{
  int sockfd;
  struct sockaddr *sockaddr;
  socklen_t *addrlen;
  int expretval;
  int experrno;
  void (*setup)(int);
  void (*cleanup)(int);
  char *name;
}

test_cases[] =
{
    {
      -1, (struct sockaddr *)&fsin1, &sinlen, -1, EBADF, NULL, NULL,
     "EBADF"
    },

    {
      -1, (struct sockaddr *)&fsin1, &sinlen, -1, ENOTSOCK, setup2,
     cleanup2, "ENOTSOCK"
    },

    {
      -1, (struct sockaddr *)&fsin1, &sinlen, -1, ENOTCONN, setup3,
     cleanup2, "ENOTCONN"
    },

  /* {
   * -1, (struct sockaddr *)&fsin1, &invalid_sinlen, -1, EINVAL, setup4,
   *  cleanup4, "EINVAL"
   * },
   */
#if 0
#ifndef UCLINUX
    {
    -1, (struct sockaddr *)-1, &sinlen, -1, EFAULT, setup4, cleanup4,
     "EFAULT"
    },

    {
    -1, (struct sockaddr *)&fsin1, NULL, -1, EFAULT, setup4,
     cleanup4, "EFAULT"
    },

    {
    -1, (struct sockaddr *)&fsin1, (socklen_t *)1, -1, EFAULT, setup4,
     cleanup4, "EFAULT"
    },
#endif
#endif
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_getpeername01
 ****************************************************************************/

void test_nuttx_syscall_getpeername01(FAR void **state)
{
#ifdef CONFIG_NET_TCP
  int total = sizeof(test_cases) / sizeof(test_cases[0]);
  int ret;
  setup();
  for (int i = 0; i < total; ++i)
    {
      if (test_cases[i].setup != NULL)
        test_cases[i].setup(i);

      assert_false(test_cases[i].setup != NULL &&
                   test_cases[i].sockfd <= 0);
      ret = getpeername(test_cases[i].sockfd, test_cases[i].sockaddr,
                        test_cases[i].addrlen);

      assert_false(ret != test_cases[i].expretval ||
                   errno != test_cases[i].experrno);
      if (test_cases[i].cleanup != NULL)
        test_cases[i].cleanup(i);
    }
#endif
}

__attribute__((unused)) static void setup(void)
{
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = 0;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  sinlen = sizeof(fsin1);
}

__attribute__((unused)) static void setup2(int i)
{
  int ret;
  ret = open("/dev/null", O_WRONLY, 0666);
  assert_true(ret >= 0);
  test_cases[i].sockfd = ret;
}

__attribute__((unused)) static void setup3(int i)
{
  test_cases[i].sockfd = socket(PF_INET, SOCK_STREAM, 0);
  if (test_cases[i].sockfd < 0)
    {
      return;
    }

  if (safe_bind(test_cases[i].sockfd, (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
      fail_msg("test fail !");
    }
}

/* static void setup4(int i)
 * {
 *   if (socketpair(PF_UNIX, SOCK_STREAM, 0, sv) < 0)
 *     {
 *         syslog(LOG_ERR, "socketpair failed for getpeername test %d",
 *         i); faiL_msg("test fail !"); return;
 *     }
 *   test_cases[i].sockfd = sv[0];
 * }
 */

__attribute__((unused)) static void cleanup2(int i)
{
  if (safe_close(test_cases[i].sockfd) < 0)
    {
      fail_msg("test fail !");
    }
}

#undef UCLINUX
#endif
