/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/accept_test.c
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
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include "SyscallTest.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_verityaccept01
 ****************************************************************************/

__attribute__((unused)) void test_nuttx_syscall_verityaccept01(
    int testno, int domain, int type, int proto, int fd,
    struct sockaddr *sockaddr, unsigned int *salen, int experrno)
{
#ifdef CONFIG_NET_TCP
  int ret;
  errno = 0;
  ret = accept(fd, sockaddr, salen);
  close(fd);
  assert_int_equal(ret, -1);

  /* syslog(LOG_INFO, "ret=%d   errnor=%d\n experrno=%d", ret,
   * errno,experrno);
   */

#endif
}

/****************************************************************************
 * Name: test_nuttx_syscall_accept01
 ****************************************************************************/

void test_nuttx_syscall_accept01(FAR void **state)
{
#ifdef CONFIG_NET_TCP
  struct sockaddr_in sin0;
  struct sockaddr_in sin1;
  struct sockaddr_in fsin1;
  int invalid_socketfd = -400;
  int devnull_fd;
  int socket_fd;
  int udp_fd;

  sin0.sin_family = AF_INET;
  sin0.sin_port = 0;
  sin0.sin_addr.s_addr = INADDR_ANY;

  devnull_fd = open("/dev/null", O_WRONLY);

  socket_fd = socket(PF_INET, SOCK_STREAM, 0);
  assert_int_not_equal(socket_fd, -1);
  bind(socket_fd, (struct sockaddr *)&sin0, sizeof(sin0));

  sin1.sin_family = AF_INET;
  sin1.sin_port = 0;
  sin1.sin_addr.s_addr = INADDR_ANY;
  udp_fd = socket(PF_INET, SOCK_DGRAM, 0);
  assert_int_not_equal(udp_fd, -1);
  bind(udp_fd, (struct sockaddr *)&sin1, sizeof(sin1));

  struct test_case
  {
    int domain;
    int type;
    int proto;
    int *fd;
    struct sockaddr *sockaddr;
    socklen_t salen;
    int experrno;
  }

  tcases[] =
    {
        {
          PF_INET, SOCK_STREAM, 0, &invalid_socketfd,
          (struct sockaddr *)&fsin1, sizeof(fsin1), EBADF
        },

        {
          PF_INET, SOCK_STREAM, 0, &devnull_fd, (struct sockaddr *)&fsin1,
          sizeof(fsin1), ENOTSOCK
        },

        {
          PF_INET, SOCK_STREAM, 0, &socket_fd, (struct sockaddr *)3,
          sizeof(fsin1), EINVAL
        },

        {
          PF_INET, SOCK_STREAM, 0, &socket_fd, (struct sockaddr *)&fsin1, 1,
          EINVAL
        },

        {
          PF_INET, SOCK_STREAM, 0, &socket_fd, (struct sockaddr *)&fsin1,
          sizeof(fsin1), EINVAL
        },

        {
          PF_INET, SOCK_STREAM, 0, &udp_fd, (struct sockaddr *)&fsin1,
          sizeof(fsin1), EOPNOTSUPP
        },
    };

  for (int i = 0; i < 6; i++)
    {
      test_nuttx_syscall_verityaccept01(
          i, tcases[i].domain, tcases[i].type, tcases[i].proto,
          *tcases[i].fd, tcases[i].sockaddr, &tcases[i].salen,
          tcases[i].experrno);
    }
#endif
}
