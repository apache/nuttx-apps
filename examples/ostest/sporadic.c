/****************************************************************************
 * apps/examples/ostest/sporadic.c
 *
 *   Copyright (C) 2015 Gregory Nutt. All rights reserved.
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

#include <nuttx/config.h>

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include <time.h>

#include "ostest.h"

#ifdef CONFIG_SCHED_SPORADIC

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* It is actually a better test without schedule locking because that
 * forces the scheduler into an uninteresting fallback mode.
 */

#undef sched_lock
#undef sched_unlock
#define sched_lock()
#define sched_unlock()

#ifndef MIN
# define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static sem_t g_sporadic_sem;
static time_t g_start_time;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void my_mdelay(unsigned int milliseconds)
{
  volatile unsigned int i;
  volatile unsigned int j;

  for (i = 0; i < milliseconds; i++)
    {
      for (j = 0; j < CONFIG_BOARD_LOOPSPERMSEC; j++)
        {
        }
    }
}

static void *nuisance_func(void *parameter)
{
  /* Synchronized start */

  while (sem_wait(&g_sporadic_sem) < 0);

  /* Sleep until we are cancelled */

  for (;;)
    {
      /* Sleep gracefully for awhile */

      usleep(500*1000);

      /* Then hog some CPU time */

      my_mdelay(100);
    }

  return NULL;
}

static void *fifo_func(void *parameter)
{
  struct sched_param param;
  time_t last;
  time_t now;
  int ret;

  while (sem_wait(&g_sporadic_sem) < 0);

  last  = g_start_time;

  for (;;)
    {
      do
        {
          sched_lock(); /* Just to exercise more logic */
          ret = sched_getparam(0, &param);
          if (ret < 0)
            {
              printf("ERROR: sched_getparam failed\n");
              return NULL;
            }

          now = time(NULL);
          sched_unlock();
        }
      while (now == last);

      sched_lock(); /* Just to exercise more logic */
      printf("%4lu FIFO:     %d\n",
             (unsigned long)(now-g_start_time), param.sched_priority);
      last = now;
      sched_unlock();
    }
}

