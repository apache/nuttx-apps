/****************************************************************************
 * apps/examples/ostest/semtimed.c
 *
 *   Copyright (C) 2014-2015 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <sched.h>
#include <errno.h>

#include "ostest.h"

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

#ifndef NULL
# error Broken toolchain does not have NULL
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static sem_t sem;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void *poster_func(void *parameter)
{
  int status;

  /* Wait for one second, then post the semaphore */

  printf("poster_func: Waiting for 1 second\n");
  sleep(1);

  printf("poster_func: Posting\n");
  status = sem_post(&sem);
  if (status != OK)
    {
      printf("poster_func: ERROR: sem_post failed\n");
    }

  return NULL;
}

static void ostest_gettime(struct timespec *tp)
{
  int status;

  status = clock_gettime(CLOCK_REALTIME, tp);
  if (status != OK)
    {
      printf("ostest_gettime: ERROR: clock_gettime failed\n");
    }
  else if (tp->tv_sec < 0 || tp->tv_nsec < 0 || tp->tv_nsec >= 1000*1000*1000)
    {
      printf("ostest_gettime: ERROR: clock_gettime returned bogus time\n");
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void semtimed_test(void)
{
  pthread_t poster_thread = (pthread_t)0;
#ifdef SDCC
  pthread_addr_t result;
#endif
  struct sched_param sparam;
  struct timespec abstime;
  struct timespec before;
  struct timespec after;
  int prio_min;
  int prio_max;
  int prio_mid;
  int errcode;
  pthread_attr_t attr;
  int status;

  printf("semtimed_test: Initializing semaphore to 0\n");
  status = sem_init(&sem, 0, 0);
  if (status != OK)
    {
      printf("semtimed_test: ERROR: sem_init failed\n");
    }

  /* First, make sure that the timeout expires if the semaphore is never posted */

  ostest_gettime(&before);

  abstime.tv_sec  = before.tv_sec + 2;
  abstime.tv_nsec = before.tv_nsec;

  printf("semtimed_test: Waiting for two second timeout\n");
  status  = sem_timedwait(&sem, &abstime);
  errcode = errno;

  ostest_gettime(&after);

  if (status == OK)
    {
      printf("semtimed_test: ERROR: sem_timedwait succeeded\n");
    }
  else
    {
      if (errcode == ETIMEDOUT)
        {
          printf("semtimed_test: PASS: first test returned timeout\n");
        }
      else
        {
          printf("semtimed_test: ERROR: sem_timedwait failed with: %d\n", errcode);
        }
    }

  printf("BEFORE: (%lu sec, %lu nsec)\n",
          (unsigned long)before.tv_sec, (unsigned long)before.tv_nsec);
  printf("AFTER:  (%lu sec, %lu nsec)\n",
          (unsigned long)after.tv_sec, (unsigned long)after.tv_nsec);

  /* Now make sure that the time wait returns successfully if the semaphore is posted */
  /* Start a poster thread.  It will wait 1 seconds and post the semaphore */

  printf("semtimed_test: Starting poster thread\n");
  status = pthread_attr_init(&attr);
  if (status != OK)
    {
      printf("semtimed_test: ERROR: pthread_attr_init failed, status=%d\n",  status);
    }

  prio_min = sched_get_priority_min(SCHED_FIFO);
  prio_max = sched_get_priority_max(SCHED_FIFO);
  prio_mid = (prio_min + prio_max) / 2;

  sparam.sched_priority = (prio_mid + prio_max) / 2;
  status = pthread_attr_setschedparam(&attr,&sparam);
  if (status != OK)
    {
      printf("semtimed_test: ERROR: pthread_attr_setschedparam failed, status=%d\n",  status);
    }
  else
    {
      printf("semtimed_test: Set thread 1 priority to %d\n",  sparam.sched_priority);
    }

  printf("semtimed_test: Starting poster thread 3\n");
  status = pthread_attr_init(&attr);
  if (status != 0)
    {
      printf("semtimed_test: ERROR: pthread_attr_init failed, status=%d\n",  status);
    }

  sparam.sched_priority = (prio_min + prio_mid) / 2;
  status = pthread_attr_setschedparam(&attr,&sparam);
  if (status != OK)
    {
      printf("semtimed_test: pthread_attr_setschedparam failed, status=%d\n",  status);
    }
  else
    {
      printf("semtimed_test: Set thread 3 priority to %d\n",  sparam.sched_priority);
    }

  status = pthread_create(&poster_thread, &attr, poster_func, NULL);
  if (status != 0)
    {
      printf("semtimed_test: ERROR: Poster thread creation failed: %d\n",  status);
      sem_destroy(&sem);
      return;
    }

  /* Up to two seconds for the semaphore to be posted */

  ostest_gettime(&before);

  abstime.tv_sec  = before.tv_sec + 2;
  abstime.tv_nsec = before.tv_nsec;

  printf("semtimed_test: Waiting for two second timeout\n");
  status  = sem_timedwait(&sem, &abstime);
  errcode = errno;

  ostest_gettime(&after);

  if (status < 0)
    {
      printf("semtimed_test: ERROR: sem_timedwait failed with: %d\n", errcode);
    }
  else
    {
      printf("semtimed_test: PASS: sem_timedwait succeeded\n");
    }

  printf("BEFORE: (%lu sec, %lu nsec)\n",
          (unsigned long)before.tv_sec, (unsigned long)before.tv_nsec);
  printf("AFTER:  (%lu sec, %lu nsec)\n",
          (unsigned long)after.tv_sec, (unsigned long)after.tv_nsec);

  /* Clean up detritus left by the pthread */

#ifdef SDCC
  if (poster_thread != (pthread_t)0)
    {
      pthread_join(poster_thread, &result);
    }
#else
  if (poster_thread != (pthread_t)0)
    {
      pthread_join(poster_thread, NULL);
    }
#endif

  sem_destroy(&sem);
}
