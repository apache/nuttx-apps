/****************************************************************************
 * apps/testing/sched/waitstop/waitstop_main.c
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

/* A test of stopped and continued children: SIGCHLD with CLD_STOPPED and
 * CLD_CONTINUED, SA_NOCLDSTOP, SA_NOCLDWAIT, waitpid() with WUNTRACED and
 * WCONTINUED, waitid() with WSTOPPED, WCONTINUED and WNOWAIT, waitpid() of
 * a process group, and SIGKILL to a stopped child.  The children are this
 * program again, started with posix_spawn(), so the test runs without
 * fork().
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <signal.h>
#include <spawn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NAME          "waitstop"

/* A wait that takes longer fails */

#define WAIT_TIMEOUT  5

/* No process and no process group has this ID */

#define NO_PID        0x7ffffff0

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct waitstop_s
{
  FAR char *self;  /* The path of this program */
  int       fails; /* The number of failed checks */
  int       skips; /* The number of skipped cases */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static volatile sig_atomic_t g_nchld;   /* SIGCHLD handler calls */
static volatile sig_atomic_t g_code;    /* si_code of the last one */
static volatile sig_atomic_t g_status;  /* si_status of the last one */
static volatile sig_atomic_t g_pid;     /* si_pid of the last one */
static volatile sig_atomic_t g_timeout; /* SIGALRM came */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void result(FAR struct waitstop_s *t, FAR const char *name,
                   bool ok, FAR const char *fmt, int a, int b)
{
  printf(NAME ": %-12s ", name);
  printf(fmt, a, b);
  printf(" %s\n", ok ? "PASS" : "FAIL");
  t->fails += !ok;
}

static void msleep(int ms)
{
  struct timespec ts;

  ts.tv_sec  = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000L;
  while (nanosleep(&ts, &ts) < 0 && errno == EINTR)
    {
    }
}

static void chld_handler(int signo, FAR siginfo_t *info, FAR void *ctx)
{
  g_nchld++;
  g_code   = info->si_code;
  g_status = info->si_status;
  g_pid    = info->si_pid;
}

static void alrm_handler(int signo)
{
  g_timeout = 1;
}

/* Catch SIGCHLD with SA_SIGINFO and flags */

static void chld_catch(int flags)
{
  struct sigaction sa;

  memset(&sa, 0, sizeof(sa));
  sa.sa_sigaction = chld_handler;
  sa.sa_flags     = SA_SIGINFO | flags;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGCHLD, &sa, NULL);
  g_nchld  = 0;
  g_code   = 0;
  g_status = 0;
  g_pid    = 0;
}

/* Wait until the SIGCHLD handler ran n times (at most 3 s) */

static bool chld_waitn(int n)
{
  int i;

  for (i = 0; i < 300 && g_nchld < n; i++)
    {
      msleep(10);
    }

  return g_nchld >= n;
}

/* waitpid() and waitid() that go on after a SIGCHLD handler, and fail with
 * ETIMEDOUT after WAIT_TIMEOUT seconds.
 */

static pid_t xwaitpid(pid_t pid, FAR int *status, int options)
{
  pid_t ret;

  g_timeout = 0;
  alarm(WAIT_TIMEOUT);
  do
    {
      ret = waitpid(pid, status, options);
    }
  while (ret < 0 && errno == EINTR && !g_timeout);

  alarm(0);
  if (ret < 0 && g_timeout)
    {
      errno = ETIMEDOUT;
    }

  return ret;
}

static int xwaitid(idtype_t idtype, id_t id, FAR siginfo_t *info,
                   int options)
{
  int ret;

  g_timeout = 0;
  alarm(WAIT_TIMEOUT);
  do
    {
      memset(info, 0, sizeof(*info));
      ret = waitid(idtype, id, info, options);
    }
  while (ret < 0 && errno == EINTR && !g_timeout);

  alarm(0);
  if (ret < 0 && g_timeout)
    {
      errno = ETIMEDOUT;
    }

  return ret;
}

/* A child: "stop code" waits 200 ms (the parent then waits already),
 * stops itself with SIGSTOP, and exits with code 300 ms after SIGCONT.
 * "exit ms code" exits with code after ms.  "pause" waits for a signal
 * that ends it.
 */

static int child_main(FAR const char *cmd, FAR const char *arg1,
                      FAR const char *arg2)
{
  if (strcmp(cmd, "stop") == 0)
    {
      msleep(200);
      raise(SIGSTOP);
      msleep(300);
      return atoi(arg1);
    }
  else if (strcmp(cmd, "exit") == 0)
    {
      msleep(atoi(arg1));
      return atoi(arg2);
    }

  for (; ; )
    {
      pause();
    }

  return 0;
}

