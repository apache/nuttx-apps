/********************************************************************************
 * apps/testing/ostest/roundrobin.c
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
 ********************************************************************************/

/********************************************************************************
 * Included Files
 ********************************************************************************/

#include <nuttx/config.h>

#include <stdio.h>
#include <stdbool.h>
#include <semaphore.h>
#include <sched.h>

#include "ostest.h"

#if CONFIG_RR_INTERVAL > 0

/********************************************************************************
 * Pre-processor Definitions
 ********************************************************************************/

/* This numbers should be tuned for different processor speeds via .config file.
 * With default values the test takes about 30s on Cortex-M3 @ 24MHz. With 32767
 * range and 10 runs it takes ~320s.
 */

#ifndef CONFIG_TESTING_OSTEST_RR_RANGE
#  define CONFIG_TESTING_OSTEST_RR_RANGE 10000
#  warning "CONFIG_TESTING_OSTEST_RR_RANGE undefined, using default value = 10000"
#elif (CONFIG_TESTING_OSTEST_RR_RANGE < 1) || (CONFIG_TESTING_OSTEST_RR_RANGE > 32767)
#  define CONFIG_TESTING_OSTEST_RR_RANGE 10000
#  warning "Invalid value of CONFIG_TESTING_OSTEST_RR_RANGE, using default value = 10000"
#endif

#ifndef CONFIG_TESTING_OSTEST_RR_RUNS
#  define CONFIG_TESTING_OSTEST_RR_RUNS 10
#  warning "CONFIG_TESTING_OSTEST_RR_RUNS undefined, using default value = 10"
#elif (CONFIG_TESTING_OSTEST_RR_RUNS < 1) || (CONFIG_TESTING_OSTEST_RR_RUNS > 32767)
#  define CONFIG_TESTING_OSTEST_RR_RUNS 10
#  warning "Invalid value of CONFIG_TESTING_OSTEST_RR_RUNS, using default value = 10"
#endif

/********************************************************************************
 * Private Data
 ********************************************************************************/

static sem_t g_rrsem;

/********************************************************************************
 * Private Functions
 ********************************************************************************/

/********************************************************************************
 * Name: get_primes
 *
 * Description
 *   This function searches for prime numbers in the most primitive way possible.
 ********************************************************************************/

static void get_primes(int *count, int *last)
{
  int number;
  int local_count = 0;

  *last = 0;    /* To make the compiler happy */

  for (number = 1; number < CONFIG_TESTING_OSTEST_RR_RANGE; number++)
    {
      int div;
      bool is_prime = true;

      for (div = 2; div <= number / 2; div++)
      if (number % div == 0)
        {
          is_prime = false;
          break;
        }

      if (is_prime)
        {
          local_count++;
          *last = number;
#if 0 /* We don't really care what the numbers are */
          printf(" Prime %d: %d\n", local_count, number);
#endif
        }
    }

  *count = local_count;
}

/********************************************************************************
 * Name: get_primes_thread
 ********************************************************************************/

static FAR void *get_primes_thread(FAR void *parameter)
{
  int id = (int)((intptr_t)parameter);
  int count;
  int last;
  int i;

  while (sem_wait(&g_rrsem) < 0);

  printf("get_primes_thread id=%d started, "
         "looking for primes < %d, doing %d run(s)\n",
         id, CONFIG_TESTING_OSTEST_RR_RANGE, CONFIG_TESTING_OSTEST_RR_RUNS);

  for (i = 0; i < CONFIG_TESTING_OSTEST_RR_RUNS; i++)
    {
      get_primes(&count, &last);
    }

  printf("get_primes_thread id=%d finished, found %d primes, last one was %d\n",
         id, count, last);

  pthread_exit(NULL);
  return NULL; /* To keep some compilers happy */
}

/********************************************************************************
 * Public Functions
 ********************************************************************************/

/********************************************************************************
 * Name: rr_test
 ********************************************************************************/

void rr_test(void)
{
  pthread_t get_primes1_thread;
  pthread_t get_primes2_thread;
  struct sched_param sparam;
  pthread_attr_t attr;
  pthread_addr_t result;
  int status;

  /* Setup common thread attrributes */

  status = pthread_attr_init(&attr);
  if (status != OK)
    {
      printf("rr_test: ERROR: pthread_attr_init failed, status=%d\n",  status);
    }

  sparam.sched_priority = sched_get_priority_min(SCHED_FIFO);
  status = pthread_attr_setschedparam(&attr, &sparam);
  if (status != OK)
    {
      printf("rr_test: ERROR: pthread_attr_setschedparam failed, status=%d\n",
              status);
    }
  else
    {
      printf("rr_test: Set thread priority to %d\n",  sparam.sched_priority);
    }

  status = pthread_attr_setschedpolicy(&attr, SCHED_RR);
  if (status != OK)
    {
      printf("rr_test: ERROR: pthread_attr_setschedpolicy failed, status=%d\n",
              status);
    }
  else
    {
      printf("rr_test: Set thread policy to SCHED_RR\n");
    }

  /* This semaphore will prevent anything from running until we are ready */

  sched_lock();
  sem_init(&g_rrsem, 0, 0);
  sem_setprotocol(&g_rrsem, SEM_PRIO_NONE);

  /* Start the threads */

  printf("rr_test: Starting first get_primes_thread\n");

  status = pthread_create(&get_primes1_thread,
                          &attr, get_primes_thread, (FAR void *)1);
  if (status != 0)
    {
      printf("         ERROR: Thread 1 creation failed: %d\n",  status);
    }

  printf("         First get_primes_thread: %d\n", (int)get_primes1_thread);
  printf("rr_test: Starting second get_primes_thread\n");

  status = pthread_create(&get_primes2_thread,
                          &attr, get_primes_thread, (FAR void *)2);
  if (status != 0)
    {
      printf("         ERROR: Thread 2 creation failed: %d\n",  status);
    }

  printf("         Second get_primes_thread: %d\n", (int)get_primes2_thread);
  printf("rr_test: "
         "Waiting for threads to complete -- this should take awhile\n");
  printf("         "
         "If RR scheduling is working, they should start and complete at\n");
  printf("         about the same time\n");

  sem_post(&g_rrsem);
  sem_post(&g_rrsem);
  sched_unlock();

  pthread_join(get_primes2_thread, &result);
  pthread_join(get_primes1_thread, &result);
  printf("rr_test: Done\n");
  sem_destroy(&g_rrsem);
}

#endif /* CONFIG_RR_INTERVAL */
