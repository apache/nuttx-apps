/****************************************************************************
 * apps/examples/fdpicxip/modules/callback.c
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

/* Hands a module function to the firmware every way the firmware will
 * accept one.
 *
 * In FDPIC a function pointer is the address of a two-word descriptor, not
 * a code address, and that descriptor lives in the module's writable
 * segment in RAM.  Firmware that stores such a pointer and later branches to
 * it therefore jumps into the module's *data* and faults -- unless the entry
 * point it came through resolved the descriptor first.
 *
 * qsort and bsearch have always done that.  pthread_create, signal and
 * task_create did not, which made them look usable and then take the board
 * down; pthread_once, task_spawn, scandir's filter and sigaction followed
 * later.  This module is what keeps all of them honest.
 *
 * scandir is one worth noting: its filter is called by scandir itself and
 * must be resolved there, while its comparison function is handed to qsort,
 * which resolves it.  Resolving both would be a bug, so both are passed here
 * at once.
 *
 * mq_notify and timer_create with SIGEV_THREAD are the interesting ones:
 * their callback runs on a work-queue worker that carries no module data
 * base.  The base is captured at registration and installed around the call,
 * so the callback still reaches the module's own globals.  Each is checked
 * by having the callback write a distinctive value into a module global.
 *
 * Everything is checked by observing a side effect the callback itself
 * produces, because a callback that never runs is the failure being tested
 * for -- and on this target the alternative failure is a HardFault, which
 * reports nothing at all.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <dirent.h>
#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <spawn.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FAIL_QSORT    0x01
#define FAIL_PTHREAD  0x02
#define FAIL_SIGNAL   0x04
#define FAIL_TASK     0x08
#define FAIL_ONCE     0x10
#define FAIL_SPAWN    0x20
#define FAIL_SCANDIR  0x40
#define FAIL_SIGACTION 0x80

/* The mq checks run under "callback mq" and report in their own byte, so the
 * exit status -- only eight bits survive waitpid -- never has to hold more
 * than eight flags at once.
 */

#define FAIL_MQ_THREAD    0x01
#define FAIL_MQ_SIGNAL    0x02
#define FAIL_TIMER_THREAD 0x04

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Written by the callbacks, read by main.  These live in the module's
 * writable segment, which is what the callback can only reach if it was
 * entered with this module's data base in the FDPIC register.
 */

static volatile int g_qsort_calls;
static volatile int g_thread_ran;
static volatile int g_signal_ran;
static volatile int g_task_ran;
static volatile int g_once_ran;
static volatile int g_spawn_ran;
static volatile int g_filter_calls;
static volatile int g_compar_calls;
static volatile int g_sigaction_ran;
static volatile int g_mq_signal_ran;
static volatile int g_mq_thread_ran;
static volatile int g_timer_thread_ran;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int cmp_int(const void *a, const void *b)
{
  g_qsort_calls++;
  return *(const int *)a - *(const int *)b;
}

static void *thread_main(void *arg)
{
  g_thread_ran = (int)(intptr_t)arg;
  return NULL;
}

static void sig_handler(int signo)
{
  g_signal_ran = signo;
}

static void sigaction_handler(int signo, siginfo_t *info, void *context)
{
  g_sigaction_ran = signo;
}

static void mq_signal_handler(int signo, siginfo_t *info, void *context)
{
  g_mq_signal_ran = signo;
}

static void mq_thread_notify(union sigval value)
{
  /* Runs on a work-queue worker, which carries no module data base of its
   * own.  Writing this module global proves the base was installed around
   * the call -- without it this store would fault or land nowhere.
   */

  g_mq_thread_ran = value.sival_int;
}

static void timer_thread_notify(union sigval value)
{
  g_timer_thread_ran = value.sival_int;
}

static int task_main(int argc, char *argv[])
{
  g_task_ran = 1;
  return 0;
}

static void once_routine(void)
{
  g_once_ran++;
}

static int spawn_main(int argc, char *argv[])
{
  g_spawn_ran = 1;
  return 0;
}

static int dir_filter(const struct dirent *entry)
{
  g_filter_calls++;
  return 1;
}

static int dir_compar(const struct dirent **a, const struct dirent **b)
{
  g_compar_calls++;
  return strcmp((*a)->d_name, (*b)->d_name);
}

/* The message-queue checks run as a second invocation, "callback mq", so
 * their two flags report in a byte of their own rather than overflowing the
 * eight-bit exit status the other checks already fill.
 */