/* Start a child with posix_spawn().  With newpgrp, the child leads a new
 * process group (without process groups, each process is one).  Return its
 * process ID, or 0 with errno set.
 */

static pid_t child_start(FAR struct waitstop_s *t, FAR char *cmd,
                         FAR char *arg1, FAR char *arg2, bool newpgrp)
{
  FAR char *argv[6];
  posix_spawnattr_t attr;
  pid_t pid = 0;
  int ret;

  argv[0] = t->self;
  argv[1] = "child";
  argv[2] = cmd;
  argv[3] = arg1;
  argv[4] = arg2;
  argv[5] = NULL;

  posix_spawnattr_init(&attr);
  ret = 0;
#ifdef CONFIG_SCHED_PROCESS_GROUPS
  if (newpgrp)
    {
      ret = posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETPGROUP);
      if (ret == 0)
        {
          ret = posix_spawnattr_setpgroup(&attr, 0);
        }
    }
#endif

  if (ret == 0)
    {
      ret = posix_spawn(&pid, t->self, NULL, &attr, argv, NULL);
    }

  posix_spawnattr_destroy(&attr);
  if (ret != 0)
    {
      errno = ret;
      return 0;
    }

  return pid;
}

/* End a child that a failed check left behind */

static void child_kill(pid_t pid)
{
  int status;

  if (pid > 0 && kill(pid, SIGKILL) == 0)
    {
      kill(pid, SIGCONT);
      xwaitpid(pid, &status, 0);
    }
}

/* Wait until the child pid stopped (WNOWAIT keeps the event) */

static bool wait_stopped_nowait(pid_t pid)
{
  siginfo_t si;

  return xwaitid(P_PID, pid, &si, WSTOPPED | WNOWAIT) == 0 &&
         si.si_code == CLD_STOPPED;
}

/* WUNTRACED, then WNOHANG, then WCONTINUED, then the exit */

static void test_stop(FAR struct waitstop_s *t)
{
  pid_t pid;
  pid_t ret;
  int st = 0;

  pid = child_start(t, "stop", "7", "", false);
  if (pid == 0)
    {
      result(t, "stop-raise", false, "posix_spawn() errno %d%c", errno, ' ');
      return;
    }

  ret = xwaitpid(pid, &st, WUNTRACED);
  result(t, "stop-raise", ret == pid && WIFSTOPPED(st) &&
         WSTOPSIG(st) == SIGSTOP, "waitpid(WUNTRACED) ok %d, status 0x%x",
         ret == pid, st);
  if (ret != pid)
    {
      child_kill(pid);
      return;
    }

  st  = 0;
  ret = xwaitpid(pid, &st, WUNTRACED | WNOHANG);
  result(t, "stop-once", ret == 0, "waitpid(WNOHANG) %d, status 0x%x",
         ret, st);

  kill(pid, SIGCONT);
  st  = 0;
  ret = xwaitpid(pid, &st, WCONTINUED);
  result(t, "cont", ret == pid && WIFCONTINUED(st) && !WIFEXITED(st) &&
         !WIFSTOPPED(st), "waitpid(WCONTINUED) ok %d, status 0x%x",
         ret == pid, st);
  if (ret == pid && WIFEXITED(st))
    {
      return;
    }

  st  = 0;
  ret = xwaitpid(pid, &st, 0);
  result(t, "cont-exit", ret == pid && WIFEXITED(st) &&
         WEXITSTATUS(st) == 7, "waitpid() ok %d, status 0x%x", ret == pid,
         st);
  if (ret != pid)
    {
      child_kill(pid);
    }
}

/* Without WUNTRACED and WCONTINUED, only the exit is reported */

static void test_untraced_no(FAR struct waitstop_s *t)
{
  pid_t pid;
  pid_t r1;
  pid_t r2;
  int st = 0;

  pid = child_start(t, "stop", "7", "", false);
  if (pid == 0 || !wait_stopped_nowait(pid))
    {
      result(t, "untraced-no", false, "no stop seen, errno %d%c", errno,
             ' ');
      child_kill(pid);
      return;
    }

  r1 = xwaitpid(pid, &st, WNOHANG);
  kill(pid, SIGCONT);
  st = 0;
  r2 = xwaitpid(pid, &st, 0);
  result(t, "untraced-no", r1 == 0 && r2 == pid && WIFEXITED(st) &&
         WEXITSTATUS(st) == 7, "waitpid(WNOHANG) %d, status 0x%x", r1, st);
  if (r2 != pid)
    {
      child_kill(pid);
    }
}

