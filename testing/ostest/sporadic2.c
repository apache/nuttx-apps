/****************************************************************************
 * apps/testing/ostest/sporadic2.c
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include <time.h>

#ifdef CONFIG_SCHED_SPORADIC

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define PRIO_LOW1     20
#define PRIO_LOW2     30
#define PRIO_MID      100
#define PRIO_HIGH1    180
#define PRIO_HIGH2    170
#define REPL_INTERVAL 100000000L
#define MAX_BUDGET    (REPL_INTERVAL / 2)
#define PRIO_HI_NDX   0
#define PRIO_LO_NDX   1

#ifndef MIN
# define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
# define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static sem_t g_sporadic_sem;
static time_t g_start_time;

/* These counters are incremented approximately once per millisecond by the
 * sporadic threads to provide a rough estimate of the time spent by each
 * thread in each state.
 */

static int32_t g_ms_cnt1[2] =
{
  0, 0
};

static int32_t g_ms_cnt2[2] =
{
  0, 0
};

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

static FAR void *sporadic_func(FAR void *parameter)
{
  FAR int32_t *counter = (FAR void *)parameter;
  struct sched_param param;
  time_t last;
  time_t now;
  int prio = 0;
  int ret;

  while (sem_wait(&g_sporadic_sem) < 0)
    {
    }

  last  = g_start_time;

  for (; ; )
    {
      do
        {
          int ndx;

          /* Hog the CPU for approximately 1 MS (depends on proper
           * calibration of the timing loop!).
           */

          my_mdelay(1);

          /* Get the current thread priority */

          ret = sched_getparam(0, &param);
          if (ret < 0)
            {
              printf("ERROR: sched_getparam failed\n");
              return NULL;
            }

          /* Increment the counter associated with the current priority
           * state.
           */

          if (param.sched_priority > PRIO_MID)
            {
              ndx = PRIO_HI_NDX;
            }
          else
            {
              ndx = PRIO_LO_NDX;
            }

          counter[ndx]++;
          now = time(NULL);
        }
      while (now == last && prio == param.sched_priority);

      prio = param.sched_priority;
      last = now;
    }
}

