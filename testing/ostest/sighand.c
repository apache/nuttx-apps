/****************************************************************************
 * apps/testing/ostest/sighand.c
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

#include <sys/types.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>
#include <sched.h>
#include <errno.h>
#include "ostest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define WAKEUP_SIGNAL 17
#define SIGVALUE_INT  42

/****************************************************************************
 * Private Data
 ****************************************************************************/

static sem_t sem1;
static sem_t sem2;
static bool sigreceived = false;
static bool thread1exited = false;
static bool thread2exited = false;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_SCHED_HAVE_PARENT
static void death_of_child(int signo, siginfo_t *info, void *ucontext)
{
  /* Use of printf in a signal handler is NOT safe! It can cause deadlocks!
   * Also, signals are not queued by NuttX.  As a consequence, some
   * notifications will get lost (or the info data can be overwrittedn)!
   * Because POSIX  does not require signals to be queued, I do not think
   * that this is a bug (the overwriting is a bug, however).
   */

  if (info)
    {
      printf("death_of_child: PID %d received signal=%d code=%d "
             "errno=%d pid=%d status=%d\n",
             getpid(), signo, info->si_code, info->si_errno,
             info->si_pid, info->si_status);
    }
  else
    {
      printf("death_of_child: PID %d received signal=%d (no info?)\n",
             getpid(), signo);
    }
}
#endif

static void wakeup_action(int signo, siginfo_t *info, void *ucontext)
{
  sigset_t oldset;
  sigset_t allsigs;
  int status;

  /* Use of printf in a signal handler is NOT safe! It can cause deadlocks! */

  printf("wakeup_action: Received signal %d\n" , signo);

  sigreceived = true;

  /* Check signo */

  if (signo != WAKEUP_SIGNAL)
    {
      printf("wakeup_action: ERROR expected signo=%d\n" , WAKEUP_SIGNAL);
    }

  /* Check siginfo */

  if (info->si_value.sival_int != SIGVALUE_INT)
    {
      printf("wakeup_action: ERROR sival_int=%d expected %d\n",
              info->si_value.sival_int, SIGVALUE_INT);
    }
  else
    {
      printf("wakeup_action: sival_int=%d\n" , info->si_value.sival_int);
    }

  if (info->si_signo != WAKEUP_SIGNAL)
    {
      printf("wakeup_action: ERROR expected si_signo=%d, got=%d\n",
               WAKEUP_SIGNAL, info->si_signo);
    }

  printf("wakeup_action: si_code=%d\n" , info->si_code);

  /* Check ucontext_t */

  printf("wakeup_action: ucontext=%p\n" , ucontext);

  /* Check sigprocmask */

  sigfillset(&allsigs);
  status = sigprocmask(SIG_SETMASK, NULL, &oldset);
  if (status != OK)
    {
      printf("wakeup_action: ERROR sigprocmask failed, status=%d\n",
              status);
    }

  if (oldset != allsigs)
    {
      printf("wakeup_action: ERROR sigprocmask=%jx expected=%jx\n",
             (uintmax_t)oldset, (uintmax_t)allsigs);
    }

  /* Checkout sem_wait */

  status = sem_wait(&sem2);
  if (status != 0)
    {
      int error = errno;
      printf("wakeup_action: ERROR sem_wait failed, errno=%d\n" , error);
    }
  else
    {
      printf("wakeup_action: sem_wait() successfully!\n");
    }
}

static int waiter_main(int argc, char *argv[])
{
  sigset_t set;
  struct sigaction act;
  struct sigaction oact;
  int status;

  printf("waiter_main: Waiter started\n");

  printf("waiter_main: Unmasking signal %d\n" , WAKEUP_SIGNAL);
  sigemptyset(&set);
  sigaddset(&set, WAKEUP_SIGNAL);
  status = sigprocmask(SIG_UNBLOCK, &set, NULL);
  if (status != OK)
    {
      printf("waiter_main: ERROR sigprocmask failed, status=%d\n",
              status);
    }

  printf("waiter_main: Registering signal handler\n");
  act.sa_sigaction = wakeup_action;
  act.sa_flags  = SA_SIGINFO;

  sigfillset(&act.sa_mask);
  sigdelset(&act.sa_mask, WAKEUP_SIGNAL);

  status = sigaction(WAKEUP_SIGNAL, &act, &oact);
  if (status != OK)
    {
      printf("waiter_main: ERROR sigaction failed, status=%d\n" , status);
    }

#ifndef SDCC
  printf("waiter_main: oact.sigaction=%p oact.sa_flags=%x "
         "oact.sa_mask=%jx\n",
          oact.sa_sigaction, oact.sa_flags, (uintmax_t)oact.sa_mask);
#endif

  /* Take the semaphore */

  printf("waiter_main: Waiting on semaphore\n");
  FFLUSH();

  status = sem_wait(&sem1);
  if (status != 0)
    {
      int error = errno;
      if (error == EINTR)
        {
          printf("waiter_main: sem_wait() successfully interrupted by "
                 "signal\n");
        }
      else
        {
          printf("waiter_main: ERROR sem_wait failed, errno=%d\n" , error);
        }
    }
  else
    {
      printf("waiter_main: ERROR awakened with no error!\n");
    }

  /* Detach the signal handler */

  act.sa_handler = SIG_DFL;
  sigaction(WAKEUP_SIGNAL, &act, &oact);

  printf("waiter_main: done\n");
  FFLUSH();

  thread1exited = true;
  return 0;
}