/* SIGTSTP from the parent, then SIGKILL ends the stopped child */

static void test_tstp(FAR struct waitstop_s *t)
{
  pid_t pid;
  pid_t ret;
  int st = 0;

  pid = child_start(t, "pause", "", "", false);
  if (pid == 0)
    {
      result(t, "tstp", false, "posix_spawn() errno %d%c", errno, ' ');
      return;
    }

  msleep(200);
  kill(pid, SIGTSTP);
  ret = xwaitpid(pid, &st, WUNTRACED);
  result(t, "tstp", ret == pid && WIFSTOPPED(st) && WSTOPSIG(st) == SIGTSTP,
         "waitpid(WUNTRACED) ok %d, status 0x%x", ret == pid, st);

  kill(pid, SIGKILL);
  st  = 0;
  ret = xwaitpid(pid, &st, 0);
  result(t, "kill-stopped", ret == pid && !WIFSTOPPED(st) &&
         !WIFCONTINUED(st), "waitpid() ok %d, status 0x%x", ret == pid, st);
  if (ret != pid)
    {
      child_kill(pid);
    }
}

/* SA_SIGINFO handler: CLD_STOPPED with SIGSTOP, then CLD_CONTINUED */

static void test_sigchld_info(FAR struct waitstop_s *t)
{
  pid_t pid;
  int st;

  chld_catch(0);
  pid = child_start(t, "stop", "7", "", false);
  if (pid == 0)
    {
      result(t, "sigchld-info", false, "posix_spawn() errno %d%c", errno,
             ' ');
      signal(SIGCHLD, SIG_DFL);
      return;
    }

  chld_waitn(1);
  result(t, "sigchld-info", g_nchld >= 1 &&
         g_code == CLD_STOPPED && g_status == SIGSTOP && g_pid == pid,
         "stop: code %d, status %d", g_code, g_status);

  kill(pid, SIGCONT);
  chld_waitn(2);

  result(t, "sigchld-info", g_nchld == 2 && g_code == CLD_CONTINUED &&
         g_status == SIGCONT && g_pid == pid, "cont: calls %d, code %d",
         g_nchld, g_code);
  if (xwaitpid(pid, &st, 0) != pid)
    {
      child_kill(pid);
    }

  signal(SIGCHLD, SIG_DFL);
}

/* SA_NOCLDSTOP: no handler call for the stop and the continue, but the
 * wait still reports them.  The exit calls the handler.
 */

static void test_nocldstop(FAR struct waitstop_s *t)
{
  pid_t pid;
  pid_t ret;
  int st = 0;

  chld_catch(SA_NOCLDSTOP);
  pid = child_start(t, "stop", "7", "", false);
  if (pid == 0)
    {
      result(t, "nocldstop", false, "posix_spawn() errno %d%c", errno, ' ');
      signal(SIGCHLD, SIG_DFL);
      return;
    }

  ret = xwaitpid(pid, &st, WUNTRACED);
  msleep(200);
  result(t, "nocldstop", ret == pid && WIFSTOPPED(st) &&
         g_nchld == 0, "stop: handler calls %d, status 0x%x", g_nchld, st);

  kill(pid, SIGCONT);
  st  = 0;
  ret = xwaitpid(pid, &st, WCONTINUED);
  result(t, "nocldstop", ret == pid && WIFCONTINUED(st) && g_nchld == 0,
         "cont: handler calls %d, status 0x%x", g_nchld, st);

  st  = 0;
  ret = xwaitpid(pid, &st, 0);
  chld_waitn(1);
  result(t, "nocldstop", ret == pid && WIFEXITED(st) && g_nchld == 1 &&
         g_code == CLD_EXITED, "exit: handler calls %d, code %d", g_nchld,
         g_code);
  if (ret != pid)
    {
      child_kill(pid);
    }

  signal(SIGCHLD, SIG_DFL);
}

/* SA_NOCLDWAIT: the stop is still reported, the exit is not (XSI) */

