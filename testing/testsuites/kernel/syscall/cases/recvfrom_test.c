/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/recvfrom_test.c
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
#include <fcntl.h>
#include <syslog.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/syscall.h>
#include <netinet/in.h>
#include "SyscallTest.h"

/****************************************************************************
 * Private data Prototypes
 ****************************************************************************/

__attribute__((unused)) static char buf[1024];
__attribute__((unused)) static int s; /* socket descriptor */
__attribute__((unused)) static int testno;
__attribute__((unused)) static struct sockaddr_in sin1, from;
__attribute__((unused)) static pthread_t thread;
__attribute__((unused)) static int
    sfd; /* shared between do_child and start_server */
__attribute__((unused)) static socklen_t fromlen;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

__attribute__((unused)) static int setup(void), setup0(void),
    setup1(void), setup2(void), cleanup(void), cleanup0(void),
    cleanup1(void);
__attribute__((unused)) static void do_child(void);

__attribute__((unused)) static void *start_server(void *);

__attribute__((unused)) static struct test_case_t
{
  int domain;            /* PF_INET, PF_UNIX, ... */
  int type;              /* SOCK_STREAM, SOCK_DGRAM ... */
  int proto;             /* protocol number (usually 0 = default) */
  void *buf;             /* recv data buffer */
  size_t buflen;         /* recv's 3rd argument */
  unsigned flags;        /* recv's 4th argument */
  struct sockaddr *from; /* from address */
  socklen_t *salen;      /* from address value/result buffer length */
  int retval;            /* syscall return value */
  int experrno;          /* expected errno */
  int (*setup)(void);
  int (*cleanup)(void);
  char *desc;
}

tdat[] =
{
    /* 1 */

    {
      PF_INET, SOCK_STREAM, 0, buf, sizeof(buf), 0,
     (struct sockaddr *)&from, &fromlen, -1, EBADF, setup0, cleanup0,
     "bad file descriptor"
    },

    /* 2 */

    {
      0, 0, 0, buf, sizeof(buf), 0, (struct sockaddr *)&from, &fromlen,
     -1, ENOTSOCK, setup0, cleanup0, "invalid socket"
    },

    /* 3 */

    {
      PF_INET, SOCK_STREAM, 0, (void *)buf, sizeof(buf), 0,
     (struct sockaddr *)-1, &fromlen, 0, ENOTSOCK, setup1, cleanup1,
     "invalid socket buffer"
    },

    /* 4 */

    {
      PF_INET, SOCK_STREAM, 0, (void *)buf, sizeof(buf), 0,
     (struct sockaddr *)&from, &fromlen, -1, EINVAL, setup2, cleanup1,
     "invalid socket addr length"
    },
#ifndef UCLINUX

    /* 5 */

    {
      PF_INET, SOCK_STREAM, 0, (void *)-1, sizeof(buf), 0,
     (struct sockaddr *)&from, &fromlen, -1, EFAULT, setup1, cleanup1,
     "invalid recv buffer"
    },
#endif
    /* 6 */

/* {
 *    PF_INET, SOCK_STREAM, 0, (void *)buf, sizeof(buf), MSG_OOB,
 *  (struct sockaddr *)&from, &fromlen,
 *  -1, EINVAL, setup1, cleanup1, "invalid MSG_OOB flag set"
 * },
 */

    /* 7 */

    {
      PF_INET, SOCK_STREAM, 0, (void *)buf, sizeof(buf),
      MSG_ERRQUEUE, (struct sockaddr *)&from, &fromlen, -1, EAGAIN,
      setup1, cleanup1, "invalid MSG_ERRQUEUE flag set"
    },
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: setup
 ****************************************************************************/

__attribute__((unused)) static int setup(void)
{
  int r = pthread_create(&thread, NULL, start_server, (void *)&sin1);
  if (r < 0)
    {
      syslog(LOG_ERR, "pthread_create fail, errno %d\n", errno);
      return -1;
    }

  return 0;
}

/****************************************************************************
 * Name: cleanup
 ****************************************************************************/

__attribute__((unused)) static int cleanup(void)
{
  assert_true(pthread_cancel(thread) == 0);
  assert_true(pthread_join(thread, NULL) == 0);
  return 0;
}

/****************************************************************************
 * Name: setup0
 ****************************************************************************/

__attribute__((unused)) static int setup0(void)
{
  if (tdat[testno].experrno == EBADF)
    {
      s = 400; /* anything not an open file */
    }

  else if ((s = open("/dev/null", O_WRONLY)) == -1)
    {
      syslog(LOG_ERR, "open /dev/null fail, errno %d\n", errno);
      fail_msg("test fail !");
      return -1;
    }

  fromlen = sizeof(from);
  return 0;
}

/****************************************************************************
 * Name: cleanup0
 ****************************************************************************/

__attribute__((unused)) static int cleanup0(void)
{
  s = -1;
  return 0;
}

/****************************************************************************
 * Name: setup1
 ****************************************************************************/

__attribute__((unused)) static int setup1(void)
{
#ifdef CONFIG_NET_TCP
  fd_set rdfds;
  struct timeval timeout;
  int n;

  s = safe_socket(tdat[testno].domain, tdat[testno].type,
                  tdat[testno].proto);
  assert_true(s >= 0);
  if (tdat[testno].type == SOCK_STREAM &&
      connect(s, (struct sockaddr *)&sin1, sizeof(sin1)) < 0)
    {
      syslog(LOG_ERR, "connect failed, errno %d\n", errno);
      fail_msg("test fail");
    }

  /* Wait for something to be readable, else we won't detect EFAULT on
   * recv
   */

  FD_ZERO(&rdfds);
  FD_SET(s, &rdfds);
  timeout.tv_sec = 2;
  timeout.tv_usec = 0;
  n = select(s + 1, &rdfds, 0, 0, &timeout);
  if (n != 1 || !FD_ISSET(s, &rdfds))
    {
      syslog(
          LOG_ERR,
          "client setup1 failed - no message ready in 2 sec, errno %d\n",
          errno);
      fail_msg("test fail");
    }

  fromlen = sizeof(from);
#endif
  return s;
}

/****************************************************************************
 * Name: setup2
 ****************************************************************************/

__attribute__((unused)) static int setup2(void)
{
  setup1();
  fromlen = -1;
  return 0;
}

/****************************************************************************
 * Name: cleanup1
 ****************************************************************************/

__attribute__((unused)) static int cleanup1(void)
{
  (void)close(s);
  s = -1;
  return 0;
}

/****************************************************************************
 * Name: start_server
 ****************************************************************************/

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
      fail_msg("test fail");
      return NULL;
    }

  if (listen(sfd, 10) < 0)
    {
      syslog(LOG_ERR, "server listen failed\n");
      close(sfd);
      fail_msg("test fail");
      return NULL;
    }

  /* assert_int_not_equal(-1,safe_getsockname(sfd, (struct sockaddr
   * *)sin0, &slen));
   */

  safe_getsockname(sfd, (struct sockaddr *)sin0, &slen);
  do_child();
  return NULL;
}

