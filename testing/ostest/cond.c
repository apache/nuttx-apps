/****************************************************************************
 * apps/testing/ostest/cond.c
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

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "ostest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

static volatile enum
{
  RUNNING,
  MUTEX_WAIT,
  COND_WAIT
} waiter_state;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static pthread_mutex_t mutex;
static pthread_cond_t  cond;
static volatile int data_available = 0;
static int waiter_nloops = 0;
static int waiter_waits = 0;
static int waiter_nerrors = 0;
static int signaler_nloops = 0;
static int signaler_already = 0;
static int signaler_state = 0;
static int signaler_nerrors = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void *thread_waiter(void *parameter)
{
  int status;

  printf("waiter_thread: Started\n");

  for (; ; )
    {
      /* Take the mutex */

      waiter_state = MUTEX_WAIT;
      status       = pthread_mutex_lock(&mutex);
      waiter_state = RUNNING;

      if (status != 0)
        {
          printf("waiter_thread: "
                 "ERROR pthread_mutex_lock failed, status=%d\n", status);
          waiter_nerrors++;
        }

      /* Check if data is available -- if data is not available then
       * wait for it
       */

      if (!data_available)
        {
          /* We are higher priority than the signaler thread so the
           * only time that the signaler thread will have a chance to run
           * is when we are waiting for the condition variable.
           * In this case, pthread_cond_wait will automatically release
           * the mutex for the signaler (then re-acquire the mutex before
           * returning.
           */

          waiter_state = COND_WAIT;
          status       = pthread_cond_wait(&cond, &mutex);
          waiter_state = RUNNING;

          if (status != 0)
            {
              printf("waiter_thread: "
                     "ERROR pthread_cond_wait failed, status=%d\n",
                     status);
              waiter_nerrors++;
            }

          waiter_waits++;
        }

      /* Now data should be available */

      if (!data_available)
        {
          printf("waiter_thread: ERROR data not available after wait\n");
          waiter_nerrors++;
        }

      /* Clear data available */

      data_available = 0;

      /* Release the mutex */

      status = pthread_mutex_unlock(&mutex);
      if (status != 0)
        {
          printf("waiter_thread: ERROR waiter: "
                 "pthread_mutex_unlock failed, status=%d\n", status);
          waiter_nerrors++;
        }

      waiter_nloops++;
    }

  return NULL;
}