static int poster_main(int argc, char *argv[])
{
  int status;

  printf("poster_main: Poster started\n");

  status = sem_post(&sem2);
  if (status != 0)
    {
      int error = errno;
      printf("poster_main: sem_post failed error=%d\n", error);
    }

  printf("poster_main: done\n");
  FFLUSH();

  thread2exited = true;
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void sighand_test(void)
{
#ifdef CONFIG_SCHED_HAVE_PARENT
  struct sigaction act;
  struct sigaction oact;
  sigset_t set;
#endif
  struct sched_param param;
  union sigval sigvalue;
  pid_t waiterpid, posterpid;
  int status;

  printf("sighand_test: Initializing semaphore to 0\n");
  sem_init(&sem1, 0, 0);
  sem_init(&sem2, 0, 0);

#ifdef CONFIG_SCHED_HAVE_PARENT
  printf("sighand_test: Unmasking SIGCHLD\n");

  sigemptyset(&set);
  sigaddset(&set, SIGCHLD);
  status = sigprocmask(SIG_UNBLOCK, &set, NULL);
  if (status != OK)
    {
      printf("sighand_test: ERROR sigprocmask failed, status=%d\n",
              status);
    }

  printf("sighand_test: Registering SIGCHLD handler\n");
  act.sa_sigaction = death_of_child;
  act.sa_flags  = SA_SIGINFO;

  sigfillset(&act.sa_mask);
  sigdelset(&act.sa_mask, SIGCHLD);

  status = sigaction(SIGCHLD, &act, &oact);
  if (status != OK)
    {
      printf("waiter_main: ERROR sigaction failed, status=%d\n" , status);
    }
#endif

  /* Start waiter thread  */

  printf("sighand_test: Starting waiter task\n");
  status = sched_getparam (0, &param);
  if (status != OK)
    {
      printf("sighand_test: ERROR sched_getparam() failed\n");
      param.sched_priority = PTHREAD_DEFAULT_PRIORITY;
    }

  waiterpid = task_create("waiter", param.sched_priority,
                           STACKSIZE, waiter_main, NULL);
  if (waiterpid == ERROR)
    {
      printf("sighand_test: ERROR failed to start waiter_main\n");
    }
  else
    {
      printf("sighand_test: Started waiter_main pid=%d\n", waiterpid);
    }

  /* Wait a bit */

  FFLUSH();
  sleep(2);

  /* Then signal the waiter thread. */

  printf("sighand_test: Signaling pid=%d with signo=%d sigvalue=%d\n",
         waiterpid, WAKEUP_SIGNAL, SIGVALUE_INT);

  sigvalue.sival_int = SIGVALUE_INT;
  status = sigqueue(waiterpid, WAKEUP_SIGNAL, sigvalue);
  if (status != OK)
    {
      printf("sighand_test: ERROR sigqueue failed\n");
      task_delete(waiterpid);
    }

  /* Start poster thread  */

  posterpid = task_create("poster", param.sched_priority,
                           STACKSIZE, poster_main, NULL);
  if (posterpid == ERROR)
    {
      printf("sighand_test: ERROR failed to start poster_main\n");
    }
  else
    {
      printf("sighand_test: Started poster_main pid=%d\n", posterpid);
    }

  /* Wait a bit */

  FFLUSH();
  sleep(2);

  /* Then check the result */

  if (!thread1exited)
    {
      printf("sighand_test: ERROR waiter task did not exit\n");
    }

  if (!thread2exited)
    {
      printf("sighand_test: ERROR poster task did not exit\n");
    }

  if (!sigreceived)
    {
      printf("sighand_test: ERROR signal handler did not run\n");
    }

  /* Detach the signal handler */

#ifdef CONFIG_SCHED_HAVE_PARENT
  act.sa_handler = SIG_DFL;
  sigaction(SIGCHLD, &act, &oact);
#endif

  printf("sighand_test: done\n");
  FFLUSH();
}