/****************************************************************************
 * Name: do_child
 ****************************************************************************/

__attribute__((unused)) static void do_child(void)
{
  struct sockaddr_in fsin;
  fd_set afds;
  fd_set rfds;
  int nfds;
  int cc;
  int fd;

  FD_ZERO(&afds);
  FD_SET(sfd, &afds);

  nfds = sfd + 1;

  /* accept connections until killed */

  while (1)
    {
      socklen_t fromlen_s;
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

          fromlen_s = sizeof(fsin);
          newfd = accept(sfd, (struct sockaddr *)&fsin, &fromlen_s);
          if (newfd >= 0)
            {
              FD_SET(newfd, &afds);
              nfds = (nfds > newfd + 1) ? (nfds) : (newfd + 1);

              /* send something back */

              (void)write(newfd, "hoser\n", 6);
            }
        }

      for (fd = 0; fd < nfds; ++fd)
          if (fd != sfd && FD_ISSET(fd, &rfds))
            {
              cc = read(fd, buf, sizeof(buf));
              if (cc == 0 || (cc < 0 && errno != EINTR))
                {
                  (void)close(fd);
                  FD_CLR(fd, &afds);
                }
            }

      pthread_testcancel();
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_recvfromtest01
 ****************************************************************************/

void test_nuttx_syscall_recvfromtest01(FAR void **state)
{
#ifdef CONFIG_NET_TCP
  int ret;
  int flag = 1;
  setup();

  for (testno = 0; testno < sizeof(tdat) / sizeof(tdat[0]); ++testno)
    {
      usleep(100000);
      if (tdat[testno].setup() < 0)
        {
          tdat[testno].cleanup();
          continue;
        }

      ret = recvfrom(s, tdat[testno].buf, tdat[testno].buflen,
                     tdat[testno].flags, tdat[testno].from,
                     tdat[testno].salen);
      if (ret >= 0)
        {
          ret = 0; /* all nonzero equal here */
        }

      if (ret != tdat[testno].retval ||
          (ret < 0 && errno != tdat[testno].experrno))
        {
          syslog(LOG_ERR,
                 "test recvfrom01 %s failed; "
                 "returned %d (expected %d), errno %d "
                 "(expected %d)\n",
                 tdat[testno].desc, ret, tdat[testno].retval, errno,
                 tdat[testno].experrno);
          flag = 0;
        }

      tdat[testno].cleanup();
    }

  cleanup();
  assert_true(flag);
#endif
}

#undef UCLINUX
#endif
