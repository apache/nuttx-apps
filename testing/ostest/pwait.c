/****************************************************************************
 * apps/testing/ostest/pwait.c
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

#include <sys/epoll.h>
#include <sys/select.h>

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "ostest.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum pwait_call_e
{
  PWAIT_PPOLL = 0,
  PWAIT_PSELECT,
  PWAIT_EPOLL,
  PWAIT_NCALLS
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static FAR const char * const g_pwait_name[PWAIT_NCALLS] =
{
  "ppoll", "pselect", "epoll_pwait"
};

static volatile sig_atomic_t g_pwait_handled;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void pwait_handler(int signo)
{
  g_pwait_handled++;
}

/* Send SIGUSR1 to the main thread while it waits */

static FAR void *pwait_thread(FAR void *arg)
{
  FAR pthread_t *waiter = arg;

  usleep(100000);
  pthread_kill(*waiter, SIGUSR1);
  return NULL;
}

/* Wait for one second with no descriptor (an empty epoll set for
 * epoll_pwait()), with a mask that unblocks SIGUSR1.  The call must give
 * EINTR after the handler ran, and then SIGUSR1 must be blocked again.
 */

static int pwait_check(int call, FAR const char *when, int epfd,
                       FAR const sigset_t *mask)
{
  struct epoll_event ev;
  struct timespec ts;
  sigset_t cur;
  int ret;

  ts.tv_sec  = 1;
  ts.tv_nsec = 0;

  switch (call)
    {
      case PWAIT_PPOLL:
        ret = ppoll(NULL, 0, &ts, mask);
        break;

      case PWAIT_PSELECT:
        ret = pselect(0, NULL, NULL, NULL, &ts, mask);
        break;

      default:
        ret = epoll_pwait(epfd, &ev, 1, 1000, mask);
        break;
    }

  if (ret != -1 || errno != EINTR || g_pwait_handled != 1)
    {
      printf("pwait_test: ERROR: %s, signal %s: ret %d errno %d "
             "handler ran %d times\n",
             g_pwait_name[call], when, ret, errno, g_pwait_handled);
      return 1;
    }

  sigprocmask(SIG_SETMASK, NULL, &cur);
  if (!sigismember(&cur, SIGUSR1))
    {
      printf("pwait_test: ERROR: %s, signal %s: mask not restored\n",
             g_pwait_name[call], when);
      return 1;
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pwait_test
 *
 * Description:
 *   The thread blocks SIGUSR1 and unblocks it only in the mask of ppoll(),
 *   pselect() or epoll_pwait().  A SIGUSR1 that is pending before the call
 *   or that comes during the wait must run the handler before the call
 *   gives EINTR.
 *
 ****************************************************************************/

void pwait_test(void)
{
  struct sigaction act;
  struct sigaction oact;
  pthread_t self;
  pthread_t thread;
  sigset_t oldmask;
  sigset_t mask;
  int errors = 0;
  int epfd;
  int call;
  int ret;

  epfd = epoll_create1(EPOLL_CLOEXEC);
  if (epfd < 0)
    {
      printf("pwait_test: ERROR: epoll_create1 failed: %d\n", errno);
      ASSERT(false);
    }

  act.sa_handler = pwait_handler;
  act.sa_flags   = 0;
  sigemptyset(&act.sa_mask);
  sigaction(SIGUSR1, &act, &oact);

  sigemptyset(&mask);
  sigaddset(&mask, SIGUSR1);
  sigprocmask(SIG_BLOCK, &mask, &oldmask);

  mask = oldmask;
  sigdelset(&mask, SIGUSR1);

  self = pthread_self();
  for (call = 0; call < PWAIT_NCALLS; call++)
    {
      /* SIGUSR1 is pending before the call */

      g_pwait_handled = 0;
      pthread_kill(self, SIGUSR1);
      errors += pwait_check(call, "pending", epfd, &mask);

      /* SIGUSR1 comes during the wait */

      g_pwait_handled = 0;
      ret = pthread_create(&thread, NULL, pwait_thread, &self);
      if (ret != 0)
        {
          printf("pwait_test: ERROR: pthread_create failed: %d\n", ret);
          ASSERT(false);
        }

      errors += pwait_check(call, "during the wait", epfd, &mask);
      pthread_join(thread, NULL);
    }

  sigprocmask(SIG_SETMASK, &oldmask, NULL);
  sigaction(SIGUSR1, &oact, NULL);
  close(epfd);

  if (errors != 0)
    {
      printf("pwait_test: ERROR: %d checks failed\n", errors);
      ASSERT(false);
    }

  printf("pwait_test: PASS\n");
}