static void test_nocldwait(FAR struct waitstop_s *t)
{
  struct sigaction sa;
  pid_t pid;
  pid_t ret;
  int st = 0;

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = SIG_DFL;
  sa.sa_flags   = SA_NOCLDWAIT;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGCHLD, &sa, NULL);

  pid = child_start(t, "stop", "7", "", false);
  if (pid == 0)
    {
      result(t, "nocldwait", false, "posix_spawn() errno %d%c", errno, ' ');
      signal(SIGCHLD, SIG_DFL);
      return;
    }

  ret = xwaitpid(pid, &st, WUNTRACED);
  result(t, "nocldwait", ret == pid && WIFSTOPPED(st),
         "waitpid(WUNTRACED) ok %d, status 0x%x", ret == pid, st);

  kill(pid, SIGCONT);
  ret = xwaitpid(pid, &st, 0);
  result(t, "nocldwait", ret < 0 && errno == ECHILD,
         "exit: ret %d, errno %d", ret, errno);
  if (ret == 0 || (ret < 0 && errno == ETIMEDOUT))
    {
      child_kill(pid);
    }

  signal(SIGCHLD, SIG_DFL);
}

/* waitid(): WNOWAIT keeps the stop, WSTOPPED takes it, then WCONTINUED */

static void test_waitid(FAR struct waitstop_s *t)
{
  siginfo_t si;
  pid_t pid;
  int r1;
  int r2;
  int c1;
  int c2;

  pid = child_start(t, "stop", "7", "", false);
  if (pid == 0)
    {
      result(t, "waitid", false, "posix_spawn() errno %d%c", errno, ' ');
      return;
    }

  r1  = xwaitid(P_PID, pid, &si, WSTOPPED | WNOWAIT);
  c1  = si.si_code;
  r2  = xwaitid(P_PID, pid, &si, WSTOPPED | WNOWAIT);
  c2  = si.si_code;
  result(t, "waitid", r1 == 0 && r2 == 0 &&
         c1 == CLD_STOPPED && c2 == CLD_STOPPED, "WNOWAIT: codes %d %d",
         c1, c2);

  r1 = xwaitid(P_PID, pid, &si, WSTOPPED);
  result(t, "waitid", r1 == 0 && si.si_code == CLD_STOPPED &&
         si.si_status == SIGSTOP && si.si_pid == pid,
         "WSTOPPED: code %d, status %d", si.si_code, si.si_status);

  r1 = xwaitid(P_PID, pid, &si, WSTOPPED | WNOHANG);
  result(t, "waitid", r1 == 0 && si.si_pid == 0,
         "WNOHANG: ret %d, pid %d", r1, si.si_pid);

  kill(pid, SIGCONT);
  r1 = xwaitid(P_PID, pid, &si, WCONTINUED);
  result(t, "waitid", r1 == 0 && si.si_code == CLD_CONTINUED &&
         si.si_status == SIGCONT, "WCONTINUED: code %d, status %d",
         si.si_code, si.si_status);

  r1 = xwaitid(P_PID, pid, &si, WEXITED);
  result(t, "waitid", r1 == 0 && si.si_code == CLD_EXITED &&
         si.si_status == 7, "WEXITED: code %d, status %d", si.si_code,
         si.si_status);
  if (r1 != 0)
    {
      child_kill(pid);
    }

  r1 = waitid(P_PID, pid, &si, WNOHANG);
  r2 = errno;
  result(t, "waitid", r1 < 0 && r2 == EINVAL,
         "no WEXITED, WSTOPPED or WCONTINUED: ret %d, errno %d", r1, r2);
}

/* waitpid(-pgid) and waitid(P_PGID) select the process group of a child.
 * Without process groups, a process group is one process.
 */

static void test_wait_pgid(FAR struct waitstop_s *t)
{
  siginfo_t si;
  pid_t a;
  pid_t ret;
  int st = 0;

  a = child_start(t, "exit", "300", "17", true);
  if (a == 0)
    {
      result(t, "wait-pgid", false, "posix_spawn() errno %d%c", errno, ' ');
      return;
    }

  ret = xwaitpid(-a, &st, WNOHANG);
  result(t, "wait-pgid", ret == 0, "waitpid(-A, WNOHANG) %d%c", ret, ' ');

  ret = xwaitid(P_PGID, a, &si, WEXITED | WNOWAIT);
  result(t, "wait-pgid", ret == 0 && si.si_pid == a &&
         si.si_code == CLD_EXITED, "waitid(P_PGID, WNOWAIT) %d, is A %d",
         ret, si.si_pid == a);

  st  = 0;
  ret = xwaitpid(-a, &st, 0);
  result(t, "wait-pgid", ret == a && WIFEXITED(st) &&
         WEXITSTATUS(st) == 17, "waitpid(-A) is A %d, status 0x%x",
         ret == a, st);
  if (ret != a)
    {
      child_kill(a);
    }

  ret = xwaitpid(-a, &st, 0);
  result(t, "wait-pgid", ret < 0 && errno == ECHILD,
         "no child left: ret %d, errno %d", ret, errno);

  ret = xwaitpid(-NO_PID, &st, WNOHANG);
  result(t, "wait-pgid", ret < 0 && errno == ECHILD,
         "no such group: ret %d, errno %d", ret, errno);
}

