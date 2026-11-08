/****************************************************************************
 * apps/testing/nettest/local/test_local_peercred.c
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
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <cmocka.h>

#include "test_local.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int test_local_peercred_get(int fd, FAR struct ucred *uc)
{
  socklen_t len = sizeof(*uc);

  memset(uc, 0, sizeof(*uc));
  return getsockopt(fd, SOL_SOCKET, SO_PEERCRED, uc, &len);
}

/* True if fd gives the credentials of process 'pid' of this user */

static bool test_local_peercred_is(int fd, pid_t pid)
{
  struct ucred uc;

  return test_local_peercred_get(fd, &uc) == 0 && uc.pid == pid &&
         uc.uid == getuid() && uc.gid == getgid();
}

/* True if fd has no peer.  As on Linux, the call succeeds and gives pid 0,
 * uid -1 and gid -1.
 */

static bool test_local_peercred_none(int fd)
{
  struct ucred uc;

  return test_local_peercred_get(fd, &uc) == 0 && uc.pid == 0 &&
         uc.uid == -1 && uc.gid == -1;
}

/* A stream socket bound to a name in P_tmpdir */

static int test_local_peercred_bind(FAR struct sockaddr_un *addr)
{
  int ret;
  int fd;

  fd = socket(AF_UNIX, SOCK_STREAM, 0);
  assert_return_code(fd, errno);

  memset(addr, 0, sizeof(*addr));
  addr->sun_family = AF_UNIX;
  snprintf(addr->sun_path, sizeof(addr->sun_path), "%s/peercred.%d",
           P_tmpdir, getpid());
  unlink(addr->sun_path);
  ret = bind(fd, (FAR struct sockaddr *)addr, sizeof(*addr));
  assert_return_code(ret, errno);
  return fd;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_local_peercred_pair
 *
 * Description:
 *   Both ends of a stream and a datagram socketpair() give the process
 *   that made the pair.
 *
 ****************************************************************************/

void test_local_peercred_pair(FAR void **state)
{
  pid_t me = getpid();
  bool ok;
  int sv[2];
  int dv[2];
  int ret;

  ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
  assert_return_code(ret, errno);
  ret = socketpair(AF_UNIX, SOCK_DGRAM, 0, dv);
  assert_return_code(ret, errno);

  ok = test_local_peercred_is(sv[0], me) &&
       test_local_peercred_is(sv[1], me) &&
       test_local_peercred_is(dv[0], me) &&
       test_local_peercred_is(dv[1], me);

  close(sv[0]);
  close(sv[1]);
  close(dv[0]);
  close(dv[1]);
  assert_true(ok);
}

/****************************************************************************
 * Name: test_local_peercred_unconn
 ****************************************************************************/

void test_local_peercred_unconn(FAR void **state)
{
  bool ok;
  int fd;

  fd = socket(AF_UNIX, SOCK_STREAM, 0);
  assert_return_code(fd, errno);
  ok = test_local_peercred_none(fd);
  close(fd);
  assert_true(ok);
}

/****************************************************************************
 * Name: test_local_peercred_listen
 ****************************************************************************/

void test_local_peercred_listen(FAR void **state)
{
  struct sockaddr_un addr;
  bool ok;
  int fd;

  fd = test_local_peercred_bind(&addr);
  ok = listen(fd, 1) == 0 && test_local_peercred_is(fd, getpid());
  close(fd);
  unlink(addr.sun_path);
  assert_true(ok);
}

#ifdef CONFIG_ARCH_HAVE_FORK
/****************************************************************************
 * Name: test_local_peercred_fork
 *
 * Description:
 *   After fork(), the end of a socketpair() in the child still gives the
 *   process that made the pair.
 *
 ****************************************************************************/

void test_local_peercred_fork(FAR void **state)
{
  pid_t me = getpid();
  pid_t pid;
  int status;
  int sv[2];
  int ret;

  ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
  assert_return_code(ret, errno);

  pid = fork();
  if (pid == 0)
    {
      _exit(test_local_peercred_is(sv[1], me) ? 0 : 1);
    }

  ret = pid > 0 ? waitpid(pid, &status, 0) : -1;
  close(sv[0]);
  close(sv[1]);
  assert_true(pid > 0);
  assert_int_equal(ret, pid);
  assert_true(WIFEXITED(status));
  assert_int_equal(WEXITSTATUS(status), 0);
}

/****************************************************************************
 * Name: test_local_peercred_accept
 *
 * Description:
 *   A child connects: the server side gives the child, and the client
 *   gives the server, right after connect() and after accept().
 *
 ****************************************************************************/

void test_local_peercred_accept(FAR void **state)
{
  struct sockaddr_un addr;
  pid_t me = getpid();
  bool server_ok;
  pid_t pid;
  int status;
  int ret;
  int fd;
  int cs;
  int as;
  char b;

  fd = test_local_peercred_bind(&addr);
  ret = listen(fd, 1);
  assert_return_code(ret, errno);

  pid = fork();
  if (pid == 0)
    {
      /* The client checks right after connect() and again after the
       * server wrote (the server has accepted then).
       */

      cs = socket(AF_UNIX, SOCK_STREAM, 0);
      if (cs < 0 || connect(cs, (FAR struct sockaddr *)&addr,
                            sizeof(addr)) < 0)
        {
          _exit(1);
        }

      status = test_local_peercred_is(cs, me) ? 0 : 2;
      read(cs, &b, 1);
      _exit(status | (test_local_peercred_is(cs, me) ? 0 : 4));
    }

  assert_true(pid > 0);
  as = accept(fd, NULL, NULL);
  server_ok = as >= 0 && test_local_peercred_is(as, pid);
  if (as >= 0)
    {
      write(as, "x", 1);
    }

  /* Close only after the client's last check */

  ret = waitpid(pid, &status, 0);
  if (as >= 0)
    {
      close(as);
    }

  close(fd);
  unlink(addr.sun_path);
  assert_true(server_ok);
  assert_int_equal(ret, pid);
  assert_true(WIFEXITED(status));
  assert_int_equal(WEXITSTATUS(status), 0);
}
#endif /* CONFIG_ARCH_HAVE_FORK */
