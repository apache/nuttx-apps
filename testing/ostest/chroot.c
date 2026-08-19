/****************************************************************************
 * apps/testing/ostest/chroot.c
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
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>

#include <sys/stat.h>
#include <sys/wait.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ostest.h"

#if defined(CONFIG_FS_CHROOT) && defined(CONFIG_SCHED_WAITPID) && \
    !defined(CONFIG_BUILD_KERNEL)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_LIBC_TMPDIR
#  define CHROOT_TMPDIR CONFIG_LIBC_TMPDIR
#else
#  define CHROOT_TMPDIR "/tmp"
#endif

#define CHROOT_JAIL    CHROOT_TMPDIR "/ostest_jail"
#define CHROOT_MARK    CHROOT_JAIL "/marker"
#define CHROOT_SECRET  CHROOT_TMPDIR "/ostest_secret"
#define CHROOT_PRIO    100

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int chroot_fail(FAR const char *msg)
{
  printf("chroot_test: ERROR %s errno=%d\n", msg, errno);
  ASSERT(false);
  return EXIT_FAILURE;
}

static int chroot_grandchild(int argc, FAR char *argv[])
{
  struct stat st;

  UNUSED(argc);
  UNUSED(argv);

  if (stat("/marker", &st) < 0)
    {
      return chroot_fail("grandchild stat(/marker) failed");
    }

  printf("chroot_test: grandchild still sees the jail\n");
  return EXIT_SUCCESS;
}

static int chroot_child(int argc, FAR char *argv[])
{
  struct stat st;
  pid_t pid;
  int status;
  int fd;
  int hostfd;

  UNUSED(argc);
  UNUSED(argv);

  hostfd = open(CHROOT_SECRET, O_RDONLY);
  if (hostfd < 0)
    {
      return chroot_fail("open(host secret) failed");
    }

  if (chdir(CHROOT_JAIL) < 0)
    {
      return chroot_fail("chdir(jail) failed");
    }

  if (chroot(".") < 0)
    {
      return chroot_fail("chroot(.) failed");
    }

  if (chdir("/") < 0)
    {
      return chroot_fail("chdir(/) failed");
    }

  if (stat("/marker", &st) < 0)
    {
      return chroot_fail("stat(/marker) failed");
    }

  printf("chroot_test: /marker is visible inside the jail\n");

  if (stat("/dev", &st) == 0)
    {
      return chroot_fail("stat(/dev) succeeded inside jail");
    }

  if (errno != ENOENT)
    {
      return chroot_fail("stat(/dev) expected ENOENT");
    }

  fd = open("/../dev", O_RDONLY);
  if (fd >= 0)
    {
      close(fd);
      errno = 0;
      return chroot_fail("open(/../dev) escaped the jail");
    }

#ifdef CONFIG_DEV_NULL
  fd = open("/../dev/null", O_RDWR);
  if (fd >= 0)
    {
      close(fd);
      errno = 0;
      return chroot_fail("open(/../dev/null) escaped the jail");
    }
#endif

  printf("chroot_test: host paths are not visible inside the jail\n");

  /* chroot() does not close pre-opened host fds (POSIX). */

  if (fcntl(hostfd, F_GETFD) < 0)
    {
      return chroot_fail("host fd closed by chroot()");
    }

  printf("chroot_test: pre-opened host fd still usable after chroot\n");
  close(hostfd);

  pid = task_create("chroot_gc", CHROOT_PRIO, STACKSIZE,
                    chroot_grandchild, NULL);
  if (pid < 0)
    {
      return chroot_fail("task_create(grandchild) failed");
    }

  if (waitpid(pid, &status, 0) != pid)
    {
      return chroot_fail("waitpid(grandchild) failed");
    }

  if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS)
    {
      printf("chroot_test: ERROR grandchild status=%d\n", status);
      ASSERT(false);
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}

static int chroot_prepare(void)
{
  int fd;
  ssize_t nwritten;
  static const char marker[] = "ostest-chroot\n";

  mkdir(CHROOT_TMPDIR, 0777);
  if (mkdir(CHROOT_JAIL, 0777) < 0 && errno != EEXIST)
    {
      printf("chroot_test: ERROR mkdir(%s) failed errno=%d\n",
             CHROOT_JAIL, errno);
      ASSERT(false);
      return ERROR;
    }

  fd = open(CHROOT_MARK, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0)
    {
      printf("chroot_test: ERROR open(%s) failed errno=%d\n",
             CHROOT_MARK, errno);
      ASSERT(false);
      return ERROR;
    }

  nwritten = write(fd, marker, sizeof(marker) - 1);
  close(fd);
  if (nwritten != (ssize_t)(sizeof(marker) - 1))
    {
      printf("chroot_test: ERROR write(marker) failed errno=%d\n", errno);
      ASSERT(false);
      return ERROR;
    }

  fd = open(CHROOT_SECRET, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0)
    {
      printf("chroot_test: ERROR open(%s) failed errno=%d\n",
             CHROOT_SECRET, errno);
      ASSERT(false);
      return ERROR;
    }

  close(fd);
  return OK;
}

static void chroot_cleanup(void)
{
  unlink(CHROOT_MARK);
  unlink(CHROOT_SECRET);
  rmdir(CHROOT_JAIL);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int chroot_test(void)
{
  pid_t pid;
  int status;
#ifdef CONFIG_DEV_NULL
  int fd;
#endif

  printf("chroot_test: Starting test\n");

  if (chroot_prepare() < 0)
    {
      return ERROR;
    }

  pid = task_create("chroot_child", CHROOT_PRIO, STACKSIZE,
                    chroot_child, NULL);
  if (pid < 0)
    {
      printf("chroot_test: ERROR task_create failed errno=%d\n", errno);
      ASSERT(false);
      chroot_cleanup();
      return ERROR;
    }

  if (waitpid(pid, &status, 0) != pid)
    {
      printf("chroot_test: ERROR waitpid failed errno=%d\n", errno);
      ASSERT(false);
      chroot_cleanup();
      return ERROR;
    }

  chroot_cleanup();

  if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS)
    {
      printf("chroot_test: ERROR child status=%d\n", status);
      ASSERT(false);
      return ERROR;
    }

#ifdef CONFIG_DEV_NULL
  fd = open("/dev/null", O_RDWR);
  if (fd < 0)
    {
      printf("chroot_test: ERROR parent lost /dev/null errno=%d\n",
             errno);
      ASSERT(false);
      return ERROR;
    }

  close(fd);
#endif

  printf("chroot_test: PASSED\n");
  return OK;
}

#endif /* CONFIG_FS_CHROOT && CONFIG_SCHED_WAITPID && !CONFIG_BUILD_KERNEL */
