/****************************************************************************
 * apps/testing/nettest/local/test_local_dgram_common.c
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

#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cmocka.h>

#include "test_local_dgram.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* A case that does not end in this time stops the program */

#define LOCAL_DGRAM_WATCHDOG 10

/****************************************************************************
 * Public Data
 ****************************************************************************/

FAR const char *g_local_dgram_dir = P_tmpdir;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static FAR void *local_dgram_job_main(FAR void *arg)
{
  FAR struct local_dgram_job_s *job = arg;

  usleep(job->ms * 1000);
  errno    = 0;
  job->ret = job->action(job);
  job->err = errno;
  return NULL;
}

static void local_dgram_watchdog(int signo)
{
  static const char msg[] = "cmocka_net_local_dgram: watchdog: "
                            "the test case does not end\n";

  write(STDOUT_FILENO, msg, sizeof(msg) - 1);
  _exit(EXIT_FAILURE);
}

static void local_dgram_close(FAR int *fd)
{
  if (*fd >= 0)
    {
      close(*fd);
      *fd = -1;
    }
}

static void local_dgram_cleanup(FAR struct nettest_local_dgram_state_s *st)
{
  local_dgram_close(&st->r1);
  local_dgram_close(&st->r2);
  local_dgram_close(&st->r3);
  local_dgram_close(&st->s);
  local_dgram_close(&st->u);
  unlink(st->p1);
  unlink(st->p2);
  unlink(st->p3);
  unlink(st->pf);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: local_dgram_addr
 ****************************************************************************/

socklen_t local_dgram_addr(FAR struct sockaddr_un *addr,
                           FAR const char *path, bool abstract)
{
  memset(addr, 0, sizeof(*addr));
  addr->sun_family = AF_UNIX;
  if (abstract)
    {
      strlcpy(addr->sun_path + 1, path, sizeof(addr->sun_path) - 1);
      return sizeof(sa_family_t) + 1 + strlen(path);
    }

  strlcpy(addr->sun_path, path, sizeof(addr->sun_path));
  return sizeof(sa_family_t) + strlen(path) + 1;
}

/****************************************************************************
 * Name: local_dgram_bind
 *
 * Description:
 *   Return a datagram socket bound to path, or -1.
 *
 ****************************************************************************/

int local_dgram_bind(FAR const char *path, bool abstract)
{
  struct sockaddr_un addr;
  socklen_t len = local_dgram_addr(&addr, path, abstract);
  int fd;

  fd = socket(AF_UNIX, SOCK_DGRAM, 0);
  if (fd < 0)
    {
      return -1;
    }

  if (!abstract)
    {
      unlink(path);
    }

  if (bind(fd, (FAR struct sockaddr *)&addr, len) < 0)
    {
      close(fd);
      return -1;
    }

  return fd;
}

/****************************************************************************
 * Name: local_dgram_connect_to
 ****************************************************************************/

int local_dgram_connect_to(int fd, FAR const char *path, bool abstract)
{
  struct sockaddr_un addr;
  socklen_t len = local_dgram_addr(&addr, path, abstract);

  return connect(fd, (FAR struct sockaddr *)&addr, len);
}

/****************************************************************************
 * Name: local_dgram_readable
 *
 * Description:
 *   Return true if poll() reports POLLIN on fd within ms milliseconds.
 *
 ****************************************************************************/

bool local_dgram_readable(int fd, int ms)
{
  struct pollfd pfd;
  struct timespec t0;
  struct timespec t1;
  long left = ms;

  clock_gettime(CLOCK_MONOTONIC, &t0);
  for (; ; )
    {
      pfd.fd      = fd;
      pfd.events  = POLLIN;
      pfd.revents = 0;
      if (poll(&pfd, 1, left) > 0 && (pfd.revents & POLLIN) != 0)
        {
          return true;
        }

      clock_gettime(CLOCK_MONOTONIC, &t1);
      left = ms - ((t1.tv_sec - t0.tv_sec) * 1000 +
                   (t1.tv_nsec - t0.tv_nsec) / 1000000);
      if (left <= 0)
        {
          return false;
        }
    }
}

/****************************************************************************
 * Name: local_dgram_expect
 *
 * Description:
 *   Wait up to 1 s for one datagram on fd and check that it is "want".
 *
 ****************************************************************************/

void local_dgram_expect(int fd, FAR const char *want)
{
  char buf[128];
  ssize_t n;

  assert_true(local_dgram_readable(fd, 1000));
  n = recv(fd, buf, sizeof(buf) - 1, 0);
  assert_return_code(n, errno);
  buf[n] = '\0';
  assert_string_equal(buf, want);
}

/****************************************************************************
 * Name: local_dgram_drain
 ****************************************************************************/

void local_dgram_drain(int fd, FAR char *buf, size_t size)
{
  while (local_dgram_readable(fd, 0))
    {
      if (recv(fd, buf, size, 0) < 0)
        {
          break;
        }
    }
}

/****************************************************************************
 * Name: local_dgram_mkfifo
 *
 * Description:
 *   Make a FIFO.  A FIFO cannot be inside a mounted file system (EPERM), so
 *   fall back to the pseudo file system.  path gets the name that worked.
 *
 ****************************************************************************/

void local_dgram_mkfifo(FAR char *path, size_t size,
                        FAR const char *first)
{
  int ret;

  strlcpy(path, first, size);
  unlink(path);
  ret = mkfifo(path, 0666);
  if (ret < 0 && errno == EPERM)
    {
      snprintf(path, size, "/dev/dgramf-%d", getpid());
      unlink(path);
      ret = mkfifo(path, 0666);
    }

  assert_return_code(ret, errno);
}

/****************************************************************************
 * Name: local_dgram_job_arg
 *
 * Description:
 *   Start another thread that does an action after ms milliseconds.
 *
 ****************************************************************************/

void local_dgram_job_arg(FAR struct local_dgram_job_s *job,
                         local_dgram_action_t action, int fd,
                         FAR const char *path, int ms, int arg)
{
  int ret;

  memset(job, 0, sizeof(*job));
  job->action = action;
  job->fd     = fd;
  job->path   = path;
  job->ms     = ms;
  job->arg    = arg;
  job->msg    = "later";
  job->target = pthread_self();
  ret = pthread_create(&job->thread, NULL, local_dgram_job_main, job);
  assert_int_equal(ret, 0);
}

/****************************************************************************
 * Name: local_dgram_job_start
 ****************************************************************************/

void local_dgram_job_start(FAR struct local_dgram_job_s *job,
                           local_dgram_action_t action, int fd,
                           FAR const char *path, int ms)
{
  local_dgram_job_arg(job, action, fd, path, ms, 0);
}

/****************************************************************************
 * Name: local_dgram_job_join
 ****************************************************************************/

void local_dgram_job_join(FAR struct local_dgram_job_s *job)
{
  pthread_join(job->thread, NULL);
}

/****************************************************************************
 * Name: local_dgram_sendto
 ****************************************************************************/

ssize_t local_dgram_sendto(int fd, FAR const char *path,
                           FAR const void *buf, size_t len, int flags)
{
  struct sockaddr_un addr;
  socklen_t addrlen = local_dgram_addr(&addr, path, false);

  return sendto(fd, buf, len, flags, (FAR struct sockaddr *)&addr, addrlen);
}

/****************************************************************************
 * Name: local_dgram_do_send
 ****************************************************************************/

int local_dgram_do_send(FAR struct local_dgram_job_s *job)
{
  return send(job->fd, job->msg, strlen(job->msg), 0);
}

/****************************************************************************
 * Name: local_dgram_do_sendto
 ****************************************************************************/

int local_dgram_do_sendto(FAR struct local_dgram_job_s *job)
{
  return local_dgram_sendto(job->fd, job->path, job->msg, strlen(job->msg),
                            0);
}

/****************************************************************************
 * Name: local_dgram_do_close
 ****************************************************************************/

int local_dgram_do_close(FAR struct local_dgram_job_s *job)
{
  return close(job->fd);
}

/****************************************************************************
 * Name: local_dgram_revents
 *
 * Description:
 *   Return revents of one poll() and check that the result agrees with it.
 *
 ****************************************************************************/

int local_dgram_revents(int fd, int events, int ms)
{
  struct pollfd pfd;
  int ret;

  pfd.fd      = fd;
  pfd.events  = events;
  pfd.revents = 0;
  ret = poll(&pfd, 1, ms);
  assert_return_code(ret, errno);
  assert_int_equal(ret, pfd.revents != 0 ? 1 : 0);
  return pfd.revents;
}

/****************************************************************************
 * Name: test_local_dgram_group_setup
 ****************************************************************************/

int test_local_dgram_group_setup(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st;
  FAR const char *dir = g_local_dgram_dir;
  int pid = getpid();

  st = zalloc(sizeof(*st));
  assert_non_null(st);

  st->big = malloc(LOCAL_DGRAM_BIGLEN);
  assert_non_null(st->big);
  memset(st->big, 'b', LOCAL_DGRAM_BIGLEN);

  snprintf(st->p1, sizeof(st->p1), "%s/dgram1-%d", dir, pid);
  snprintf(st->p2, sizeof(st->p2), "%s/dgram2-%d", dir, pid);
  snprintf(st->p3, sizeof(st->p3), "%s/dgram3-%d", dir, pid);
  snprintf(st->pf, sizeof(st->pf), "%s/dgramf-%d", dir, pid);
  snprintf(st->abs, sizeof(st->abs), "dgramabs-%d", pid);

  st->r1 = -1;
  st->r2 = -1;
  st->r3 = -1;
  st->s  = -1;
  st->u  = -1;

  signal(SIGALRM, local_dgram_watchdog);
  *state = st;
  return 0;
}

/****************************************************************************
 * Name: test_local_dgram_group_teardown
 ****************************************************************************/

int test_local_dgram_group_teardown(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;

  alarm(0);
  signal(SIGALRM, SIG_DFL);
  local_dgram_cleanup(st);
  free(st->big);
  free(st);
  return 0;
}

/****************************************************************************
 * Name: test_local_dgram_setup
 *
 * Description:
 *   Make r1, a socket bound to p1, and s, a socket that is not connected.
 *
 ****************************************************************************/

int test_local_dgram_setup(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;

  local_dgram_cleanup(st);
  alarm(LOCAL_DGRAM_WATCHDOG);

  st->r1 = local_dgram_bind(st->p1, false);
  assert_return_code(st->r1, errno);

  st->s = socket(AF_UNIX, SOCK_DGRAM, 0);
  assert_return_code(st->s, errno);
  return 0;
}

/****************************************************************************
 * Name: test_local_dgram_connected_setup
 *
 * Description:
 *   As test_local_dgram_setup(), and connect s to p1.
 *
 ****************************************************************************/

int test_local_dgram_connected_setup(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  int ret;

  test_local_dgram_setup(state);
  ret = local_dgram_connect_to(st->s, st->p1, false);
  assert_return_code(ret, errno);
  return 0;
}

/****************************************************************************
 * Name: test_local_dgram_teardown
 ****************************************************************************/

int test_local_dgram_teardown(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;

  alarm(0);
  local_dgram_cleanup(st);
  return 0;
}
