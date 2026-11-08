/****************************************************************************
 * apps/testing/fs/filelock/filelock_main.c
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

/* A signal must end the wait for a file lock.  POSIX fcntl(): "If a signal
 * is caught while fcntl() is waiting for a region, fcntl() shall be
 * interrupted.  Upon return from the signal handler, fcntl() shall return
 * -1 with errno set to [EINTR], and the lock operation shall not be done."
 *
 * The main thread holds a write lock for FILELOCK_HOLD_MS.  A thread with
 * its own open file waits for the same lock and gets SIGUSR1 (a handler
 * without SA_RESTART) after FILELOCK_SIGNAL_MS.  The file locks of NuttX
 * belong to the thread and the open file, so the thread is a different
 * owner.  The test never hangs: the main thread always unlocks.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/file.h>

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FILELOCK_PATH      "/tmp/filelock.lck"
#define FILELOCK_HOLD_MS   2000
#define FILELOCK_SIGNAL_MS 300

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum filelock_how_e
{
  HOW_FCNTL,                   /* fcntl(F_SETLKW) */
  HOW_FLOCK,                   /* flock(LOCK_EX) */
  HOW_LOCKF,                   /* lockf(F_LOCK) */
  HOW_NUM
};

struct filelock_waiter_s
{
  FAR const char *path;
  enum filelock_how_e how;
  int ret;
  int err;
  volatile sig_atomic_t handler; /* The signal handler ran */
  long ms;                       /* Time from the start of the wait */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static FAR const char * const g_names[HOW_NUM] =
{
  "fcntl", "flock", "lockf"
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Return the number of errors: 0 or 1 */

static int filelock_check(FAR const char *how, FAR const char *name,
                          bool ok, long val)
{
  printf("filelock: %s %s %s: %ld\n", how, name, ok ? "PASS" : "FAIL", val);
  return ok ? 0 : 1;
}

static long filelock_now_ms(void)
{
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static void filelock_sleep_ms(long ms)
{
  struct timespec ts;

  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000;
  while (nanosleep(&ts, &ts) < 0 && errno == EINTR)
    {
    }
}

/* The signal carries the waiter of the thread that gets it */

static void filelock_handler(int signo, FAR siginfo_t *info,
                             FAR void *context)
{
  FAR struct filelock_waiter_s *w = info->si_value.sival_ptr;

  if (w != NULL)
    {
      w->handler = 1;
    }
}

static int filelock_setlk(int fd, int cmd, short type)
{
  struct flock fl;

  memset(&fl, 0, sizeof(fl));
  fl.l_type = type;
  fl.l_whence = SEEK_SET;
  return fcntl(fd, cmd, &fl);
}

static FAR void *filelock_waiter(FAR void *arg)
{
  FAR struct filelock_waiter_s *w = arg;
  long t0;
  int fd;

  fd = open(w->path, O_RDWR | O_CLOEXEC);
  if (fd < 0)
    {
      w->ret = -2;
      w->err = errno;
      return NULL;
    }

  t0 = filelock_now_ms();
  errno = 0;
  switch (w->how)
    {
      case HOW_FCNTL:
        w->ret = filelock_setlk(fd, F_SETLKW, F_WRLCK);
        break;

      case HOW_FLOCK:
        w->ret = flock(fd, LOCK_EX);
        break;

      default:
        w->ret = lockf(fd, F_LOCK, 0);
        break;
    }

  w->err = errno;
  w->ms = filelock_now_ms() - t0;
  close(fd);
  return NULL;
}

/* Hold the lock on fd while a thread waits for it.  Send the signal to the
 * thread after FILELOCK_SIGNAL_MS if sig is true.  Unlock after
 * FILELOCK_HOLD_MS.
 */

static int filelock_run(int fd, bool sig, FAR struct filelock_waiter_s *w)
{
  struct sched_param param;
  pthread_attr_t attr;
  pthread_t thread;
  union sigval value;
  long t0;
  int ret;

  if (filelock_setlk(fd, F_SETLK, F_WRLCK) < 0)
    {
      printf("filelock: F_SETLK: errno %d\n", errno);
      return -1;
    }

  /* The thread runs below the main thread, so that the main thread can
   * unlock also if the thread does not sleep.
   */

  sched_getparam(0, &param);
  param.sched_priority--;
  pthread_attr_init(&attr);
  pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
  pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
  pthread_attr_setschedparam(&attr, &param);

  t0 = filelock_now_ms();
  ret = pthread_create(&thread, &attr, filelock_waiter, w);
  pthread_attr_destroy(&attr);
  if (ret != 0)
    {
      printf("filelock: pthread_create: %d\n", ret);
      filelock_setlk(fd, F_SETLK, F_UNLCK);
      return -1;
    }

  filelock_sleep_ms(FILELOCK_SIGNAL_MS);
  if (sig)
    {
      value.sival_ptr = w;
      pthread_sigqueue(thread, SIGUSR1, value);
    }

  filelock_sleep_ms(FILELOCK_HOLD_MS - (filelock_now_ms() - t0));
  filelock_setlk(fd, F_SETLK, F_UNLCK);
  pthread_join(thread, NULL);
  return 0;
}

/* A lock that another owner holds: the calls that do not wait fail.
 * Return the number of errors.
 */

static int filelock_nowait(FAR const char *path, int fd)
{
  int errors = 0;
  int fd2;
  int ret;

  if (filelock_setlk(fd, F_SETLK, F_WRLCK) < 0)
    {
      return filelock_check("fcntl", "setlk", false, errno);
    }

  fd2 = open(path, O_RDWR | O_CLOEXEC);
  if (fd2 < 0)
    {
      errors = filelock_check("fcntl", "open", false, errno);
      filelock_setlk(fd, F_SETLK, F_UNLCK);
      return errors;
    }

  ret = filelock_setlk(fd2, F_SETLK, F_WRLCK);
  errors += filelock_check("fcntl", "busy",
                           ret < 0 && (errno == EAGAIN || errno == EACCES),
                           errno);
  ret = flock(fd2, LOCK_EX | LOCK_NB);
  errors += filelock_check("flock", "busy",
                           ret < 0 && errno == EWOULDBLOCK, errno);
  ret = lockf(fd2, F_TLOCK, 0);
  errors += filelock_check("lockf", "busy",
                           ret < 0 && (errno == EAGAIN || errno == EACCES),
                           errno);

  close(fd2);
  filelock_setlk(fd, F_SETLK, F_UNLCK);
  return errors;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR const char *path = FILELOCK_PATH;
  struct filelock_waiter_s w;
  struct sigaction act;
  int errors;
  int how;
  int fd;

  if (argc > 1)
    {
      path = argv[1];
    }

  fd = open(path, O_RDWR | O_CREAT | O_CLOEXEC, 0666);
  if (fd < 0)
    {
      printf("filelock: open %s: errno %d\n", path, errno);
      return EXIT_FAILURE;
    }

  memset(&act, 0, sizeof(act));
  act.sa_sigaction = filelock_handler;
  act.sa_flags = SA_SIGINFO;
  sigemptyset(&act.sa_mask);
  sigaction(SIGUSR1, &act, NULL);

  errors = filelock_nowait(path, fd);

  /* A signal ends the wait with EINTR, before the unlock */

  for (how = 0; how < HOW_NUM; how++)
    {
      memset(&w, 0, sizeof(w));
      w.path = path;
      w.how = how;
      if (filelock_run(fd, true, &w) < 0)
        {
          errors++;
          continue;
        }

      printf("filelock: %s signal: ret %d errno %d after %ld ms\n",
             g_names[how], w.ret, w.err, w.ms);
      errors += filelock_check(g_names[how], "eintr",
                               w.ret == -1 && w.err == EINTR, w.err);
      errors += filelock_check(g_names[how], "early",
                               w.ms < FILELOCK_HOLD_MS - FILELOCK_SIGNAL_MS,
                               w.ms);
      errors += filelock_check(g_names[how], "handler", w.handler != 0,
                               w.handler);
    }

  /* Without a signal the thread gets the lock after the unlock */

  memset(&w, 0, sizeof(w));
  w.path = path;
  w.how = HOW_FCNTL;
  if (filelock_run(fd, false, &w) < 0)
    {
      errors++;
    }
  else
    {
      errors += filelock_check("fcntl", "wait",
                               w.ret == 0 &&
                               w.ms >= FILELOCK_HOLD_MS - FILELOCK_SIGNAL_MS,
                               w.ms);
    }

  close(fd);
  unlink(path);
  printf("filelock: DONE errors %d\n", errors);
  return errors == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
