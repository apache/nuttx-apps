/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/setsocketopt01_test.c
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
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <syslog.h>
#include <sys/ioctl.h>
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
 * Name: test_nuttx_syscall_setsockopt01
 ****************************************************************************/

void test_nuttx_syscall_setsockopt01(FAR void **state)
{
#if defined(CONFIG_NET) && defined(CONFIG_NET_SOCKOPTS)
  struct sockaddr_in addr;
  int optval;
  struct test_case
  {             /* test case structure */
    int domain; /* PF_INET, PF_UNIX, ... */
    int type;   /* SOCK_STREAM, SOCK_DGRAM ... */
    int proto;  /* protocol number (usually 0 = default) */
    int level;  /* IPPROTO_* */
    int optname;
    void *optval;
    int optlen;
    int experrno; /* expected errno */
    char *desc;
  }

  testcase_list[] =
    {
        {
          -1, -1, -1, SOL_SOCKET, SO_OOBINLINE, &optval, sizeof(optval),
        EBADF, "invalid file descriptor"
        },

        {
          -1, -1, -1, SOL_SOCKET, SO_OOBINLINE, &optval, sizeof(optval),
        ENOTSOCK, "non-socket file descriptor"
        },

        {
          PF_INET, SOCK_STREAM, 0, SOL_SOCKET, SO_OOBINLINE, NULL,
        sizeof(optval), EFAULT, "invalid option buffer"
        },

        {
          PF_INET, SOCK_STREAM, 0, SOL_SOCKET, SO_OOBINLINE, &optval, 0,
        EINVAL, "invalid optlen"
        },

        {
          PF_INET, SOCK_STREAM, 0, 500, SO_OOBINLINE, &optval,
        sizeof(optval), ENOPROTOOPT, "invalid level"
        },

        {
          PF_INET, SOCK_STREAM, 0, IPPROTO_UDP, SO_OOBINLINE, &optval,
        sizeof(optval), ENOPROTOOPT, "invalid option name (UDP)"
        },

        {
          PF_INET, SOCK_STREAM, 0, IPPROTO_IP, -1, &optval, sizeof(optval),
        ENOPROTOOPT, "invalid option name (IP)"
        },

        {
          PF_INET, SOCK_STREAM, 0, IPPROTO_TCP, -1, &optval, sizeof(optval),
        ENOPROTOOPT, "invalid option name (TCP)"
        }
    };

  /* initialize local sockaddr */

  addr.sin_family = AF_INET;
  addr.sin_port = 0;
  addr.sin_addr.s_addr = INADDR_ANY;
  for (int n = 0; n < sizeof(testcase_list) / sizeof(testcase_list[0]);
       n++)
    {
      int ret;
      struct test_case *tc = testcase_list + n;
      int tmpfd;
      int fd;
      if (tc->domain == -1)
        {
          tmpfd = fd = SAFE_OPEN("/dev/null", O_WRONLY);
          if (fd < 0)
            continue;
        }

      else
        {
          tmpfd = fd = safe_socket(tc->domain, tc->type, tc->proto);
          if (fd < 0)
            continue;
          if (safe_bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
            continue;
        }

      /* Use closed file descriptor rather than -1 */

      if (tc->experrno == EBADF)
          if (safe_close(tmpfd) < 0)
            continue;
      ret =
          setsockopt(fd, tc->level, tc->optname, tc->optval, tc->optlen);
      if (tc->experrno != EBADF)
        {
          safe_close(fd);
          fd = -1;
        }

      if (ret != -1 || errno != tc->experrno)
        {
          syslog(LOG_ERR,
                 "setsockopt %s fail, returned %d (expected -1), "
                 "errno %d (expected %d)\n",
                 tc->desc, ret, errno, tc->experrno);
          fail_msg("test fail !");
          continue;
        }
    }

#endif
}
