/****************************************************************************
 * apps/testing/sched/pgrptest/pgrptest_main.c
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

/* A test of process groups and sessions: setpgid(), getpgid(), setsid(),
 * getsid(), kill() and killpg() to a process group, POSIX_SPAWN_SETPGROUP
 * and POSIX_SPAWN_SETSID, and the INTR character of a terminal.  The
 * children are this program again, started with posix_spawn(), so most
 * cases run without fork().  The fork() cases print SKIP without it.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NAME          "pgrptest"

/* A child reads its pipe on CHILD_RFD and writes its answers to
 * CHILD_WFD.
 */

#define CHILD_RFD     20
#define CHILD_WFD     21

/* The exit code of a child that a signal ended (its handler exits) */

#define SIGEXIT(s)    (100 + (s))

/* No process and no process group has this ID */

#define NO_PID        0x7ffffff0

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct pgrptest_s
{
  FAR char *self;  /* The path of this program */
  int       fails; /* The number of failed cases */
  int       skips; /* The number of skipped cases */
};

struct child_s
{
  pid_t pid;   /* The process ID, or 0 */
  int   wfd;   /* The write end of the pipe that the child reads */
  int   rfd;   /* The read end of the pipe of the answers */
  int   v[4];  /* The answers of the child */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* The signal handler of kill-self counts here */

static volatile sig_atomic_t g_usr1;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void result(FAR struct pgrptest_s *t, FAR const char *name,
                   bool ok, FAR const char *fmt, int a, int b)
{
  printf(NAME ": %-11s ", name);
  printf(fmt, a, b);
  printf(" %s\n", ok ? "PASS" : "FAIL");
  t->fails += !ok;
}

static void skip(FAR struct pgrptest_s *t, FAR const char *name,
                 FAR const char *why)
{
  printf(NAME ": %-11s %s SKIP\n", name, why);
  t->skips++;
}

static void child_signal(int signo)
{
  _exit(SIGEXIT(signo));
}

static void parent_usr1(int signo)
{
  g_usr1++;
}

/* A child: set the signal handlers, do the request cmd, write the
 * answers (the parent waits for them), then wait until the parent closes
 * the pipe (or a signal ends the child).
 */

static int child_main(FAR const char *cmd, FAR const char *arg)
{
  int v[4];
  char c;

  signal(SIGTERM, child_signal);
  signal(SIGINT, child_signal);
  signal(SIGUSR1, child_signal);

  memset(v, 0, sizeof(v));
  if (strcmp(cmd, "join") == 0)
    {
      /* setpgid(0, pgid) in the child, as a shell does after fork() */

      errno = 0;
      v[0] = setpgid(0, atoi(arg));
      v[1] = errno;
      v[2] = getpgrp();
    }
  else if (strcmp(cmd, "setsid") == 0)
    {
      v[0] = setsid();
      v[1] = getsid(0) == getpid() && getpgrp() == getpid();
      v[2] = setsid() < 0 ? errno : 0;
      v[3] = setpgid(0, 0) < 0 ? errno : 0;
    }

  write(CHILD_WFD, v, sizeof(v));
  while (read(CHILD_RFD, &c, 1) < 0 && errno == EINTR)
    {
    }

  return 0;
}

/* Start a child with posix_spawn() and wait for its answers: then its
 * signal handlers are set.  flags and pgroup go to the spawn attributes.
 * Return 0 or an error number.
 */

static int child_start(FAR struct pgrptest_s *t, FAR struct child_s *child,
                       FAR char *cmd, FAR char *arg, short flags,
                       pid_t pgroup)
{
  FAR char *argv[5];
  posix_spawn_file_actions_t actions;
  posix_spawnattr_t attr;
  int in[2];
  int out[2];
  int ret;

  child->pid = 0;
  if (pipe2(in, O_CLOEXEC) < 0)
    {
      return errno;
    }

  if (pipe2(out, O_CLOEXEC) < 0)
    {
      ret = errno;
      close(in[0]);
      close(in[1]);
      return ret;
    }

  argv[0] = t->self;
  argv[1] = "child";
  argv[2] = cmd;
  argv[3] = arg;
  argv[4] = NULL;

  posix_spawn_file_actions_init(&actions);
  posix_spawn_file_actions_adddup2(&actions, in[0], CHILD_RFD);
  posix_spawn_file_actions_adddup2(&actions, out[1], CHILD_WFD);
  posix_spawnattr_init(&attr);
  ret = posix_spawnattr_setflags(&attr, flags);
  if (ret == 0 && (flags & POSIX_SPAWN_SETPGROUP) != 0)
    {
      ret = posix_spawnattr_setpgroup(&attr, pgroup);
    }

  if (ret == 0)
    {
      ret = posix_spawn(&child->pid, t->self, &actions, &attr, argv, NULL);
    }

  posix_spawnattr_destroy(&attr);
  posix_spawn_file_actions_destroy(&actions);
  close(in[0]);
  close(out[1]);
  if (ret != 0)
    {
      child->pid = 0;
      close(in[1]);
      close(out[0]);
      return ret;
    }

  child->wfd = in[1];
  child->rfd = out[0];
  if (read(child->rfd, child->v, sizeof(child->v)) != sizeof(child->v))
    {
      memset(child->v, 0, sizeof(child->v));
      return EIO;
    }

  return 0;
}

/* Wait at most 3 s for a child that a signal should end.  Return its exit
 * status, or -1 if it does not end: then close its pipe, so that it ends.
 */

static int child_reap(FAR struct child_s *child)
{
  int status = -1;
  int i;

  if (child->pid <= 0)
    {
      return -1;
    }

  for (i = 0; i < 30; i++)
    {
      if (waitpid(child->pid, &status, WNOHANG) == child->pid)
        {
          break;
        }

      status = -1;
      usleep(100 * 1000);
    }

  close(child->wfd);
  close(child->rfd);
  if (status == -1)
    {
      waitpid(child->pid, NULL, 0);
    }

  child->pid = 0;
  return status;
}

/* Close the pipe of a child: it ends with the exit code 0 */

static void child_stop(FAR struct child_s *child)
{
  if (child->pid > 0)
    {
      close(child->wfd);
      waitpid(child->pid, NULL, 0);
      close(child->rfd);
      child->pid = 0;
    }
}

static bool ended_by(int status, int signo)
{
  return status != -1 &&
         ((WIFSIGNALED(status) && WTERMSIG(status) == signo) ||
          (WIFEXITED(status) && WEXITSTATUS(status) == SIGEXIT(signo)));
}

/* setpgid(), getpgid(), setsid() and getsid() of the caller */

static void test_self(FAR struct pgrptest_s *t)
{
  int ret;

  errno = 0;
  ret = setpgid(0, 0);
  result(t, "self", ret == 0 && getpgrp() == getpid(),
         "setpgid(0, 0) %d, getpgrp() == getpid() %d", ret,
         getpgrp() == getpid());

  result(t, "getpgid", getpgid(0) == getpgrp() &&
         getpgid(getpid()) == getpgrp(),
         "getpgid(0) %d, getpgid(getpid()) %d", getpgid(0) == getpgrp(),
         getpgid(getpid()) == getpgrp());

  result(t, "getsid", getsid(0) > 0 && getsid(getpid()) == getsid(0) &&
         getsid(0) != getpid(), "getsid(0) %d, not a leader %d",
         getsid(0), getsid(0) != getpid());

  errno = 0;
  ret = setsid();
  result(t, "setsid", ret < 0 && errno == EPERM,
         "setsid() of a group leader %d, errno %d", ret, errno);

  errno = 0;
  ret = setpgid(0, -1);
  result(t, "einval", ret < 0 && errno == EINVAL,
         "setpgid(0, -1) %d, errno %d", ret, errno);

  errno = 0;
  ret = getpgid(-1);
  result(t, "einval", ret < 0 && errno == EINVAL,
         "getpgid(-1) %d, errno %d", ret, errno);

  errno = 0;
  ret = setpgid(NO_PID, 0);
  result(t, "esrch", ret < 0 && errno == ESRCH,
         "setpgid(no process) %d, errno %d", ret, errno);

  errno = 0;
  ret = setpgid(getppid(), 0);
  result(t, "esrch", ret < 0 && errno == ESRCH,
         "setpgid(parent) %d, errno %d", ret, errno);

  errno = 0;
  ret = getpgid(NO_PID);
  result(t, "esrch", ret < 0 && errno == ESRCH,
         "getpgid(no process) %d, errno %d", ret, errno);

  errno = 0;
  ret = getsid(NO_PID);
  result(t, "esrch", ret < 0 && errno == ESRCH,
         "getsid(no process) %d, errno %d", ret, errno);

  errno = 0;
  ret = setpgid(0, NO_PID);
  result(t, "eperm", ret < 0 && errno == EPERM,
         "setpgid(0, no group) %d, errno %d", ret, errno);
}

/* POSIX_SPAWN_SETPGROUP, setpgid() on a child, kill() and killpg() to a
 * process group
 */

static void test_spawn_pgroup(FAR struct pgrptest_s *t)
{
  posix_spawnattr_t attr;
  struct child_s a;
  struct child_s b;
  struct child_s c;
  char arg[16];
  pid_t got = 0;
  pid_t pgid;
  int ret;
  int sa;
  int sb;

  /* A leads a new process group, B joins it */

  ret = child_start(t, &a, "wait", "", POSIX_SPAWN_SETPGROUP, 0);
  result(t, "setpgroup", ret == 0 && getpgid(a.pid) == a.pid,
         "posix_spawn() %d, getpgid(A) == A %d", ret,
         ret == 0 && getpgid(a.pid) == a.pid);
  if (ret != 0)
    {
      return;
    }

  pgid = a.pid;
  posix_spawnattr_init(&attr);
  ret = posix_spawnattr_setpgroup(&attr, a.pid);
  if (ret == 0)
    {
      ret = posix_spawnattr_getpgroup(&attr, &got);
    }

  posix_spawnattr_destroy(&attr);
  result(t, "getpgroup", ret == 0 && got == a.pid,
         "posix_spawnattr_getpgroup() %d, pgroup == A %d", ret,
         got == a.pid);

  ret = child_start(t, &b, "wait", "", POSIX_SPAWN_SETPGROUP, a.pid);
  result(t, "setpgroup", ret == 0 && getpgid(b.pid) == a.pid,
         "posix_spawn() %d, getpgid(B) == A %d", ret,
         ret == 0 && getpgid(b.pid) == a.pid);
  result(t, "setpgroup", getsid(a.pid) == getsid(0) &&
         getsid(b.pid) == getsid(0),
         "getsid(A) == getsid(0) %d, getsid(B) %d",
         getsid(a.pid) == getsid(0), getsid(b.pid) == getsid(0));

  /* posix_spawn() is an exec(): the parent cannot move the child now */

  errno = 0;
  ret = setpgid(a.pid, getpgrp());
  result(t, "eacces", ret < 0 && errno == EACCES,
         "setpgid(A) after posix_spawn() %d, errno %d", ret, errno);

  /* C joins the process group of A itself */

  snprintf(arg, sizeof(arg), "%d", a.pid);
  ret = child_start(t, &c, "join", arg, 0, 0);
  if (ret == 0)
    {
      result(t, "join", c.v[0] == 0 && c.v[2] == a.pid,
             "child setpgid(0, A) errno %d, getpgrp() == A %d", c.v[1],
             c.v[2] == a.pid);
    }
  else
    {
      result(t, "join", false, "posix_spawn() %d%c", ret, ' ');
    }

  errno = 0;
  ret = kill(-a.pid, 0);
  result(t, "kill-0", ret == 0, "kill(-A, 0) %d, errno %d", ret, errno);

  /* kill(-A) ends A, B and C, and not the caller */

  errno = 0;
  ret = kill(-a.pid, SIGTERM);
  result(t, "kill-grp", ret == 0, "kill(-A, SIGTERM) %d, errno %d", ret,
         errno);
  sa  = child_reap(&a);
  sb  = child_reap(&b);
  ret = child_reap(&c);
  result(t, "kill-grp", ended_by(sa, SIGTERM) && ended_by(sb, SIGTERM),
         "waitpid(): A ended %d, B ended %d", ended_by(sa, SIGTERM),
         ended_by(sb, SIGTERM));
  result(t, "kill-grp", ended_by(ret, SIGTERM), "waitpid(): C ended %d%c",
         ended_by(ret, SIGTERM), ' ');

  errno = 0;
  ret = kill(-pgid, 0);
  result(t, "kill-esrch", ret < 0 && errno == ESRCH,
         "kill(-A, 0) after the end %d, errno %d", ret, errno);

  /* killpg() */

  ret = child_start(t, &a, "wait", "", POSIX_SPAWN_SETPGROUP, 0);
  if (ret == 0)
    {
      ret = child_start(t, &b, "wait", "", POSIX_SPAWN_SETPGROUP, a.pid);
    }

  if (ret != 0)
    {
      result(t, "killpg", false, "posix_spawn() %d%c", ret, ' ');
      child_stop(&a);
      return;
    }

  errno = 0;
  ret = killpg(a.pid, SIGTERM);
  sa  = child_reap(&a);
  sb  = child_reap(&b);
  result(t, "killpg", ret == 0 && ended_by(sa, SIGTERM) &&
         ended_by(sb, SIGTERM), "killpg(A, SIGTERM) errno %d, B ended %d",
         errno, ended_by(sb, SIGTERM));

  /* A process group that the session does not have */

  ret = child_start(t, &a, "wait", "", POSIX_SPAWN_SETPGROUP, NO_PID);
  result(t, "spawn-eperm", ret == EPERM,
         "POSIX_SPAWN_SETPGROUP, no group: posix_spawn() %d (want %d)",
         ret, EPERM);
  child_stop(&a);
}

/* kill(0) reaches each process of the process group of the caller */

static void test_kill_self(FAR struct pgrptest_s *t)
{
  struct sigaction act;
  struct child_s c;
  int ret;
  int st;

  /* Without a process group of its own, kill(0) could reach the shell */

  if (getpgrp() != getpid())
    {
      skip(t, "kill-self", "the caller leads no process group");
      return;
    }

  memset(&act, 0, sizeof(act));
  act.sa_handler = parent_usr1;
  sigaction(SIGUSR1, &act, NULL);

  ret = child_start(t, &c, "wait", "", POSIX_SPAWN_SETPGROUP, getpgrp());
  if (ret != 0)
    {
      result(t, "kill-self", false, "posix_spawn() %d%c", ret, ' ');
      signal(SIGUSR1, SIG_DFL);
      return;
    }

  g_usr1 = 0;
  errno = 0;
  ret = kill(0, SIGUSR1);
  st = child_reap(&c);
  result(t, "kill-self", ret == 0 && g_usr1 == 1,
         "kill(0, SIGUSR1) %d, handler %d", ret, g_usr1);
  result(t, "kill-self", ended_by(st, SIGUSR1), "child ended %d%c",
         ended_by(st, SIGUSR1), ' ');

  signal(SIGUSR1, SIG_DFL);
}

/* POSIX_SPAWN_SETSID and setsid() in a child */

static void test_spawn_setsid(FAR struct pgrptest_s *t)
{
  struct child_s s;
  int ret;

  ret = child_start(t, &s, "wait", "", POSIX_SPAWN_SETSID, 0);
  result(t, "setsid-sp", ret == 0 && getsid(s.pid) == s.pid &&
         getpgid(s.pid) == s.pid, "posix_spawn() %d, new session %d",
         ret, ret == 0 && getsid(s.pid) == s.pid &&
         getpgid(s.pid) == s.pid);

  /* The child is in another session */

  errno = 0;
  ret = setpgid(s.pid, getpgrp());
  result(t, "eperm-ses", ret < 0 && errno == EPERM,
         "setpgid(child in another session) %d, errno %d", ret, errno);
  child_stop(&s);

  /* A session leader cannot join a process group */

  ret = child_start(t, &s, "wait", "",
                    POSIX_SPAWN_SETSID | POSIX_SPAWN_SETPGROUP, getpgrp());
  result(t, "setsid-pg", ret == EPERM,
         "SETSID | SETPGROUP: posix_spawn() %d (want %d)", ret, EPERM);
  child_stop(&s);

  /* setsid() in a child that leads no process group */

  ret = child_start(t, &s, "setsid", "", 0, 0);
  if (ret == 0)
    {
      result(t, "setsid-ch", s.v[0] == s.pid && s.v[1],
             "child setsid() == pid %d, getsid(0), getpgrp() %d",
             s.v[0] == s.pid, s.v[1]);
      result(t, "setsid-ch", s.v[2] == EPERM && s.v[3] == EPERM,
             "second setsid() errno %d, leader setpgid(0, 0) errno %d",
             s.v[2], s.v[3]);
    }
  else
    {
      result(t, "setsid-ch", false, "posix_spawn() %d%c", ret, ' ');
    }

  child_stop(&s);
}

/* The INTR character of a pseudo-terminal goes to each process of its
 * foreground process group.
 */

static void test_terminal(FAR struct pgrptest_s *t)
{
#if defined(CONFIG_PSEUDOTERM_SUSV1) && defined(CONFIG_TTY_SIGINT)
  struct child_s a;
  struct child_s b;
  FAR char *name = NULL;
  int master;
  int slave;
  int ret;
  int sa;
  int sb;

  master = posix_openpt(O_RDWR | O_NOCTTY | O_CLOEXEC);
  if (master >= 0 && grantpt(master) == 0 && unlockpt(master) == 0)
    {
      name = ptsname(master);
    }

  slave = name != NULL ? open(name, O_RDWR | O_NOCTTY | O_CLOEXEC) : -1;
  if (slave < 0)
    {
      skip(t, "terminal", "no pseudo-terminal");
      if (master >= 0)
        {
          close(master);
        }

      return;
    }

  b.pid = 0;
  ret = child_start(t, &a, "wait", "", POSIX_SPAWN_SETPGROUP, 0);
  if (ret == 0)
    {
      ret = child_start(t, &b, "wait", "", POSIX_SPAWN_SETPGROUP, a.pid);
    }

  if (ret != 0 || tcsetpgrp(slave, a.pid) < 0)
    {
      result(t, "terminal", false, "posix_spawn() %d or tcsetpgrp(), "
             "errno %d", ret, errno);
    }
  else
    {
      ret = write(master, "\003", 1);
      sa  = child_reap(&a);
      sb  = child_reap(&b);
      result(t, "terminal", ret == 1 && ended_by(sa, SIGINT) &&
             ended_by(sb, SIGINT), "^C: A ended %d, B ended %d",
             ended_by(sa, SIGINT), ended_by(sb, SIGINT));
    }

  child_stop(&a);
  child_stop(&b);
  close(slave);
  close(master);
#else
  skip(t, "terminal", "no CONFIG_PSEUDOTERM_SUSV1 or CONFIG_TTY_SIGINT");
#endif
}

/* The parent moves its children before they exec(): this needs fork() */

static void test_fork(FAR struct pgrptest_s *t)
{
#ifdef CONFIG_ARCH_HAVE_FORK
  int p[2];
  pid_t a;
  pid_t b;
  pid_t c;
  char ch;
  int ret;
  int st;

  if (pipe2(p, O_CLOEXEC) < 0)
    {
      skip(t, "fork", "no pipe");
      return;
    }

  a = fork();
  if (a == 0)
    {
      close(p[1]);
      read(p[0], &ch, 1);
      _exit(0);
    }

  b = fork();
  if (b == 0)
    {
      /* A child of B is in the process group of B */

      close(p[1]);
      read(p[0], &ch, 1);
      c = fork();
      if (c == 0)
        {
          _exit(getpgrp() == a ? 0 : 1);
        }

      st = 0;
      waitpid(c, &st, 0);
      _exit(WIFEXITED(st) ? WEXITSTATUS(st) : 2);
    }

  errno = 0;
  ret = setpgid(a, a);
  result(t, "fork", ret == 0, "parent setpgid(A, A) %d, errno %d", ret,
         errno);
  errno = 0;
  ret = setpgid(b, a);
  result(t, "fork", ret == 0 && getpgid(b) == a,
         "parent setpgid(B, A) %d, errno %d", ret, errno);

  close(p[0]);
  close(p[1]);
  waitpid(a, NULL, 0);
  st = 0;
  ret = waitpid(b, &st, 0) == b && WIFEXITED(st) &&
        WEXITSTATUS(st) == 0;
  result(t, "fork", ret, "a child of B is in the group A %d%c", ret, ' ');
#else
  skip(t, "fork", "no fork(): setpgid() of a child before exec()");
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct pgrptest_s t;

  if (argc >= 3 && strcmp(argv[1], "child") == 0)
    {
      return child_main(argv[2], argc > 3 ? argv[3] : "");
    }

  t.self  = argv[0];
  t.fails = 0;
  t.skips = 0;

  test_self(&t);
  test_spawn_pgroup(&t);
  test_kill_self(&t);
  test_spawn_setsid(&t);
  test_terminal(&t);
  test_fork(&t);

  printf(NAME ": done, %d failed, %d skipped\n", t.fails, t.skips);
  return t.fails != 0;
}
