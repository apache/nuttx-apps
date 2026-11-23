/****************************************************************************
 * apps/testing/ostest/prioinherit.c
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

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef CONFIG_SIM_WALLTIME_SLEEP
#  include <nuttx/arch.h>
#endif

#include <sys/wait.h>

#include "ostest.h"

#if defined(CONFIG_PRIORITY_INHERITANCE) && !defined(CONFIG_DISABLE_PTHREAD)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Priority inheritance is supported only for mutexes.  A mutex is held by
 * exactly one thread, so this test uses a single low priority holder.
 */

#define NLOWPRI_THREADS  1
#define NHIGHPRI_THREADS 1

#define NUMBER_OF_COMPETING_THREADS     3
#define COMPETING_THREAD_START_PRIORITY 200
#define PRIORIY_SPREED                  10

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum thstate_e
{
  NOTSTARTED = 0,
  RUNNING,
  WAITING,
  HOLDING,
  DONE
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static pthread_mutex_t g_mutex;
static volatile enum thstate_e g_middlestate;
static volatile enum thstate_e g_highstate[NHIGHPRI_THREADS];
static volatile enum thstate_e g_lowstate[NLOWPRI_THREADS];
static volatile int g_priority_tracking[NUMBER_OF_COMPETING_THREADS];
static int g_highpri;
static int g_medpri;
static int g_lowpri;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sleep_and_display
 ****************************************************************************/

static void sleep_and_display(int n, int us)
{
  struct sched_param sparam;

  us /= 100;

  do
    {
      int status = sched_getparam(gettid(), &sparam);

      if (status != 0)
        {
          printf("priority_inheritance: ERROR sched_getparam failed\n");
          ASSERT(false);
        }

      if (us == 0 || g_priority_tracking[n] != sparam.sched_priority)
        {
          if (g_priority_tracking[n] == 0)
            {
              printf("priority_inheritance: "
                     "Task%1d initial priority is:%d\n",
                     n, sparam.sched_priority);
            }
          else
            {
              printf("priority_inheritance: "
                     "Task%1d priority was:%d is:%d\n",
                     n, g_priority_tracking[n], sparam.sched_priority);
            }

          g_priority_tracking[n] = sparam.sched_priority;
          FFLUSH();
        }

      usleep(100);
      us--;
    }
  while (us > 0);
}

/****************************************************************************
 *
 ****************************************************************************/

static FAR void *adversary(FAR void *parameter)
{
  int index        = (uintptr_t)parameter;
  int inital_delay = index * 10000;
  int hold_delay   = index == 0 ? 100000 : 1000;

  sleep_and_display(index, inital_delay);
  printf("priority_inheritance: "
         "Task%1d Started, waiting %d uS to take the mutex\n", index,
         inital_delay);
  pthread_mutex_lock(&g_mutex);
  sleep_and_display(index,  hold_delay);
  pthread_mutex_unlock(&g_mutex);
  printf("priority_inheritance: Task%1d Released\n", index);
  sleep_and_display(index, 0);
  return NULL;
}

/****************************************************************************
 * Name: nhighpri_waiting
 ****************************************************************************/

static int nhighpri_waiting(void)
{
  int n = 0;
  int i;

  for (i = 0; i < NHIGHPRI_THREADS; i++)
    {
      if (g_highstate[i] == WAITING)
        {
          n++;
        }
    }

  return n;
}

/****************************************************************************
 * Name: nhighpri_running
 ****************************************************************************/

static int nhighpri_running(void)
{
  int n = 0;
  int i;

  for (i = 0; i < NHIGHPRI_THREADS; i++)
    {
      if (g_highstate[i] != DONE)
        {
          n++;
        }
    }

  return n;
}

/****************************************************************************
 * Name: highpri_thread
 ****************************************************************************/

static FAR void *highpri_thread(FAR void *parameter)
{
  int threadno = (int)((intptr_t)parameter);
  int ret;

  g_highstate[threadno - 1] = RUNNING;

  printf("highpri_thread-%d: Started\n", threadno);
  FFLUSH();
  sleep(1);

  printf("highpri_thread-%d: Calling pthread_mutex_lock()\n", threadno);
  g_highstate[threadno - 1] = WAITING;
  ret                       = pthread_mutex_lock(&g_mutex);
  g_highstate[threadno - 1] = DONE;

  if (ret != 0)
    {
      printf("highpri_thread-%d: "
             "ERROR pthread_mutex_lock failed: %d\n", threadno, ret);
      ASSERT(false);
    }
  else if (g_middlestate == RUNNING)
    {
      printf("highpri_thread-%d: SUCCESS midpri_thread is still running!\n",
             threadno);
    }
  else
    {
      printf("highpri_thread-%d: ERROR --  "
             "midpri_thread has already exited!\n",
             threadno);
      ASSERT(false);
    }

  pthread_mutex_unlock(&g_mutex);
  printf("highpri_thread-%d: Okay... I'm done!\n", threadno);
  FFLUSH();
  return NULL;
}

/****************************************************************************
 * Name: hog_cpu
 ****************************************************************************/

static inline void hog_cpu(void)
{
#ifdef CONFIG_SIM_WALLTIME_SLEEP
  /* The simulator doesn't have any mechanism to do asynchronous pre-emption
   * (basically because it doesn't have any interrupts/asynchronous events).
   * The simulator does "fake" a timer interrupt in up_idle() -- the idle
   * thread that only executes when nothing else is running.  In the
   * simulator, we cannot suspend the middle priority task, or we wouldn't
   * have the test that we want.  So, we have no option but to pump the fake
   * clock here by calling up_idle().  Sigh!
   */

  up_idle();
#else
  /* On real platforms with a real timer interrupt, we really can hog the
   * CPU.  When the sleep() goes off in priority_inheritance(), it will
   * wake up and start the high priority thread.
   */

  volatile int i;

  for (i = 0; i < INT_MAX; i++);
#endif
}

/****************************************************************************
 * Name: medpri_thread
 ****************************************************************************/

static FAR void *medpri_thread(FAR void *parameter)
{
  printf("medpri_thread: Started ... I won't let go of the CPU!\n");
  g_middlestate = RUNNING;
  FFLUSH();

  /* The following loop will completely block lowpri_thread from running.
   * UNLESS priority inheritance is working.  In that case, its priority
   * will be boosted.
   */

  while (nhighpri_running() > 0)
    {
      hog_cpu();
    }

  printf("medpri_thread: Okay... I'm done!\n");
  FFLUSH();
  g_middlestate = DONE;
  return NULL;
}

/****************************************************************************
 * Name: lowpri_thread
 ****************************************************************************/

static FAR void *lowpri_thread(FAR void *parameter)
{
  FAR void *retval = (FAR void *)-1;
  struct sched_param sparam;
  int threadno = (int)((intptr_t)parameter);
  int expected;
  int policy;
  int ret;
  int nwaiting = 0;
  int i;

  g_lowstate[threadno - 1] = RUNNING;
  printf("lowpri_thread-%d: Started\n", threadno);

  ret = pthread_getschedparam(pthread_self(), &policy, &sparam);
  if (ret != 0)
    {
      printf("lowpri_thread-%d: ERROR pthread_getschedparam failed: %d\n",
             threadno, ret);
      ASSERT(false);
    }
  else
    {
      printf("lowpri_thread-%d: initial priority: %d\n", threadno,
          sparam.sched_priority);
      if (sparam.sched_priority != g_lowpri)
        {
          printf("               ERROR should have been %d\n", g_lowpri);
          ASSERT(false);
        }
    }

  g_lowstate[threadno - 1] = WAITING;
  ret = pthread_mutex_lock(&g_mutex);
  if (ret != 0)
    {
      printf("lowpri_thread-%d: ERROR pthread_mutex_lock failed: %d\n",
             threadno, ret);
      ASSERT(false);
    }
  else
    {
      /* The mutex is now held by this low priority thread.  Publish the
       * HOLDING state so that priority_inheritance() only starts the middle
       * priority thread once the mutex is actually owned.
       */

      g_lowstate[threadno - 1] = HOLDING;

      /* Hold the mutex until the middle priority thread runs and the high
       * priority thread is waiting on it.  If priority inheritance does not
       * work, the middle priority thread keeps this holder from running and
       * the test hangs.
       */

      while (g_middlestate == NOTSTARTED &&
             nhighpri_waiting() < NHIGHPRI_THREADS)
        {
          printf("lowpri_thread-%d: "
                 "Waiting for the middle pri task to run\n", threadno);
          printf("    g_middlestate:  %d\n", (int)g_middlestate);
          for (i = 0; i < NHIGHPRI_THREADS; i++)
            {
              printf("    g_highstate[%d]: %d\n", i, (int)g_highstate[i]);
            }

          printf("    I still hold the mutex\n");
          FFLUSH();
          sleep(1);
        }

      sched_lock(); /* Needs to be atomic */
      nwaiting = nhighpri_waiting();
      sched_unlock();

      printf("lowpri_thread-%d: No. highpri thread waiting: %d\n",
             threadno, nwaiting);

      /* We only get here because priority inheritance boosted this holder
       * above the middle priority thread, so that thread must still be
       * running.
       */

      if (g_middlestate == RUNNING)
        {
          retval = NULL;
        }
      else
        {
          printf("lowpri_thread-%d: ERROR the middle priority task has "
                 "already exited!\n", threadno);
          printf("               g_middlestate:  %d\n",
                 (int)g_middlestate);
          for (i = 0; i < NHIGHPRI_THREADS; i++)
            {
              printf("               g_highstate[%d]: %d\n",
                     i, (int)g_highstate[i]);
            }

          ASSERT(false);
        }
    }

  ret = pthread_getschedparam(pthread_self(), &policy, &sparam);
  pthread_mutex_unlock(&g_mutex);
  if (ret != 0)
    {
      printf("lowpri_thread-%d: ERROR pthread_getschedparam failed: %d\n",
             threadno, ret);
      ASSERT(false);
    }
  else
    {
      if (nwaiting > 0)
        {
          expected = g_highpri;
        }
      else
        {
          expected = g_lowpri;
        }

      printf("lowpri_thread-%d: %s priority before unlock: %d\n",
             threadno,
             sparam.sched_priority != expected ? "ERROR" : "SUCCESS",
             sparam.sched_priority);

      if (sparam.sched_priority != expected)
        {
          printf("               ERROR should have been %d\n", expected);
          ASSERT(false);
        }
    }

  ret = pthread_getschedparam(pthread_self(), &policy, &sparam);
  if (ret != 0)
    {
      printf("lowpri_thread-%d: ERROR pthread_getschedparam failed: %d\n",
             threadno, ret);
      ASSERT(false);
    }
  else
    {
      printf("lowpri_thread-%d: %s final priority: %d\n",
             threadno,
             sparam.sched_priority != g_lowpri ? "ERROR" : "SUCCESS",
             sparam.sched_priority);

      if (sparam.sched_priority != g_lowpri)
        {
          printf("               ERROR should have been %d\n", g_lowpri);
          ASSERT(false);
        }
    }

  printf("lowpri_thread-%d: Okay... I'm done!\n", threadno);
  FFLUSH();
  g_lowstate[threadno - 1] = DONE;
  return retval;
}
#endif /* CONFIG_PRIORITY_INHERITANCE && !CONFIG_DISABLE_PTHREAD */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: priority_inheritance
 ****************************************************************************/

void priority_inheritance(void)
{
#if defined(CONFIG_PRIORITY_INHERITANCE) && !defined(CONFIG_DISABLE_PTHREAD)
  pthread_t lowpri[NLOWPRI_THREADS];
  pthread_t medpri;
  pthread_t highpri[NHIGHPRI_THREADS];
  pthread_addr_t result;
  pthread_attr_t attr;
  pthread_mutexattr_t mattr;
  struct sched_param sparam;
  int my_pri;
  int status;
  int i;
  pthread_t competing[NUMBER_OF_COMPETING_THREADS];
  int priority;
  int restoration_result;

  printf("priority_inheritance: Started\n");

  g_middlestate = NOTSTARTED;
  for (i = 0; i < NHIGHPRI_THREADS; i++)
    {
      g_highstate[i] = NOTSTARTED;
    }

  for (i = 0; i < NLOWPRI_THREADS; i++)
    {
      g_lowstate[i] = NOTSTARTED;
    }

  status = sched_getparam(gettid(), &sparam);
  if (status != 0)
    {
      printf("priority_inheritance: ERROR sched_getparam failed\n");
      ASSERT(false);
      sparam.sched_priority = PTHREAD_DEFAULT_PRIORITY;
    }

  my_pri = sparam.sched_priority;

  g_highpri = sched_get_priority_max(SCHED_FIFO);
  g_lowpri = sched_get_priority_min(SCHED_FIFO);
  g_medpri = my_pri - 1;

  /* Create the mutex used by the test with priority inheritance enabled */

  pthread_mutexattr_init(&mattr);
  pthread_mutexattr_setprotocol(&mattr, PTHREAD_PRIO_INHERIT);
  status = pthread_mutex_init(&g_mutex, &mattr);
  if (status != 0)
    {
      printf("priority_inheritance: "
             "ERROR pthread_mutex_init failed, status=%d\n", status);
      ASSERT(false);
    }

  /* Start the low priority threads */

  for (i = 0; i < NLOWPRI_THREADS; i++)
    {
      int threadno = i + 1;

      printf("priority_inheritance: "
             "Starting lowpri_thread-%d (of %d) at %d\n",
             threadno, NLOWPRI_THREADS, g_lowpri);
      status = pthread_attr_init(&attr);
      if (status != 0)
        {
          printf("priority_inheritance: "
                 "ERROR pthread_attr_init failed, status=%d\n",
                 status);
          ASSERT(false);
        }

      sparam.sched_priority = g_lowpri;
      status = pthread_attr_setschedparam(&attr, &sparam);
      if (status != OK)
        {
          printf("priority_inheritance: "
                 "ERROR pthread_attr_setschedparam failed, status=%d\n",
                 status);
          ASSERT(false);
        }
      else
        {
          printf("priority_inheritance: "
                 "Set lowpri_thread-%d priority to %d\n",
                 threadno, sparam.sched_priority);
        }

      status = pthread_create(&lowpri[i], &attr, lowpri_thread,
                              (FAR void *)((uintptr_t)threadno));
      if (status != 0)
        {
          printf("priority_inheritance: "
                 "ERROR pthread_create failed, status=%d\n", status);
          ASSERT(false);
        }
    }

  /* Wait until the low priority thread holds the mutex before starting
   * the middle priority thread, so that no fixed delay decides whether the
   * priority inheritance relationship is set up.
   */

  printf("priority_inheritance: Waiting for lowpri_thread to take the "
         "mutex\n");
  for (i = 0; i < NLOWPRI_THREADS; i++)
    {
      while (g_lowstate[i] < HOLDING)
        {
          usleep(1000);
        }
    }

  /* Start the medium priority thread */

  printf("priority_inheritance: Starting medpri_thread at %d\n", g_medpri);
  status = pthread_attr_init(&attr);
  if (status != 0)
    {
      printf("priority_inheritance: "
             "ERROR pthread_attr_init failed, status=%d\n", status);
      ASSERT(false);
    }

  sparam.sched_priority = g_medpri;
  status = pthread_attr_setschedparam(&attr, &sparam);
  if (status != OK)
    {
      printf("priority_inheritance: "
             "ERROR pthread_attr_setschedparam failed, status=%d\n",
              status);
      ASSERT(false);
    }
  else
    {
      printf("priority_inheritance: Set medpri_thread priority to %d\n",
             sparam.sched_priority);
    }

  FFLUSH();

  status = pthread_create(&medpri, &attr, medpri_thread, NULL);
  if (status != 0)
    {
      printf("priority_inheritance: "
             "ERROR pthread_create failed, status=%d\n",
             status);
      ASSERT(false);
    }

  printf("priority_inheritance: Waiting...\n");
  sleep(1);

  /* Start the high priority threads */

  for (i = 0; i < NHIGHPRI_THREADS; i++)
    {
      int threadno = i + 1;

      printf("priority_inheritance: "
             "Starting highpri_thread-%d (of %d) at %d\n",
             threadno, NHIGHPRI_THREADS, g_highpri);
      status = pthread_attr_init(&attr);
      if (status != 0)
        {
          printf("priority_inheritance: "
                 "ERROR pthread_attr_init failed, status=%d\n", status);
          ASSERT(false);
        }

      sparam.sched_priority = g_highpri - i;
      status = pthread_attr_setschedparam(&attr, &sparam);
      if (status != OK)
        {
          printf("priority_inheritance: "
                 "ERROR pthread_attr_setschedparam failed, status=%d\n",
                 status);
          ASSERT(false);
        }
      else
        {
          printf("priority_inheritance: "
                 "Set highpri_thread-%d priority to %d\n",
                 threadno, sparam.sched_priority);
        }

      FFLUSH();

      status = pthread_create(&highpri[i], &attr, highpri_thread,
                              (FAR void *)((uintptr_t)threadno));
      if (status != 0)
        {
          printf("priority_inheritance: "
                 "ERROR pthread_create failed, status=%d\n", status);
          ASSERT(false);
        }
    }

  FFLUSH();

  /* Wait for all thread instances to complete */

  for (i = 0; i < NHIGHPRI_THREADS; i++)
    {
      printf("priority_inheritance: "
             "Waiting for highpri_thread-%d to complete\n", i + 1);
      FFLUSH();
      pthread_join(highpri[i], &result);
    }

  printf("priority_inheritance: Waiting for medpri_thread to complete\n");
  FFLUSH();
  pthread_join(medpri, &result);

  for (i = 0; i < NLOWPRI_THREADS; i++)
    {
      printf("priority_inheritance: "
             "Waiting for lowpri_thread-%d to complete\n", i + 1);
      FFLUSH();
      pthread_join(lowpri[i], &result);
    }

  /* Perform restoration test */

  printf("priority_inheritance: Restoration Test:\n");

  /* The competing threads are threads, not tasks: a task of its own
   * would not share g_mutex and g_priority_tracking in a kernel build.
   */

  priority = COMPETING_THREAD_START_PRIORITY;

  for (i = 0; i < NUMBER_OF_COMPETING_THREADS; i++)
    {
      g_priority_tracking[i] = 0;

      status = pthread_attr_init(&attr);
      sparam.sched_priority = priority;
      status |= pthread_attr_setschedparam(&attr, &sparam);
      status |= pthread_create(&competing[i], &attr, adversary,
                               (FAR void *)((uintptr_t)i));
      if (status != 0)
        {
          printf("priority_inheritance: "
                 "ERROR Task%1d not started, status=%d\n", i, status);
          ASSERT(false);
        }

      priority += PRIORIY_SPREED;
    }

  priority = COMPETING_THREAD_START_PRIORITY;
  restoration_result = 0;
  for (i = 0; i < NUMBER_OF_COMPETING_THREADS; i++)
    {
      printf("priority_inheritance: "
             "Waiting for Task-%d to complete\n", i);

      pthread_join(competing[i], &result);
      if (priority != g_priority_tracking[i])
        {
          printf("priority_inheritance: "
                 "Task-%d Priority is %d, and was not restored to %d\n",
                  i, g_priority_tracking[i], priority);

          restoration_result |= 1 << i;
        }

      priority += PRIORIY_SPREED;
    }

  if (restoration_result != 0)
    {
      printf("priority_inheritance: ERROR: FAIL Priorities were not "
             "correctly restored.\n");
      ASSERT(false);
    }
  else
    {
      printf("priority_inheritance: PASSED Priority were correctly"
             " restored.\n");
    }

  printf("priority_inheritance: Finished\n");
  pthread_mutex_destroy(&g_mutex);
  FFLUSH();
#endif /* CONFIG_PRIORITY_INHERITANCE && !CONFIG_DISABLE_PTHREAD */
}