/* waitpid(0) selects the children in the process group of the caller,
 * also after their exit.
 */

static void test_wait_pgid0(FAR struct waitstop_s *t)
{
#ifdef CONFIG_SCHED_PROCESS_GROUPS
  pid_t a;
  pid_t b;
  pid_t ret;
  int st = 0;

  /* A is in its own process group and exits first, B is in ours */

  a = child_start(t, "exit", "100", "17", true);
  b = child_start(t, "exit", "600", "34", false);
  if (a == 0 || b == 0)
    {
      result(t, "wait-pgid0", false, "posix_spawn() errno %d%c", errno,
             ' ');
      child_kill(a);
      child_kill(b);
      return;
    }

  ret = xwaitpid(0, &st, 0);
  result(t, "wait-pgid0", ret == b && WIFEXITED(st) &&
         WEXITSTATUS(st) == 34, "waitpid(0) is B %d, status 0x%x",
         ret == b, st);
  if (ret != b)
    {
      child_kill(b);
    }

  /* A exited before: its child status keeps its process group */

  st  = 0;
  ret = xwaitpid(-a, &st, WNOHANG);
  result(t, "wait-pgid0", ret == a && WIFEXITED(st) &&
         WEXITSTATUS(st) == 17, "exited A: waitpid(-A) is A %d, "
         "status 0x%x", ret == a, st);
  if (ret != a)
    {
      child_kill(a);
    }
#else
  printf(NAME ": %-12s no CONFIG_SCHED_PROCESS_GROUPS SKIP\n", "wait-pgid0");
  t->skips++;
#endif
}

/* The stop and the continue of a child in another process group */

static void test_stop_pgid(FAR struct waitstop_s *t)
{
  pid_t a;
  pid_t ret;
  int st = 0;

  a = child_start(t, "stop", "9", "", true);
  if (a == 0)
    {
      result(t, "stop-pgid", false, "posix_spawn() errno %d%c", errno, ' ');
      return;
    }

  ret = xwaitpid(-a, &st, WUNTRACED);
  result(t, "stop-pgid", ret == a && WIFSTOPPED(st) &&
         WSTOPSIG(st) == SIGSTOP, "waitpid(-A, WUNTRACED) is A %d, "
         "status 0x%x", ret == a, st);

  /* waitpid(0) does not see it: another process group */

  ret = xwaitpid(0, &st, WUNTRACED | WNOHANG);
  result(t, "stop-pgid", ret < 0 && errno == ECHILD,
         "waitpid(0): ret %d, errno %d", ret, errno);

  kill(-a, SIGCONT);
  st  = 0;
  ret = xwaitpid(-a, &st, WCONTINUED);
  result(t, "stop-pgid", ret == a && WIFCONTINUED(st),
         "waitpid(-A, WCONTINUED) is A %d, status 0x%x", ret == a, st);

  st  = 0;
  ret = xwaitpid(-a, &st, 0);
  result(t, "stop-pgid", ret == a && WIFEXITED(st) && WEXITSTATUS(st) == 9,
         "waitpid(-A) is A %d, status 0x%x", ret == a, st);
  if (ret != a)
    {
      child_kill(a);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct waitstop_s t;
  struct sigaction sa;

  if (argc >= 3 && strcmp(argv[1], "child") == 0)
    {
      return child_main(argv[2], argc > 3 ? argv[3] : "",
                        argc > 4 ? argv[4] : "");
    }

  t.self  = argv[0];
  t.fails = 0;
  t.skips = 0;

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = alrm_handler;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGALRM, &sa, NULL);

  test_stop(&t);
  test_untraced_no(&t);
  test_tstp(&t);
  test_sigchld_info(&t);
  test_nocldstop(&t);
  test_nocldwait(&t);
  test_waitid(&t);
  test_wait_pgid(&t);
  test_wait_pgid0(&t);
  test_stop_pgid(&t);

  signal(SIGALRM, SIG_DFL);
  printf(NAME ": done, %d failed, %d skipped\n", t.fails, t.skips);
  return t.fails != 0;
}