static void *sporadic_func(void *parameter)
{
  struct sched_param param;
  time_t last;
  time_t now;
  int prio = 0;
  int ret;

  while (sem_wait(&g_sporadic_sem) < 0);

  last  = g_start_time;

  for (;;)
    {
      do
        {
          sched_lock(); /* Just to exercise more logic */
          ret = sched_getparam(0, &param);
          if (ret < 0)
            {
              printf("ERROR: sched_getparam failed\n");
              return NULL;
            }

          now = time(NULL);
          sched_unlock();
        }
      while (now == last && prio == param.sched_priority);

      sched_lock(); /* Just to exercise more logic */
      printf("%4lu SPORADIC: %d->%d\n",
             (unsigned long)(now-g_start_time), prio, param.sched_priority);
      prio = param.sched_priority;
      last = now;
      sched_unlock();
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void sporadic_test(void)
{
  pthread_t nuisance_thread = (pthread_t)0;
  pthread_t sporadic_thread = (pthread_t)0;
  pthread_t fifo_thread = (pthread_t)0;
#ifdef SDCC
  pthread_addr_t result;
#endif
  FAR void *result;
  struct sched_param myparam;
  struct sched_param sparam;
  int prio_min;
  int prio_max;
  int prio_low;
  int prio_mid;
  int prio_high;
  pthread_attr_t attr;
  int ret;

#if CONFIG_SCHED_SPORADIC_MAXREPL < 5
  printf("sporadic_test: CONFIG_SCHED_SPORADIC_MAXREPL is small: %d\n",
         CONFIG_SCHED_SPORADIC_MAXREPL);
  printf("  -- There will some errors in the replenishment interval\n");
#endif

  printf("sporadic_test: Initializing semaphore to 0\n");
  sem_init(&g_sporadic_sem, 0, 0);

  prio_min  = sched_get_priority_min(SCHED_FIFO);
  prio_max  = sched_get_priority_max(SCHED_FIFO);

  prio_low  = prio_min + ((prio_max - prio_min) >> 2);
  prio_mid  = (prio_min + prio_max) >> 1;
  prio_high = prio_max - ((prio_max - prio_min) >> 2);

  /* Temporarily set our priority to prio_high + 2 */

  ret = sched_getparam(0, &myparam);
  if (ret != OK)
    {
      printf("sporadic_test: ERROR: sched_getparam failed, ret=%d\n", ret);
    }

  sparam.sched_priority = prio_high + 2;
  ret = sched_setparam(0, &sparam);
  if (ret != OK)
    {
      printf("sporadic_test: ERROR: sched_setparam failed, ret=%d\n", ret);
    }

  ret = pthread_attr_init(&attr);
  if (ret != OK)
    {
      printf("sporadic_test: ERROR: pthread_attr_init failed, ret=%d\n",
             ret);
    }

  /* This semaphore will prevent anything from running until we are ready */

  sched_lock();
  sem_init(&g_sporadic_sem, 0, 0);

  /* Start a FIFO thread at the highest priority (prio_max + 1) */

  printf("sporadic_test: Starting FIFO thread at priority %d\n", prio_mid);

  ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
  if (ret != OK)
    {
      printf("sporadic_test: ERROR: pthread_attr_setschedpolicy failed, ret=%d\n",
             ret);
    }

  sparam.sched_priority = prio_high + 1;
  ret = pthread_attr_setschedparam(&attr, &sparam);
  if (ret != OK)
    {
      printf("sporadic_test: ERROR: pthread_attr_setschedparam failed, ret=%d\n",
             ret);
    }

  ret = pthread_create(&nuisance_thread, &attr, nuisance_func, NULL);
  if (ret != 0)
    {
      printf("sporadic_test: ERROR: FIFO thread creation failed: %d\n",
             ret);
    }

  /* Start a FIFO thread at the middle priority */

  sparam.sched_priority = prio_mid;
  ret = pthread_attr_setschedparam(&attr, &sparam);
  if (ret != OK)
    {
      printf("sporadic_test: ERROR: pthread_attr_setschedparam failed, ret=%d\n",
             ret);
    }

  ret = pthread_create(&fifo_thread, &attr, fifo_func, NULL);
  if (ret != 0)
    {
      printf("sporadic_test: ERROR: FIFO thread creation failed: %d\n",
             ret);
    }

  /* Start a sporadic thread, with the following parameters: */

  printf("sporadic_test: Starting sporadic thread at priority %d\n",
         prio_high, prio_low);

  ret = pthread_attr_setschedpolicy(&attr, SCHED_SPORADIC);
  if (ret != OK)
    {
      printf("sporadic_test: ERROR: pthread_attr_setschedpolicy failed, ret=%d\n",
             ret);
    }

  sparam.sched_priority               = prio_high;
  sparam.sched_ss_low_priority        = prio_low;
  sparam.sched_ss_repl_period.tv_sec  = 5;
  sparam.sched_ss_repl_period.tv_nsec = 0;
  sparam.sched_ss_init_budget.tv_sec  = 2;
  sparam.sched_ss_init_budget.tv_nsec = 0;
  sparam.sched_ss_max_repl            = CONFIG_SCHED_SPORADIC_MAXREPL;

  ret = pthread_attr_setschedparam(&attr, &sparam);
  if (ret != OK)
    {
      printf("sporadic_test: ERROR: pthread_attr_setsched param failed, ret=%d\n",
             ret);
    }

  ret = pthread_create(&sporadic_thread, &attr, sporadic_func, (pthread_addr_t)1);
  if (ret != 0)
    {
      printf("sporadic_test: ERROR: sporadic thread creation failed: %d\n",
             ret);
    }

  g_start_time = time(NULL);

  sem_post(&g_sporadic_sem);
  sem_post(&g_sporadic_sem);
  sem_post(&g_sporadic_sem);

  /* Wait a while then kill the FIFO thread */

  sleep(15);
  ret = pthread_cancel(fifo_thread);
  pthread_join(fifo_thread, &result);

  /* Wait a bit longer then kill the nuisance thread */

  sleep(10);
  ret = pthread_cancel(nuisance_thread);
  pthread_join(nuisance_thread, &result);

  /* Wait a bit longer then kill the sporadic thread */

  sleep(10);
  ret = pthread_cancel(sporadic_thread);
  pthread_join(sporadic_thread, &result);
  sched_unlock();

  printf("sporadic_test: Done\n");
  sem_destroy(&g_sporadic_sem);

  ret = sched_setparam(0, &myparam);
  if (ret != OK)
    {
      printf("sporadic_test: ERROR: sched_setparam failed, ret=%d\n", ret);
    }
}

#endif /* CONFIG_SCHED_SPORADIC */
