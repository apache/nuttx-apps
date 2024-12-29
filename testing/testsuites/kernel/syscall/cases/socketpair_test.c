/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/socketpair_test.c
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
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <syslog.h>
#include <sys/syscall.h>
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
 * Name: test_nuttx_syscall_socketpair01
 ****************************************************************************/

void test_nuttx_syscall_socketpair01(FAR void **state)
{
#ifdef CONFIG_NET_TCP
  int fds[2];
  int ret;
  struct test_case_t
  {
    int domain;
    int type;
    int proto;
    int *sv;
    int retval;
    int experrno;
    char *desc;
  }

  tdat[] =
    {
        {
          0, SOCK_STREAM, 0, fds, -1, EAFNOSUPPORT, "invalid domain"
        },

        {
          PF_INET, 75, 0, fds, -1, EINVAL, "invalid type"
        },

        {
          PF_UNIX, SOCK_DGRAM, 0, fds, 0, 0, "UNIX domain dgram"
        },

        {
          PF_INET, SOCK_RAW, 0, fds, -1, EPROTONOSUPPORT,
        "raw open as non-root"
        },

  #ifndef UCLINUX
        {
          PF_UNIX, SOCK_STREAM, 0, 0, -1, EFAULT, "bad aligned pointer"
        },

        {
          PF_UNIX, SOCK_STREAM, 0, (int *)7, -1, EFAULT,
        "bad unaligned pointer"
        },

  #endif
      /* {PF_INET, SOCK_DGRAM, 17, fds, -1, EOPNOTSUPP, "UDP socket"}, */

        {
          PF_INET, SOCK_DGRAM, 6, fds, -1, EPROTONOSUPPORT, "TCP dgram"
        },

      /* {PF_INET, SOCK_STREAM, 6, fds, -1, EOPNOTSUPP, "TCP socket"}, */

        {
          PF_INET, SOCK_STREAM, 1, fds, -1, EPROTONOSUPPORT,
        "ICMP stream"
        }
    };

  for (int n = 0; n < sizeof(tdat) / sizeof(tdat[0]); n++)
    {
      struct test_case_t *tc = &tdat[n];
      ret = socketpair(tc->domain, tc->type, tc->proto, tc->sv);
      if (ret == 0)
        {
          SAFE_CLOSE(fds[0]);
          SAFE_CLOSE(fds[1]);
        }

      if (ret != tc->retval || errno != tc->experrno)
        {
          syslog(LOG_ERR,
                 "test socketpair01 %s failed; "
                 "returned %d (expected %d), errno %d "
                 "(expected %d)\n",
                 tc->desc, ret, tc->retval, errno, tc->experrno);
          assert_true(0);
        }
    }

#endif
}

/****************************************************************************
 * Name: test_nuttx_syscall_socketpair02
 ****************************************************************************/

void test_nuttx_syscall_socketpair02(FAR void **state)
{
#ifdef CONFIG_NET_TCP
  int fds[2];
  int ret;
  int flag = 1;
  struct tcase
  {
    int type;
    int flag;
    int fl_flag;
    char *des;
  }

  tcases[] =
    {
        {
          SOCK_STREAM, 0, F_GETFD, "no close-on-exec"
        },

        {
          SOCK_STREAM | SOCK_CLOEXEC, FD_CLOEXEC, F_GETFD, "close-on-exec"
        },

        {
          SOCK_STREAM, 0, F_GETFL, "no non-blocking"
        },

        {
          SOCK_STREAM | SOCK_NONBLOCK, O_NONBLOCK, F_GETFL,
        "non-blocking"
        }
    };

  for (int n = 0; n < sizeof(tcases) / sizeof(tcases[0]); n++)
    {
      int res;
      struct tcase *tc = &tcases[n];
      ret = socketpair(PF_UNIX, tc->type, 0, fds);
      if (ret == -1)
        {
          flag = 0;
          syslog(LOG_ERR,
                 "test socketpair02 %s failed; "
                 "returned -1, errno %d\n",
                 tc->des, errno);
          continue;
        }

      for (int i = 0; i < 2; i++)
        {
          res = SAFE_FCNTL(fds[i], tc->fl_flag);

          if (tc->flag != 0 && (res & tc->flag) == 0)
            {
              flag = 0;
              syslog(LOG_ERR,
                     "socketpair() failed to set %s flag for fds[%d]\n",
                     tc->des, i);
              break;
            }

          if (tc->flag == 0 && (res & tc->flag) != 0)
            {
              flag = 0;
              syslog(LOG_ERR,
                     "socketpair() failed to set %s flag for fds[%d]\n",
                     tc->des, i);
              break;
            }
        }

      SAFE_CLOSE(fds[0]);
      SAFE_CLOSE(fds[1]);
    }

  if (fds[0] > 0)
    {
      SAFE_CLOSE(fds[0]);
    }

  if (fds[1] > 0)
    {
      SAFE_CLOSE(fds[1]);
    }

  assert_true(flag);
#endif
}

#undef UCLINUX
#endif
