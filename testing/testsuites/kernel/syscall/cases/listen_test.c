/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/listen_test.c
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
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
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
 * Name: test_nuttx_syscall_listen01
 ****************************************************************************/

void test_nuttx_syscall_listen01(FAR void **state)
{
#ifdef CONFIG_NET_TCP
  int testno;
  int flag = 1;
  int s; /* socket descriptor */
  struct test_case_t
  {               /* test case structure */
    int domain;   /* PF_INET, PF_UNIX, ... */
    int type;     /* SOCK_STREAM, SOCK_DGRAM ... */
    int proto;    /* protocol number (usually 0 = default) */
    int backlog;  /* connect's 3rd argument */
    int retval;   /* syscall return value */
    int experrno; /* expected errno */
    const char setup[10];
    const char cleanup[10];
    char desc[20];
  }

  tdat[] =
    {
        {
          0, 0, 0, 0, -1, EBADF, "setup0", "cleanup0",
        "bad file descriptor"
        },

        {
          0, 0, 0, 0, -1, ENOTSOCK, "setup0", "cleanup0", "not a socket"
        },

        {
          PF_INET, SOCK_DGRAM, 0, 0, -1, EOPNOTSUPP, "setup1", "cleanup1",
        "UDP listen"
        },
    };

  const char set[] = "setup0";
  const char clean[] = "cleanup0";

  for (testno = 0; testno < sizeof(tdat) / sizeof(tdat[0]); ++testno)
    {
      /* setup0(void) */

      if (!strcmp(tdat[testno].setup, set))
        {
          if (tdat[testno].experrno == EBADF)
            s = -400; /* anything not an open file */
          else if ((s = open("/dev/null", O_WRONLY)) == -1)
            syslog(LOG_ERR, "error opening /dev/null - errno: %s\n",
                   strerror(errno));
        }

      else
        {
          /* setup1(void) */

          s = safe_socket(tdat[testno].domain, tdat[testno].type,
                          tdat[testno].proto);
        }

      int rec = safe_listen(s, tdat[testno].backlog);

      if (rec != tdat[testno].retval ||
          (rec < 0 && errno != tdat[testno].experrno))
        {
          syslog(LOG_ERR,
                 "%s ; returned"
                 " %d (expected %d), errno %d (expected"
                 " %d)\n",
                 tdat[testno].desc, rec, tdat[testno].retval, errno,
                 tdat[testno].experrno);
          flag = 0;
        }

      else
        {
          syslog(LOG_INFO, "%s successful\n", tdat[testno].desc);
        }

      if (!strcmp(tdat[testno].cleanup, clean))
        {
          /* cleanup0(void) */

          s = -1;
        }

      else
        {
          /* cleanup1(void) */

          (void)close(s);
          s = -1;
        }
    }

  assert_true(flag);
#endif
}
