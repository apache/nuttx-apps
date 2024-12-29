/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/connect_test.c
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
#include <pthread.h>
#include <errno.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <netinet/in.h>
#ifdef CONFIG_FDCHECK
#include <nuttx/fdcheck.h>
#endif
#include "SyscallTest.h"

__attribute__((unused)) static int s; /* socket descriptor */
__attribute__((unused)) static int testno;
__attribute__((unused)) static struct sockaddr_in sin1, sin2, sin4;
__attribute__((unused)) static pthread_t thread;
__attribute__((unused)) static int
    sfd; /* shared between start_server and do_child */
__attribute__((unused)) static int setup(void), setup0(void),
    setup1(void), setup2(void), cleanup(void), cleanup0(void),
    cleanup1(void);
__attribute__((unused)) static void do_child(void);

__attribute__((unused)) static void *start_server(void *);

__attribute__((unused)) static struct test_case_t
{
  int domain;                /* PF_INET, PF_UNIX, ... */
  int type;                  /* SOCK_STREAM, SOCK_DGRAM ... */
  int proto;                 /* protocol number (usually 0 = default) */
  struct sockaddr *sockaddr; /* socket address buffer */
  int salen;                 /* connect's 3rd argument */
  int retval;                /* syscall return value */
  int experrno;              /* expected errno */
  int (*setup)(void);
  int (*cleanup)(void);
  char *desc;
}

tdat[] =
    {
        {
          PF_INET, SOCK_STREAM, 0, (struct sockaddr *)&sin1,
        sizeof(struct sockaddr_in), -1, EBADF, setup0, cleanup0,
        "bad file descriptor"
        },
    #ifndef UCLINUX

        /* Skip since uClinux does not implement memory protection */

        {
          PF_INET, SOCK_STREAM, 0, (struct sockaddr *)-1,
        sizeof(struct sockaddr_in), -1, EFAULT, setup1, cleanup1,
        "invalid socket buffer"
        },

    #endif
        {
          PF_INET, SOCK_STREAM, 0, (struct sockaddr *)&sin1, 3, -1, EINVAL,
        setup1, cleanup1, "invalid salen"
        },

        {
          0, 0, 0, (struct sockaddr *)&sin1, sizeof(sin1), -1, ENOTSOCK,
        setup0, cleanup0, "invalid socket"
        },

        {
          PF_INET, SOCK_STREAM, 0, (struct sockaddr *)&sin1,
          sizeof(sin1), -1, EISCONN, setup2, cleanup1,
          "already connected"
        },

        {
          PF_INET, SOCK_STREAM, 0, (struct sockaddr *)&sin2,
          sizeof(sin2), -1, ECONNREFUSED, setup1, cleanup1,
          "connection refused"
        },

  /* if CONFIG_DEBUG_ASSERTIONS is set,
   * the connect operation will be crash
   */

    #ifndef CONFIG_DEBUG_ASSERTIONS
        {
          PF_INET, SOCK_STREAM, 0, (struct sockaddr *)&sin4,
          sizeof(sin4), -1, EAFNOSUPPORT, setup1, cleanup1,
          "invalid address family"
        },
    #endif
    };

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_connect01
 ****************************************************************************/

void test_nuttx_syscall_connect01(FAR void **state)
{
#ifdef CONFIG_NET_TCP
  int ret;
  int flag = 1;
  setup();

  for (testno = 0; testno < sizeof(tdat) / sizeof((tdat)[0]); ++testno)
    {
      usleep(100000);
      if (tdat[testno].setup() < 0)
        {
          tdat[testno].cleanup();
          continue;
        }

      ret = connect(s, tdat[testno].sockaddr, tdat[testno].salen);

      if (ret != tdat[testno].retval ||
          (ret < 0 && errno != tdat[testno].experrno))
        {
          syslog(LOG_ERR,
                 "test connect %s failed; "
                 "returned %d (expected %d), errno %d "
                 "(expected %d)\n",
                 tdat[testno].desc, ret, tdat[testno].retval, errno,
                 tdat[testno].experrno);

          /* fail_msg("test fail !"); */

          flag = 1;
        }

      tdat[testno].cleanup();
    }

  cleanup();
  assert_true(flag);
#endif
}

__attribute__((unused)) static int setup(void)
{
  int r = pthread_create(&thread, NULL, start_server, (void *)&sin1);
  if (r < 0)
      return -1;

  sin2.sin_family = AF_INET;

  /* this port must be unused! */

  sin2.sin_port = get_unused_port(NULL, AF_INET, SOCK_STREAM);
  if (sin2.sin_port < 0)
      return -1;
  sin2.sin_addr.s_addr = INADDR_ANY;

  sin4.sin_family = 47; /* bogus address family */
  sin4.sin_port = 0;
  sin4.sin_addr.s_addr = htonl(0x0afffefd);
  return 0;
}