static int run_mq(void)
{
  struct mq_attr attr;
  struct sigaction sa;
  struct sigevent ev;
  struct itimerspec its;
  timer_t timer;
  mqd_t mqd;
  int fails = 0;
  int i;

  attr.mq_maxmsg = 1;
  attr.mq_msgsize = 1;
  attr.mq_flags = 0;

  mqd = mq_open("/cbmq", O_CREAT | O_RDWR, 0644, &attr);
  if (mqd == (mqd_t)-1)
    {
      return FAIL_MQ_THREAD | FAIL_MQ_SIGNAL | FAIL_TIMER_THREAD;
    }

  /* mq_notify with SIGEV_THREAD.  The callback runs on a work-queue worker
   * that carries no module data base of its own; the base captured at
   * registration is installed around the call, so the callback reaches this
   * module's globals.  A distinctive sival marks that the right callback
   * ran with the right argument.
   */

  memset(&ev, 0, sizeof(ev));
  ev.sigev_notify = SIGEV_THREAD;
  ev.sigev_notify_function = mq_thread_notify;
  ev.sigev_value.sival_int = 0x5a;

  if (mq_notify(mqd, &ev) < 0)
    {
      fails |= FAIL_MQ_THREAD;
    }
  else
    {
      char buf[1];

      /* The notification fires on a send to an empty queue; drain the
       * message afterwards so the queue is empty for the next test and the
       * next send does not block on a full queue.
       */

      mq_send(mqd, "x", 1, 0);

      for (i = 0; i < 50 && g_mq_thread_ran == 0; i++)
        {
          usleep(10000);
        }

      mq_receive(mqd, buf, sizeof(buf), NULL);

      if (g_mq_thread_ran != 0x5a)
        {
          fails |= FAIL_MQ_THREAD;
        }
    }

  /* mq_notify with SIGEV_SIGNAL, delivered to this task and caught by a
   * handler installed through sigaction.
   */

  memset(&sa, 0, sizeof(sa));
  sa.sa_sigaction = mq_signal_handler;
  sa.sa_flags = SA_SIGINFO;
  sigaction(SIGUSR1, &sa, NULL);

  memset(&ev, 0, sizeof(ev));
  ev.sigev_notify = SIGEV_SIGNAL;
  ev.sigev_signo = SIGUSR1;

  if (mq_notify(mqd, &ev) < 0)
    {
      fails |= FAIL_MQ_SIGNAL;
    }
  else
    {
      char buf[1];

      mq_send(mqd, "x", 1, 0);

      for (i = 0; i < 50 && g_mq_signal_ran == 0; i++)
        {
          usleep(10000);
        }

      mq_receive(mqd, buf, sizeof(buf), NULL);

      if (g_mq_signal_ran != SIGUSR1)
        {
          fails |= FAIL_MQ_SIGNAL;
        }
    }

  mq_close(mqd);
  mq_unlink("/cbmq");

  /* timer_create with SIGEV_THREAD reaches the module the same way: the
   * expiry callback runs on the worker with the module's base installed.
   */

  memset(&ev, 0, sizeof(ev));
  ev.sigev_notify = SIGEV_THREAD;
  ev.sigev_notify_function = timer_thread_notify;
  ev.sigev_value.sival_int = 0xa5;

  if (timer_create(CLOCK_MONOTONIC, &ev, &timer) < 0)
    {
      fails |= FAIL_TIMER_THREAD;
    }
  else
    {
      its.it_value.tv_sec = 0;
      its.it_value.tv_nsec = 20 * 1000 * 1000;
      its.it_interval.tv_sec = 0;
      its.it_interval.tv_nsec = 0;
      timer_settime(timer, 0, &its, NULL);

      for (i = 0; i < 50 && g_timer_thread_ran == 0; i++)
        {
          usleep(10000);
        }

      if (g_timer_thread_ran != 0xa5)
        {
          fails |= FAIL_TIMER_THREAD;
        }

      timer_delete(timer);
    }

  syslog(LOG_INFO,
         "[callback] mq_thread=%#x mq_signal=%d timer_thread=%#x -- %s\n",
         g_mq_thread_ran, g_mq_signal_ran, g_timer_thread_ran,
         fails == 0 ? "PASS" : "FAIL");

  return fails;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char *argv[])
{
  int v[5] =
  {
    5, 3, 1, 4, 2
  };

  struct dirent **namelist;
  pthread_once_t once = PTHREAD_ONCE_INIT;
  struct sigaction sa;
  pthread_t tid;
  int fails = 0;
  int n;
  int i;

  if (argc > 1 && strcmp(argv[1], "mq") == 0)
    {
      return run_mq();
    }

  /* qsort -- the path that always worked */

  qsort(v, 5, sizeof(int), cmp_int);
  if (g_qsort_calls == 0 || v[0] != 1 || v[4] != 5)
    {
      fails |= FAIL_QSORT;
    }

  /* pthread_create.  The thread inherits this module's D-Space, so it can
   * reach g_thread_ran only if it was entered at the right address.
   */

  if (pthread_create(&tid, NULL, thread_main, (void *)(intptr_t)7) != 0)
    {
      fails |= FAIL_PTHREAD;
    }
  else
    {
      pthread_join(tid, NULL);
      if (g_thread_ran != 7)
        {
          fails |= FAIL_PTHREAD;
        }
    }

  /* signal, delivered to this task */

  if (signal(SIGUSR1, sig_handler) == SIG_ERR)
    {
      fails |= FAIL_SIGNAL;
    }
  else
    {
      /* raise() rather than kill(getpid(), ...): getpid is not in the
       * firmware's export list, and raise targets the calling thread, which
       * is exactly what this is checking.
       */

      raise(SIGUSR1);

      for (i = 0; i < 50 && g_signal_ran == 0; i++)
        {
          usleep(10000);
        }

      if (g_signal_ran != SIGUSR1)
        {
          fails |= FAIL_SIGNAL;
        }
    }

  /* task_create.  A separate task, but the same D-Space. */

  if (task_create("cbtask", SCHED_PRIORITY_DEFAULT, 2048, task_main,
                  NULL) < 0)
    {
      fails |= FAIL_TASK;
    }
  else
    {
      for (i = 0; i < 50 && g_task_ran == 0; i++)
        {
          usleep(10000);
        }

      if (g_task_ran == 0)
        {
          fails |= FAIL_TASK;
        }
    }

  /* sigaction, delivered to this task.  signal() above resolves the handler
   * before calling sigaction; this reaches sigaction directly, which is the
   * path signal() does not cover.
   */

  memset(&sa, 0, sizeof(sa));
  sa.sa_sigaction = sigaction_handler;
  sa.sa_flags = SA_SIGINFO;

  if (sigaction(SIGUSR2, &sa, NULL) < 0)
    {
      fails |= FAIL_SIGACTION;
    }
  else
    {
      raise(SIGUSR2);

      for (i = 0; i < 50 && g_sigaction_ran == 0; i++)
        {
          usleep(10000);
        }

      if (g_sigaction_ran != SIGUSR2)
        {
          fails |= FAIL_SIGACTION;
        }
    }

  /* pthread_once.  Runs on this thread, so only the code address matters. */

  if (pthread_once(&once, once_routine) != 0 || g_once_ran != 1)
    {
      fails |= FAIL_ONCE;
    }

  /* task_spawn.  Like task_create, a separate task with the same D-Space. */

  if (task_spawn("cbspawn", spawn_main, NULL, NULL, NULL, NULL) < 0)
    {
      fails |= FAIL_SPAWN;
    }
  else
    {
      for (i = 0; i < 50 && g_spawn_ran == 0; i++)
        {
          usleep(10000);
        }

      if (g_spawn_ran == 0)
        {
          fails |= FAIL_SPAWN;
        }
    }

  /* scandir takes two callbacks that reach the module by different routes:
   * the filter is called by scandir itself, while the comparison function is
   * handed to qsort.  Passing both at once is the point -- it is what would
   * catch the comparison function being resolved twice.
   */

  namelist = NULL;
  n = scandir("/mnt/xipfs", &namelist, dir_filter, dir_compar);
  if (n < 0 || g_filter_calls == 0)
    {
      fails |= FAIL_SCANDIR;
    }

  if (namelist != NULL)
    {
      while (n-- > 0)
        {
          free(namelist[n]);
        }

      free(namelist);
    }

  syslog(LOG_INFO,
         "[callback] qsort=%d pthread=%d signal=%d task=%d sigaction=%d "
         "once=%d spawn=%d filter=%d compar=%d -- %s\n",
         g_qsort_calls, g_thread_ran, g_signal_ran, g_task_ran,
         g_sigaction_ran, g_once_ran, g_spawn_ran, g_filter_calls,
         g_compar_calls, fails == 0 ? "PASS" : "FAIL");

  return fails;
}
