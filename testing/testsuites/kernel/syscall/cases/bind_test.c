/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/bind_test.c
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
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <grp.h>
#include <pwd.h>
#include <syslog.h>
#include <pthread.h>
#include <netinet/in.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_bind01
 ****************************************************************************/

void test_nuttx_syscall_bind01(FAR void **state)
{
  int inet_socket;
  int dev_null;

  struct sockaddr_in sin1;
  struct sockaddr_in sin2;
  struct sockaddr_un sun;
  struct test_case
  {
    int *socket_fd;
    struct sockaddr *sockaddr;
    socklen_t salen;
    int retval;
    int experrno;
    char *desc;
  }

  tcases[] =
    {
        {
          &inet_socket, (struct sockaddr *)&sin1, 3, -1, EINVAL,
          "invalid salen"
        },

        {
          &dev_null, (struct sockaddr *)&sin1, sizeof(sin1), -1, ENOTSOCK,
          "invalid socket"
        },

        {
          &inet_socket, (struct sockaddr *)&sin2, sizeof(sin2), 0, 0,
          "INADDR_ANYPORT"
        },

        {
          &inet_socket, (struct sockaddr *)&sun, sizeof(sun), -1,
          EAFNOSUPPORT, "UNIX-domain of current directory"
        },
    };

  /* initialize sockaddr's */

  sin1.sin_family = AF_INET;
  sin1.sin_port = 25197;
  sin1.sin_addr.s_addr = INADDR_ANY;
  sin2.sin_family = AF_INET;
  sin2.sin_port = 0;
  sin2.sin_addr.s_addr = INADDR_ANY;

  sun.sun_family = AF_UNIX;
  strncpy(sun.sun_path, ".", sizeof(sun.sun_path));
  inet_socket = socket(PF_INET, SOCK_STREAM, 0);
  dev_null = open("/dev/null", O_WRONLY);

  int test_ret;
  int test_err;
  int i;
  for (i = 0; i < 4; i++)
    {
      inet_socket = socket(PF_INET, SOCK_STREAM, 0);
      dev_null = open("/dev/null", O_WRONLY);
      if (tcases[i].experrno)
        {
          test_ret = bind(*tcases[i].socket_fd, tcases[i].sockaddr,
                          tcases[i].salen);
          test_err = errno;
          close(inet_socket);
          close(dev_null);
          assert_int_not_equal(test_ret, 0);
          assert_int_equal(test_ret, -1);
          assert_int_equal(test_err, tcases[i].experrno);
        }

      else
        {
          test_ret = bind(*tcases[i].socket_fd, tcases[i].sockaddr,
                          tcases[i].salen);
          close(inet_socket);
          close(dev_null);
          assert_int_not_equal(test_ret, -1);
          assert_int_equal(test_ret, 0);
        }
    }
}