static void sporadic_test_case(int32_t budget_1_ns, int32_t budget_2_ns)
{
  pthread_t sporadic_thread1 = (pthread_t)0;
  pthread_t sporadic_thread2 = (pthread_t)0;
#ifdef SDCC
  pthread_addr_t result;
#endif
  FAR void *result;
  struct sched_param myparam;
  struct sched_param sparam;
  pthread_attr_t attr;
  int ret;

#if CONFIG_SCHED_SPORADIC_MAXREPL < 5
  printf("sporadic_test: CONFIG_SCHED_SPORADIC_MAXREPL is small: %d\n",
         CONFIG_SCHED_SPORADIC_MAXREPL);
  printf("  -- There will some errors in the replenishment interval\n");
#endif

  sem_init(&g_sporadic_sem, 0, 0);

  /* initilize global worker-thread millisecons-counters */

  g_ms_cnt1[PRIO_HI_NDX] = 0;
  g_ms_cnt1[PRIO_LO_NDX] = 0;
  g_ms_cnt2[PRIO_HI_NDX] = 0;
  g_ms_cnt2[PRIO_LO_NDX] = 0;

  ret = sched_getparam(0, &myparam);
  if (ret != OK)
    {
      printf("sporadic_test: ERROR: sched_getparam failed, ret=%d\n", ret);
    }

  /* Temporarily set our priority to PRIO_HIGH + 2 */

  sparam.sched_priority = MAX(PRIO_HIGH1, PRIO_HIGH2) + 2;
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

  sem_init(&g_sporadic_sem, 0, 0);

  /* Start a sporadic thread, with the following parameters: */

  ret = pthread_attr_setschedpolicy(&attr, SCHED_SPORADIC);
  if (ret != OK)
    {
      printf("sporadic_test: ERROR: pthread_attr_setschedpolicy failed, "
             "ret=%d\n",
             ret);
    }

  sparam.sched_priority               = PRIO_HIGH1;
  sparam.sched_ss_low_priority        = PRIO_LOW1;
  sparam.sched_ss_repl_period.tv_sec  = 0;
  sparam.sched_ss_repl_period.tv_nsec = REPL_INTERVAL;
  sparam.sched_ss_init_budget.tv_sec  = 0;
  sparam.sched_ss_init_budget.tv_nsec = budget_1_ns;
  sparam.sched_ss_max_repl            = CONFIG_SCHED_SPORADIC_MAXREPL;

  ret = pthread_attr_setschedparam(&attr, &sparam);
  if (ret != OK)
    {
      printf("sporadic_test: ERROR: pthread_attr_setsched param failed, "
             "ret=%d\n",
             ret);
    }

  ret = pthread_create(&sporadic_thread1, &attr, sporadic_func,
                       (pthread_addr_t)g_ms_cnt1);
  if (ret != 0)
    {
      printf("sporadic_test: ERROR: sporadic thread creation failed: %d\n",
             ret);
    }

  ret = pthread_attr_setschedpolicy(&attr, SCHED_SPORADIC);
  if (ret != OK)
    {
      printf("sporadic_test: ERROR: pthread_attr_setschedpolicy failed, "
             "ret=%d\n",
             ret);
    }

  sparam.sched_priority               = PRIO_HIGH2;
  sparam.sched_ss_low_priority        = PRIO_LOW2;
  sparam.sched_ss_init_budget.tv_nsec = budget_2_ns;

  ret = pthread_attr_setschedparam(&attr, &sparam);
  if (ret != OK)
    {
      printf("sporadic_test: ERROR: pthread_attr_setsched param failed, "
             "ret=%d\n",
             ret);
    }

  ret = pthread_create(&sporadic_thread2, &attr, sporadic_func,
                       (pthread_addr_t)g_ms_cnt2);
  if (ret != 0)
    {
      printf("sporadic_test: ERROR: sporadic thread creation failed: %d\n",
             ret);
    }

  ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
  if (ret != OK)
    {
      printf("sporadic_test: ERROR: pthread_attr_setschedpolicy failed, "
             "ret=%d\n",
             ret);
    }

  g_start_time = time(NULL);

  sem_post(&g_sporadic_sem);
  sem_post(&g_sporadic_sem);

  sleep(100);

  ret = pthread_cancel(sporadic_thread1);
  pthread_join(sporadic_thread1, &result);

  ret = pthread_cancel(sporadic_thread2);
  pthread_join(sporadic_thread1, &result);

  sem_destroy(&g_sporadic_sem);
  ret = sched_setparam(0, &myparam);
  if (ret != OK)
    {
      printf("sporadic_test: ERROR: sched_setparam failed, ret=%d\n", ret);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int sporadic2_test(void)
{
  int32_t budget_1_ns;
  int32_t budget_2_ns = 30 * 1000000;
  int i;

  printf("Sporadic 1: prio high %d, low %d, repl %" PRIi32 " ns\n",
         PRIO_HIGH1, PRIO_LOW1, REPL_INTERVAL);
  printf("Sporadic 2: prio high %d, low %d, repl %" PRIi32 " ns\n",
         PRIO_HIGH2, PRIO_LOW2, REPL_INTERVAL);
  printf("\n%14s%10s%7s%7s\n", "THREAD", "BUDGET", "HI MS", "LO MS");

  for (budget_1_ns = 0, i = 1;
       budget_1_ns <= MAX_BUDGET;
       budget_1_ns += 10 * 1000000, i++)
    {
      sporadic_test_case(budget_1_ns, budget_2_ns);

      printf("%3d Sporadic 1 %09" PRIi32 " %6" PRIi32 " %6" PRIi32 "\n",
             i,
             budget_1_ns, g_ms_cnt1[PRIO_HI_NDX], g_ms_cnt1[PRIO_LO_NDX]);
      printf("    Sporadic 2 %09" PRIi32 " %6" PRIi32 " %6" PRIi32 "\n",
             budget_2_ns, g_ms_cnt2[PRIO_HI_NDX], g_ms_cnt2[PRIO_LO_NDX]);
    }

  return 0;
}

#endif /* CONFIG_SCHED_SPORADIC */