static void *thread_signaler(void *parameter)
{
  int status;
  int i;

  printf("thread_signaler: Started\n");
  for (i = 0; i < 32; i++)
    {
      /* Take the mutex.  The waiter is higher priority and should
       * run until it waits for the condition.  So, at this point
       * signaler should be waiting for the condition.
       */

      status = pthread_mutex_lock(&mutex);
      if (status != 0)
        {
          printf("thread_signaler: "
                 "ERROR pthread_mutex_lock failed, status=%d\n", status);
          signaler_nerrors++;
        }

      /* Verify the state */

      if (waiter_state != COND_WAIT)
        {
          printf("thread_signaler: "
                 "ERROR waiter state = %d != COND_WAITING\n", waiter_state);
          signaler_state++;
        }

      if (data_available)
        {
          printf("thread_signaler: "
                 "ERROR data already available, waiter_state=%d\n",
                  waiter_state);
          signaler_already++;
        }

      /* Set data available and signal the waiter */

      data_available = 1;
      status = pthread_cond_signal(&cond);
      if (status != 0)
        {
          printf("thread_signaler: "
                 "ERROR pthread_cond_signal failed, status=%d\n", status);
          signaler_nerrors++;
        }

      /* Release the mutex */

      status = pthread_mutex_unlock(&mutex);
      if (status != 0)
        {
          printf("thread_signaler: "
                 "ERROR pthread_mutex_unlock failed, status=%d\n", status);
          signaler_nerrors++;
        }

#if defined(CONFIG_SMP) && (CONFIG_SMP_NCPUS > 1)
      /* Workaround for SMP:
       * In multi-core environment, thread_signaler would be excecuted prior
       * to the thread_waiter, even though priority of thread_signaler is
       * lower than the thread_waiter. In this case, thread_signaler will
       * aquire mutex before the thread_waiter aquires it and will show
       * the error message such as "thread_signaler: ERROR waiter state...".
       * To avoid this situaltion, we add the following usleep()
       */

      usleep(10 * 1000);
#endif

      signaler_nloops++;
    }

  printf("thread_signaler: Terminating\n");
  pthread_exit(NULL);
  return NULL; /* Non-reachable -- needed for some compilers */
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void cond_test(void)
{
  pthread_t waiter;
  pthread_t signaler;
  pthread_attr_t attr;
#ifdef SDCC
  pthread_addr_t result;
#endif
  struct sched_param sparam;
  int prio_min;
  int prio_max;
  int prio_mid;
  int status;

  /* Initialize the mutex */

  printf("cond_test: Initializing mutex\n");
  status = pthread_mutex_init(&mutex, NULL);
  if (status != 0)
    {
      printf("cond_test: "
             "ERROR pthread_mutex_init failed, status=%d\n", status);
    }

  /* Initialize the condition variable */

  printf("cond_test: Initializing cond\n");
  status = pthread_cond_init(&cond, NULL);
  if (status != 0)
    {
      printf("cond_test: "
             "ERROR pthread_condinit failed, status=%d\n", status);
    }

  /* Start the waiter thread at higher priority */

  printf("cond_test: Starting waiter\n");
  status = pthread_attr_init(&attr);
  if (status != 0)
    {
      printf("cond_test: pthread_attr_init failed, status=%d\n", status);
    }

  prio_min = sched_get_priority_min(SCHED_FIFO);
  prio_max = sched_get_priority_max(SCHED_FIFO);
  prio_mid = (prio_min + prio_max) / 2;

  sparam.sched_priority = prio_mid;
  status = pthread_attr_setschedparam(&attr, &sparam);
  if (status != OK)
    {
      printf("cond_test: "
             "pthread_attr_setschedparam failed, status=%d\n", status);
    }
  else
    {
      printf("cond_test: Set thread 1 priority to %d\n",
              sparam.sched_priority);
    }

  status = pthread_create(&waiter, &attr, thread_waiter, NULL);
  if (status != 0)
    {
      printf("cond_test: pthread_create failed, status=%d\n", status);
    }

  printf("cond_test: Starting signaler\n");
  status = pthread_attr_init(&attr);
  if (status != 0)
    {
      printf("cond_test: pthread_attr_init failed, status=%d\n", status);
    }

  sparam.sched_priority = (prio_min + prio_mid) / 2;
  status = pthread_attr_setschedparam(&attr, &sparam);
  if (status != OK)
    {
      printf("cond_test: pthread_attr_setschedparam failed, status=%d\n",
              status);
    }
  else
    {
      printf("cond_test: Set thread 2 priority to %d\n",
              sparam.sched_priority);
    }

  status = pthread_create(&signaler, &attr, thread_signaler, NULL);
  if (status != 0)
    {
      printf("cond_test: pthread_create failed, status=%d\n", status);
    }

  /* Wait for the threads to stop */

#ifdef SDCC
  pthread_join(signaler, &result);
#else
  pthread_join(signaler, NULL);
#endif
  printf("cond_test: signaler terminated, now cancel the waiter\n");
  pthread_detach(waiter);
  pthread_cancel(waiter);

  printf("cond_test: \tWaiter\tSignaler\n");
  printf("cond_test: Loops\t%d\t%d\n", waiter_nloops, signaler_nloops);
  printf("cond_test: Errors\t%d\t%d\n", waiter_nerrors, signaler_nerrors);
  printf("cond_test:\n");
  printf("cond_test: %d times, waiter did not have to wait for data\n",
          waiter_nloops - waiter_waits);
  printf("cond_test: %d times, "
         "data was already available when the signaler run\n",
          signaler_already);
  printf("cond_test: %d times, "
         "the waiter was in an unexpected state when the signaler ran\n",
         signaler_state);
}
