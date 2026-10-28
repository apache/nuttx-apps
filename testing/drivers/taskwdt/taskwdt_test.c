/****************************************************************************
 * apps/testing/drivers/taskwdt/taskwdt_test.c
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

#include <sys/ioctl.h>
#include <sys/param.h>
#include <sched.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nuttx/irq.h>
#include <nuttx/timers/watchdog.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FEED_PERIOD_MS 250 /* How often the feeding cases feed */

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct testcase_s
{
  FAR const char *name;
  CODE int (*run)(unsigned int arg);
  unsigned int defarg;       /* Default of the only argument */
  bool resets;               /* The case must end in a system reset */
  FAR const char *help;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int case_idle(unsigned int sec);
static int case_feed(unsigned int sec);
static int case_write(unsigned int sec);
static int case_disarm(unsigned int sec);
static int case_stop(unsigned int sec);
static int case_status(unsigned int arg);
static int case_errors(unsigned int arg);
static int case_multi(unsigned int sec);
static int case_leak(unsigned int count);
static int case_starve(unsigned int timeout_ms);
static int case_exit(unsigned int timeout_ms);
static int case_kill(unsigned int timeout_ms);
static int case_two(unsigned int timeout_ms);
static int case_late(unsigned int timeout_ms);
static int case_latestop(unsigned int timeout_ms);
static int case_latev(unsigned int timeout_ms);
static int case_latestart(unsigned int timeout_ms);
static int case_handoff(unsigned int sec);
static int case_hog(unsigned int sec);
static int case_hang(unsigned int sec);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct testcase_s g_cases[] =
{
  { "idle", case_idle, 30, false, "sec: no channel, base layer feeds" },
  { "feed", case_feed, 20, false, "sec: feed with WDIOC_KEEPALIVE" },
  { "write", case_write, 20, false, "sec: feed with write()" },
  { "disarm", case_disarm, 12, false, "sec: 'V' disarms the channel" },
  { "stop", case_stop, 12, false, "sec: WDIOC_STOP and WDIOC_START" },
  { "status", case_status, 0, false, "WDIOC_GETSTATUS" },
  { "errors", case_errors, 0, false, "error codes" },
  { "multi", case_multi, 15, false, "sec: several channels" },
  { "leak", case_leak, 1000, false, "count: open/close cycles" },
  { "starve", case_starve, 2000, true, "ms: channel never fed" },
  { "exit", case_exit, 2000, true, "ms: channel closed while armed" },
  { "kill", case_kill, 2000, true, "ms: task deleted, channel armed" },
  { "two", case_two, 2000, true, "ms: only one of two threads feeds" },
  { "late", case_late, 1000, true, "ms: feeding resumes after expiry" },
  { "latestop", case_latestop, 500, true, "ms: WDIOC_STOP after expiry" },
  { "latev", case_latev, 500, true, "ms: 'V' after expiry" },
  { "latestart", case_latestart, 500, true, "ms: WDIOC_START after expiry" },
  { "handoff", case_handoff, 12, false, "sec: one opens, another feeds" },
  { "hog", case_hog, 0, true, "scheduler locked, work queue starves" },
  { "hang", case_hang, 0, true, "interrupts disabled" }
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int chan_open(unsigned int timeout_ms)
{
  int fd = open(CONFIG_WATCHDOG_DEVPATH, O_WRONLY);

  if (fd < 0)
    {
      printf("open %s failed: %d\n", CONFIG_WATCHDOG_DEVPATH, errno);
      return -1;
    }

  if (timeout_ms > 0 && ioctl(fd, WDIOC_SETTIMEOUT, timeout_ms) < 0)
    {
      printf("WDIOC_SETTIMEOUT %u failed: %d\n", timeout_ms, errno);
      close(fd);
      return -1;
    }

  return fd;
}

static int fail(FAR const char *name, FAR const char *why)
{
  printf("FAIL: %s: %s\n", name, why);
  return EXIT_FAILURE;
}

static int pass(FAR const char *name)
{
  printf("PASS: %s\n", name);
  return EXIT_SUCCESS;
}

/* Feed the channel every FEED_PERIOD_MS for sec seconds */

static void feed_for(int fd, unsigned int sec, bool useioctl)
{
  unsigned int i;

  for (i = 0; i < sec * 1000 / FEED_PERIOD_MS; i++)
    {
      if (useioctl)
        {
          ioctl(fd, WDIOC_KEEPALIVE, 0);
        }
      else
        {
          write(fd, "x", 1);
        }

      usleep(FEED_PERIOD_MS * 1000);
    }
}

/* Never return, give the supervisor all the time it needs */

static int wait_for_reset(unsigned int sec)
{
  printf("expect: system reset\n");
  fflush(stdout);
  sleep(sec);
  printf("FAIL: no reset after %u s\n", sec);
  return EXIT_FAILURE;
}

static volatile bool g_stop;

/* Threads of the two and multi cases, each with a channel of its own.
 * The name tells them apart in the timeout report.
 */

static FAR void *thread_feeder(FAR void *arg)
{
  int fd;

  pthread_setname_np(pthread_self(), "feeder");
  fd = chan_open((unsigned int)(uintptr_t)arg);
  if (fd >= 0)
    {
      while (!g_stop)
        {
          write(fd, "x", 1);
          usleep(FEED_PERIOD_MS * 1000);
        }

      write(fd, "V", 1);
      close(fd);
    }

  return NULL;
}

static FAR void *thread_starver(FAR void *arg)
{
  pthread_setname_np(pthread_self(), "starver");
  chan_open((unsigned int)(uintptr_t)arg);
  pause();
  return NULL;
}

/* Child task for the kill case */

static int child_main(int argc, FAR char *argv[])
{
  int fd = chan_open(atoi(argv[1]));

  if (fd >= 0)
    {
      feed_for(fd, 3600, false);
    }

  return 0;
}

/* Cases that must not reset */

static int case_idle(unsigned int sec)
{
  printf("idle for %u s with no channel\n", sec);
  sleep(sec);
  return pass("idle");
}

static int case_feed(unsigned int sec)
{
  int fd = chan_open(1000);

  if (fd < 0)
    {
      return EXIT_FAILURE;
    }

  feed_for(fd, sec, true);
  write(fd, "V", 1);
  close(fd);
  return pass("feed");
}

static int case_write(unsigned int sec)
{
  int fd = chan_open(1000);

  if (fd < 0)
    {
      return EXIT_FAILURE;
    }

  feed_for(fd, sec, false);
  write(fd, "V", 1);
  close(fd);
  return pass("write");
}

static int case_disarm(unsigned int sec)
{
  int fd = chan_open(1000);

  if (fd < 0)
    {
      return EXIT_FAILURE;
    }

  write(fd, "V", 1);
  printf("disarmed with V, waiting %u s without feeding\n", sec);
  sleep(sec);

  /* Writing anything else must not rearm it */

  write(fd, "x", 1);
  sleep(2);
  close(fd);
  return pass("disarm");
}

static int case_stop(unsigned int sec)
{
  struct watchdog_status_s st;
  int fd = chan_open(1000);

  if (fd < 0)
    {
      return EXIT_FAILURE;
    }

  if (ioctl(fd, WDIOC_STOP, 0) < 0)
    {
      return fail("stop", "WDIOC_STOP");
    }

  ioctl(fd, WDIOC_GETSTATUS, &st);
  if (st.flags != 0)
    {
      return fail("stop", "still active after WDIOC_STOP");
    }

  printf("stopped, waiting %u s without feeding\n", sec);
  sleep(sec);

  if (ioctl(fd, WDIOC_START, 0) < 0)
    {
      return fail("stop", "WDIOC_START");
    }

  ioctl(fd, WDIOC_GETSTATUS, &st);
  if ((st.flags & WDFLAGS_ACTIVE) == 0 || st.timeleft == 0)
    {
      return fail("stop", "not active after WDIOC_START");
    }

  feed_for(fd, 2, true);
  write(fd, "V", 1);
  close(fd);
  return pass("stop");
}

static int case_status(unsigned int arg)
{
  struct watchdog_status_s st;
  uint32_t before;
  int fd = chan_open(3000);

  if (fd < 0)
    {
      return EXIT_FAILURE;
    }

  if (ioctl(fd, WDIOC_GETSTATUS, &st) < 0 ||
      (st.flags & WDFLAGS_ACTIVE) == 0 || st.timeout != 3000 ||
      st.timeleft == 0 || st.timeleft > 3000)
    {
      return fail("status", "bad status after open");
    }

  usleep(1000 * 1000);
  ioctl(fd, WDIOC_GETSTATUS, &st);
  before = st.timeleft;
  if (before > 2200)
    {
      return fail("status", "timeleft does not decrease");
    }

  ioctl(fd, WDIOC_KEEPALIVE, 0);
  ioctl(fd, WDIOC_GETSTATUS, &st);
  if (st.timeleft <= before)
    {
      return fail("status", "KEEPALIVE did not restart the timeout");
    }

  write(fd, "V", 1);
  close(fd);
  return pass("status");
}

static int case_errors(unsigned int arg)
{
  int fd = chan_open(0);

  if (fd < 0)
    {
      return EXIT_FAILURE;
    }

  if (ioctl(fd, WDIOC_SETTIMEOUT, 0) >= 0 || errno != EINVAL)
    {
      return fail("errors", "SETTIMEOUT 0 must fail with EINVAL");
    }

  if (ioctl(fd, WDIOC_GETSTATUS, NULL) >= 0 || errno != EINVAL)
    {
      return fail("errors", "GETSTATUS NULL must fail with EINVAL");
    }

  if (ioctl(fd, _WDIOC(0x7f), 0) >= 0 || errno != ENOTTY)
    {
      return fail("errors", "unknown ioctl must fail with ENOTTY");
    }

  write(fd, "V", 1);
  close(fd);
  return pass("errors");
}

static int case_multi(unsigned int sec)
{
  pthread_t feeder;
  int fd = chan_open(1000);

  if (fd < 0)
    {
      return EXIT_FAILURE;
    }

  /* Another channel is opened and disarmed, only the feeder one counts */

  write(fd, "V", 1);
  pthread_create(&feeder, NULL, thread_feeder, (FAR void *)(uintptr_t)1000);
  printf("one channel fed, one disarmed, for %u s\n", sec);
  sleep(sec);
  g_stop = true;
  pthread_join(feeder, NULL);
  close(fd);
  return pass("multi");
}

static int case_leak(unsigned int count)
{
  struct mallinfo before;
  struct mallinfo after;
  unsigned int i;
  int fd;

  before = mallinfo();
  for (i = 0; i < count; i++)
    {
      fd = chan_open(0);
      if (fd < 0)
        {
          return EXIT_FAILURE;
        }

      write(fd, "V", 1);
      close(fd);
    }

  after = mallinfo();
  printf("heap in use: %d -> %d bytes after %u cycles\n",
         before.uordblks, after.uordblks, count);
  if (after.uordblks > before.uordblks)
    {
      return fail("leak", "heap grew");
    }

  return pass("leak");
}

/* Cases that must reset the system */

static int case_starve(unsigned int timeout_ms)
{
  int fd = chan_open(timeout_ms);

  if (fd < 0)
    {
      return EXIT_FAILURE;
    }

  printf("channel of %u ms never fed\n", timeout_ms);
  return wait_for_reset(60);
}

static int case_exit(unsigned int timeout_ms)
{
  int fd = chan_open(timeout_ms);

  if (fd < 0)
    {
      return EXIT_FAILURE;
    }

  /* Closed while armed: nobody can feed it anymore */

  close(fd);
  printf("channel of %u ms closed while armed\n", timeout_ms);
  return wait_for_reset(60);
}

static int case_kill(unsigned int timeout_ms)
{
  FAR char *argv[2];
  char arg[16];
  int pid;

  snprintf(arg, sizeof(arg), "%u", timeout_ms);
  argv[0] = arg;
  argv[1] = NULL;
  pid = task_create("victim", CONFIG_TESTING_TASKWDT_PRIORITY,
                    CONFIG_TESTING_TASKWDT_STACKSIZE, child_main, argv);
  if (pid < 0)
    {
      return fail("kill", "task_create");
    }

  sleep(1);
  printf("deleting pid %d that feeds a channel of %u ms\n", pid,
         timeout_ms);
  task_delete(pid);
  return wait_for_reset(60);
}

static int case_two(unsigned int timeout_ms)
{
  pthread_t feeder;
  pthread_t starver;

  pthread_create(&feeder, NULL, thread_feeder,
                 (FAR void *)(uintptr_t)timeout_ms);
  pthread_create(&starver, NULL, thread_starver,
                 (FAR void *)(uintptr_t)timeout_ms);

  printf("the starver thread never feeds, the feeder thread does\n");
  return wait_for_reset(60);
}

static int case_late(unsigned int timeout_ms)
{
  int fd = chan_open(timeout_ms);

  if (fd < 0)
    {
      return EXIT_FAILURE;
    }

  printf("starving %u ms, then feeding again\n", timeout_ms * 2);
  usleep(timeout_ms * 2 * 1000);
  feed_for(fd, 60, true);
  printf("FAIL: no reset after the timeout, feeding again\n");
  return EXIT_FAILURE;
}

/* A task that is late must not escape by changing its channel, before the
 * monitor notices that it is late.  Wait a little more than the timeout,
 * less than the keepalive interval, and try.
 */

static int late_op(unsigned int timeout_ms, int cmd)
{
  int fd = chan_open(timeout_ms);

  if (fd < 0)
    {
      return EXIT_FAILURE;
    }

  usleep((timeout_ms + timeout_ms / 5) * 1000);

  if (cmd == 0)
    {
      write(fd, "V", 1);
      printf("channel of %u ms late, then a 'V'\n", timeout_ms);
    }
  else
    {
      ioctl(fd, cmd, 0);
      printf("channel of %u ms late, then %s\n", timeout_ms,
             cmd == WDIOC_STOP ? "WDIOC_STOP" : "WDIOC_START");
    }

  feed_for(fd, 60, true);
  printf("FAIL: no reset after the timeout, changing the channel\n");
  return EXIT_FAILURE;
}

static int case_latestop(unsigned int timeout_ms)
{
  return late_op(timeout_ms, WDIOC_STOP);
}

static int case_latev(unsigned int timeout_ms)
{
  return late_op(timeout_ms, 0);
}

static int case_latestart(unsigned int timeout_ms)
{
  return late_op(timeout_ms, WDIOC_START);
}

static FAR void *thread_handoff(FAR void *arg)
{
  feed_for((int)(uintptr_t)arg, 12, false);
  return NULL;
}

static int case_handoff(unsigned int sec)
{
  pthread_t thread;
  int fd = chan_open(2000);

  if (fd < 0)
    {
      return EXIT_FAILURE;
    }

  pthread_create(&thread, NULL, thread_handoff, (FAR void *)(uintptr_t)fd);
  pthread_join(thread, NULL);
  write(fd, "V", 1);
  close(fd);
  return pass("handoff");
}

static int case_hog(unsigned int sec)
{
  /* Nothing else runs, in particular not the low priority work queue
   * that feeds the hardware watchdog.
   */

  printf("scheduler locked\n");
  fflush(stdout);
  sched_lock();
  while (true)
    {
    }

  return EXIT_FAILURE;
}

static int case_hang(unsigned int sec)
{
#ifdef CONFIG_BUILD_FLAT
  printf("interrupts disabled\n");
  fflush(stdout);
  enter_critical_section();
  while (true)
    {
    }

  return EXIT_FAILURE;
#else
  return fail("hang", "needs a flat build");
#endif
}

static void usage(FAR const char *progname)
{
  int i;

  printf("Usage: %s <case> [arg]\n\n", progname);
  printf("Cases (* = ends in a system reset):\n");
  for (i = 0; i < nitems(g_cases); i++)
    {
      printf("  %-7s%c %s\n", g_cases[i].name,
             g_cases[i].resets ? '*' : ' ', g_cases[i].help);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int i;

  if (argc >= 2)
    {
      for (i = 0; i < nitems(g_cases); i++)
        {
          if (strcmp(argv[1], g_cases[i].name) == 0)
            {
              return g_cases[i].run(argc > 2 ? atoi(argv[2]) :
                                    g_cases[i].defarg);
            }
        }
    }

  usage(argv[0]);
  return EXIT_FAILURE;
}