__attribute__((unused)) static int cleanup(void)
{
  assert_true(pthread_cancel(thread) == 0);
  assert_true(pthread_join(thread, NULL) == 0);
  return 0;
}

__attribute__((unused)) static int setup0(void)
{
  if (tdat[testno].experrno == EBADF)
    s = -400; /* anything not an open file */
  else if ((s = open("/dev/null", O_WRONLY)) == -1)
    {
      syslog(LOG_ERR, "open /dev/null fail, errno %d\n", errno);
      fail_msg("test fail !");
      return -1;
    }

  return 0;
}

__attribute__((unused)) static int cleanup0(void)
{
  close(s);
  s = -1;
  return 0;
}

__attribute__((unused)) static int setup1(void)
{
  s = safe_socket(tdat[testno].domain, tdat[testno].type,
                  tdat[testno].proto);
  return s;
}

__attribute__((unused)) static int cleanup1(void)
{
  (void)close(s);
  s = -1;
  return 0;
}

__attribute__((unused)) static int setup2(void)
{
  if (setup1() < 0) /* get a socket in s */
      return -1;
  return safe_connect(s, (const struct sockaddr *)&sin1, sizeof(sin1));
}

__attribute__((unused)) static void *start_server(void *arg)
{
  /* 设置取消状态为启用 */

  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

  /* 设置取消类型为异步取消 */

  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

  struct sockaddr_in *sin0 = (struct sockaddr_in *)arg;
  socklen_t slen = sizeof(*sin0);

  sin0->sin_family = AF_INET;
  sin0->sin_port = 0; /* pick random free port */
  sin0->sin_addr.s_addr = INADDR_ANY;

  sfd = socket(PF_INET, SOCK_STREAM, 0);
  if (sfd < 0)
    {
      syslog(LOG_ERR, "server socket failed\n");
      fail_msg("test fail !");
      return NULL;
    }

  if (bind(sfd, (struct sockaddr *)sin0, sizeof(*sin0)) < 0)
    {
      syslog(LOG_ERR, "server bind failed\n");
      close(sfd);
      fail_msg("test fail !");
      return NULL;
    }

  if (listen(sfd, 10) < 0)
    {
      syslog(LOG_ERR, "server listen failed\n");
      close(sfd);
      fail_msg("test fail !");
      return NULL;
    }

  safe_getsockname(sfd, (struct sockaddr *)sin0, &slen);
  do_child();
  return NULL;
}

__attribute__((unused)) static void do_child(void)
{
  struct sockaddr_in fsin;
  fd_set afds;
  fd_set rfds;
  int nfds;
  int cc;
  int fd;
#ifdef CONFIG_FDCHECK
  int fd_tmp = 0;
#endif
  char c;

  FD_ZERO(&afds);
  FD_SET(sfd, &afds);

  nfds = sfd + 1;

  /* accept connections until killed */

  while (1)
    {
      socklen_t fromlen;

      memcpy(&rfds, &afds, sizeof(rfds));

      if (select(nfds, &rfds, NULL, NULL, NULL) < 0)
          if (errno != EINTR)
            {
              syslog(LOG_ERR, "select exit fail\n");
              fail_msg("test fail !");
              return;
            }

      if (FD_ISSET(sfd, &rfds))
        {
          int newfd;

          fromlen = sizeof(fsin);
          newfd = accept(sfd, (struct sockaddr *)&fsin, &fromlen);
          if (newfd >= 0)
            {
              FD_SET(newfd, &afds);
              nfds = (nfds > newfd + 1) ? (nfds) : (newfd + 1);
            }
        }

#ifdef CONFIG_FDCHECK
      for (fd = 0; fd < fdcheck_restore(nfds - 1) + 1; ++fd)
        {
          fd_tmp = fdcheck_protect(fd);
          if (fd_tmp != sfd && FD_ISSET(fd_tmp, &rfds))
            {
              if ((cc = read(fd_tmp, &c, 1)) == 0)
                {
                  (void)close(fd_tmp);
                  FD_CLR(fd_tmp, &afds);
                }
            }
        }

#else
      for (fd = 0; fd < nfds; ++fd)
          if (fd != sfd && FD_ISSET(fd, &rfds))
            {
              if ((cc = read(fd, &c, 1)) == 0)
                {
                  (void)close(fd);
                  FD_CLR(fd, &afds);
                }
            }

#endif
      pthread_testcancel();
    }
}

#undef UCLINUX
#endif
