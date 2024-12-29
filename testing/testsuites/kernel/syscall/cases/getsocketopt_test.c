/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/getsocketopt_test.c
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
#include <netinet/in.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <fcntl.h>
#include "SyscallTest.h"

__attribute__((unused)) static int testno;
__attribute__((unused)) static int s; /* socket descriptor */
__attribute__((unused)) static struct sockaddr_in sin0, fsin1;
__attribute__((unused)) static int sinlen;
__attribute__((unused)) static int optval;
__attribute__((unused)) static socklen_t optlen;

__attribute__((unused)) static void setup(void);
__attribute__((unused)) static void setup0(void);
__attribute__((unused)) static void setup1(void);
__attribute__((unused)) static void cleanup0(void);
__attribute__((unused)) static void cleanup1(void);

__attribute__((unused)) static struct test_case_t
{
  int domain; /* PF_INET, PF_UNIX, ... */
  int type;   /* SOCK_STREAM, SOCK_DGRAM ... */
  int proto;  /* protocol number (usually 0 = default) */
  int level;  /* IPPROTO_* */
  int optname;
  void *optval;
  socklen_t *optlen;
  struct sockaddr *sin;
  int salen;
  int retval;   /* syscall return value */
  int experrno; /* expected errno */
  void (*setup)(void);
  void (*cleanup)(void);
  char *desc;
}

tdat[] =
{
    {
      PF_INET, SOCK_STREAM, 0, SOL_SOCKET, SO_OOBINLINE, &optval, &optlen,
     (struct sockaddr *)&fsin1, sizeof(fsin1), -1, EBADF, setup0,
     cleanup0, "bad file descriptor"
    },

    {
      PF_INET, SOCK_STREAM, 0, SOL_SOCKET, SO_OOBINLINE, &optval, &optlen,
     (struct sockaddr *)&fsin1, sizeof(fsin1), -1, ENOTSOCK, setup0,
     cleanup0, "bad file descriptor"
    },
#ifndef UCLINUX
    {
      PF_INET, SOCK_STREAM, 0, SOL_SOCKET, SO_OOBINLINE, 0, &optlen,
     (struct sockaddr *)&fsin1, sizeof(fsin1), -1, EFAULT, setup1,
     cleanup1, "invalid option buffer"
    },

    {
      PF_INET, SOCK_STREAM, 0, SOL_SOCKET, SO_OOBINLINE, &optval, 0,
     (struct sockaddr *)&fsin1, sizeof(fsin1), -1, EFAULT, setup1,
     cleanup1, "invalid optlen"
    },
#endif /* UCLINUX */
    /* {PF_INET, SOCK_STREAM, 0, 500, SO_OOBINLINE, &optval, &optlen,
     * (struct sockaddr *)&fsin1, sizeof(fsin1), -1,
     * EOPNOTSUPP, setup1, cleanup1, "invalid level"},
     * {PF_INET, SOCK_STREAM, 0, IPPROTO_UDP, SO_OOBINLINE, &optval,
     * &optlen, (struct sockaddr *)&fsin1, sizeof(fsin1),
     * -1, EOPNOTSUPP, setup1, cleanup1, "invalid option name"},
     * {PF_INET, SOCK_STREAM, 0, IPPROTO_UDP, SO_OOBINLINE, &optval,
     * &optlen, (struct sockaddr *)&fsin1, sizeof(fsin1),
     * -1, EOPNOTSUPP, setup1, cleanup1,
     * "invalid option name (UDP)"},
     */

    {
      PF_INET, SOCK_STREAM, 0, IPPROTO_IP, -1, &optval, &optlen,
     (struct sockaddr *)&fsin1, sizeof(fsin1), -1, ENOPROTOOPT, setup1,
     cleanup1, "invalid option name (IP)"
    },

    {
      PF_INET, SOCK_STREAM, 0, IPPROTO_TCP, -1, &optval, &optlen,
     (struct sockaddr *)&fsin1, sizeof(fsin1), -1, ENOPROTOOPT, setup1,
     cleanup1, "invalid option name (TCP)"
    },
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_getsockopt01
 ****************************************************************************/

void test_nuttx_syscall_getsockopt01(FAR void **state)
{
#if defined(CONFIG_NET) && defined(CONFIG_NET_SOCKOPTS)
  int ret;
  int flag = 1;
  setup();
  for (testno = 0; testno < sizeof(tdat) / sizeof(tdat[0]); ++testno)
    {
      tdat[testno].setup();
      if (s < 0)
        continue;
      ret = getsockopt(s, tdat[testno].level, tdat[testno].optname,
                       tdat[testno].optval, tdat[testno].optlen);
      if (ret != tdat[testno].retval || errno != tdat[testno].experrno)
        {
          syslog(LOG_ERR,
                 "%s ; returned"
                 " %d (expected %d), errno %d (expected"
                 " %d)\n",
                 tdat[testno].desc, ret, tdat[testno].retval, errno,
                 tdat[testno].experrno);
          flag = 0;
        }

      tdat[testno].cleanup();
    }

  assert_true(flag);
#endif
}

__attribute__((unused)) static void setup(void)
{
  /* initialize local sockaddr */

  sin0.sin_family = AF_INET;
  sin0.sin_port = 0;
  sin0.sin_addr.s_addr = INADDR_ANY;
}

__attribute__((unused)) static void setup0(void)
{
  if (tdat[testno].experrno == EBADF)
    s = -400; /* anything not an open file */
  else if ((s = open("/dev/null", O_WRONLY)) == -1)
    {
      syslog(LOG_ERR,
             "error opening /dev/null - "
             "errno: %s",
             strerror(errno));
      fail_msg("test fail !");
    }
}

__attribute__((unused)) static void cleanup0(void)
{
  if (tdat[testno].experrno != EBADF && s > 0)
    (void)close(s);
  s = -1;
}

__attribute__((unused)) static void setup1(void)
{
  s = safe_socket(tdat[testno].domain, tdat[testno].type,
                  tdat[testno].proto);
  if (s < 0)
    fail_msg("setup1 fail");
  int ret = safe_bind(s, (struct sockaddr *)&sin0, sizeof(sin0));
  assert_int_not_equal(ret, -1);
  sinlen = sizeof(fsin1);
  optlen = sizeof(optval);
}

__attribute__((unused)) static void cleanup1(void)
{
  (void)close(s);
  s = -1;
}

#undef UCLINUX
#endif
