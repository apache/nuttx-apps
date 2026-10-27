/****************************************************************************
 * apps/testing/ostest/pthread_sigqueue.c
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

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "ostest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_SIGNO     SIGUSR1
#define TEST_VAL_INT   42

/****************************************************************************
 * Private Data
 ****************************************************************************/

static sem_t g_ready_sem;
static sem_t g_done_sem;
static struct sigaction g_oact;
static bool g_act_installed;
static volatile bool g_sigreceived;
static volatile int g_received_signo;
static volatile int g_received_code;
static volatile int g_received_val;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sigqueue_handler
 ****************************************************************************/

static void sigqueue_handler(int signo, FAR siginfo_t *info,
                             FAR void *ucontext)
{
  if (info != NULL)
    {
      g_received_signo = signo;
      g_received_code  = info->si_code;
      g_received_val   = info->si_value.sival_int;
      g_sigreceived    = true;
    }

  sem_post(&g_done_sem);
}

/****************************************************************************
 * Name: sigqueue_thread
 ****************************************************************************/

static FAR void *sigqueue_thread(FAR void *arg)
{
  struct sigaction act;
  sigset_t set;
  int status;

  /* Setup signal action */

  act.sa_sigaction = sigqueue_handler;
  act.sa_flags     = SA_SIGINFO;
  sigemptyset(&act.sa_mask);

  status = sigaction(TEST_SIGNO, &act, &g_oact);
  if (status != OK)
    {
      printf("sigqueue_thread: sigaction failed, errno=%d\n", errno);
      ASSERT(false);
      return (FAR void *)ERROR;
    }

  g_act_installed = true;

  /* Unblock test signal */

  sigemptyset(&set);
  sigaddset(&set, TEST_SIGNO);
  status = pthread_sigmask(SIG_UNBLOCK, &set, NULL);
  if (status != OK)
    {
      printf("sigqueue_thread: pthread_sigmask failed: %d\n", status);
      ASSERT(false);
      return (FAR void *)ERROR;
    }

  /* Notify sender that we are ready */

  sem_post(&g_ready_sem);

  /* Wait for signal delivery */

  while (sem_wait(&g_done_sem) < 0)
    {
      if (errno != EINTR)
        {
          printf("sigqueue_thread: sem_wait failed: %d\n", errno);
          ASSERT(false);
          return (FAR void *)ERROR;
        }
    }

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pthread_sigqueue_test
 ****************************************************************************/

void pthread_sigqueue_test(void)
{
  pthread_t thread;
  union sigval val;
  int status;

  sem_init(&g_ready_sem, 0, 0);
  sem_init(&g_done_sem, 0, 0);

  g_sigreceived    = false;
  g_received_signo = -1;
  g_received_code  = -1;
  g_received_val   = -1;
  g_act_installed  = false;

  /* Test 1: Invalid signal number error handling */

  val.sival_int = TEST_VAL_INT;
  status = pthread_sigqueue(pthread_self(), -1, val);
  if (status != EINVAL)
    {
      printf("pthread_sigqueue_test: signo err: %d\n", status);
      ASSERT(false);
    }

  /* Test 2: Invalid thread ID error handling */

  status = pthread_sigqueue((pthread_t)-1, TEST_SIGNO, val);
  if (status != ESRCH)
    {
      printf("pthread_sigqueue_test: thread err: %d\n", status);
      ASSERT(false);
    }

  /* Test 3: Normal signal queuing to thread */

  status = pthread_create(&thread, NULL, sigqueue_thread, NULL);
  if (status != OK)
    {
      printf("pthread_sigqueue_test: pthread_create: %d\n", status);
      ASSERT(false);
      goto errout;
    }

  /* Wait until target thread has installed the handler and unblocked */

  while (sem_wait(&g_ready_sem) < 0)
    {
      if (errno != EINTR)
        {
          printf("pthread_sigqueue_test: sem_wait: %d\n", errno);
          ASSERT(false);
          goto errout;
        }
    }

  /* Send signal with payload to target thread */

  status = pthread_sigqueue(thread, TEST_SIGNO, val);
  if (status != OK)
    {
      printf("pthread_sigqueue_test: pthread_sigqueue: %d\n", status);
      ASSERT(false);
      goto errout;
    }

  /* Join target thread */

  status = pthread_join(thread, NULL);
  if (status != OK)
    {
      printf("pthread_sigqueue_test: pthread_join: %d\n", status);
      ASSERT(false);
      goto errout;
    }

  /* Restore signal action */

  if (g_act_installed)
    {
      status = sigaction(TEST_SIGNO, &g_oact, NULL);
      if (status != OK)
        {
          printf("pthread_sigqueue_test: sigaction restore: %d\n", errno);
          ASSERT(false);
        }

      g_act_installed = false;
    }

  /* Verify received signal properties */

  if (!g_sigreceived)
    {
      printf("pthread_sigqueue_test: signal not received\n");
      ASSERT(false);
    }

  if (g_received_signo != TEST_SIGNO)
    {
      printf("pthread_sigqueue_test: signo mismatch: %d\n",
             g_received_signo);
      ASSERT(false);
    }

  if (g_received_code != SI_QUEUE)
    {
      printf("pthread_sigqueue_test: si_code mismatch: %d\n",
             g_received_code);
      ASSERT(false);
    }

  if (g_received_val != TEST_VAL_INT)
    {
      printf("pthread_sigqueue_test: val mismatch: %d\n",
             g_received_val);
      ASSERT(false);
    }

  printf("pthread_sigqueue_test: SUCCESS\n");

errout:
  if (g_act_installed)
    {
      sigaction(TEST_SIGNO, &g_oact, NULL);
      g_act_installed = false;
    }

  sem_destroy(&g_ready_sem);
  sem_destroy(&g_done_sem);
}
