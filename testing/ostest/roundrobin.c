/****************************************************************************
 * apps/testing/ostest/roundrobin.c
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
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "ostest.h"

#if CONFIG_RR_INTERVAL > 0

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Each thread searches for prime numbers below RR_RANGE, one number at a
 * time, and counts its time slices.  The test ends when the two threads
 * have RR_RUNS time slices each.
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

/****************************************************************************
 * Private Data
 ****************************************************************************/

static volatile int g_rr_last;
static volatile int g_rr_slices[2];
static volatile bool g_rr_stop;
static volatile unsigned int g_rr_primes[2];
static sem_t g_rrsem;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: is_prime
 *
 * Description
 *   This function tests a number for a prime in the most primitive
 *   way possible.
 ****************************************************************************/

static bool is_prime(int number)
{
  int div;

  for (div = 2; div <= number / 2; div++)
    {
      if (number % div == 0)
        {
          return false;
        }
    }

  return true;
}

/****************************************************************************
 * Name: get_primes_thread
 *
 * Description
 *   Search for primes until the two threads have RR_RUNS time slices each.
 *   A thread starts a new time slice when the other thread ran after its
 *   last number.
 ****************************************************************************/

static FAR void *get_primes_thread(FAR void *parameter)
{
  int id = (int)((intptr_t)parameter);
  int number = 1;

  while (sem_wait(&g_rrsem) < 0);

  printf("get_primes_thread id=%d started, "
         "looking for primes < %d, running %d time slice(s)\n",
         id, CONFIG_TESTING_OSTEST_RR_RANGE, CONFIG_TESTING_OSTEST_RR_RUNS);

  while (!g_rr_stop &&
         g_rr_slices[0] + g_rr_slices[1] < 2 * CONFIG_TESTING_OSTEST_RR_RUNS)
    {
      if (is_prime(number))
        {
          g_rr_primes[id - 1]++;
        }

      number = number % CONFIG_TESTING_OSTEST_RR_RANGE + 1;

      if (g_rr_last != id)
        {
          g_rr_last = id;
          g_rr_slices[id - 1]++;
        }
    }

  printf("get_primes_thread id=%d finished, %d time slice(s), "
         "found %u primes\n", id, g_rr_slices[id - 1], g_rr_primes[id - 1]);

  pthread_exit(NULL);
  return NULL; /* To keep some compilers happy */
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: rr_test
 ****************************************************************************/

void rr_test(void)
{
  pthread_t get_primes1_thread;
  pthread_t get_primes2_thread;
  struct sched_param sparam;
  pthread_attr_t attr;
  pthread_addr_t result;
#ifdef CONFIG_SMP
  cpu_set_t cpuset;
#endif
  int status;
  int diff;
  int i;

  /* Setup common thread attributes */

  status = pthread_attr_init(&attr);
  if (status != OK)
    {
      printf("rr_test: ERROR: pthread_attr_init failed, status=%d\n",
             status);
      ASSERT(false);
    }

  sparam.sched_priority = sched_get_priority_min(SCHED_FIFO);
  status = pthread_attr_setschedparam(&attr, &sparam);
  if (status != OK)
    {
      printf("rr_test: ERROR: pthread_attr_setschedparam failed,"
             " status=%d\n", status);
      ASSERT(false);
    }
  else
    {
      printf("rr_test: Set thread priority to %d\n",
             sparam.sched_priority);
    }

  status = pthread_attr_setschedpolicy(&attr, SCHED_RR);
  if (status != OK)
    {
      printf("rr_test: ERROR: pthread_attr_setschedpolicy failed,"
             " status=%d\n", status);
      ASSERT(false);
    }
  else
    {
      printf("rr_test: Set thread policy to SCHED_RR\n");
    }

#ifdef CONFIG_SMP
  /* RR case on SMP only run on core0 */

  CPU_ZERO(&cpuset);
  CPU_SET(0, &cpuset);
  status = pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);
  if (status != OK)
    {
      printf("rr_test: ERROR: pthread_attr_setaffinity_np failed,"
             " status=%d\n", status);
      ASSERT(false);
    }
#endif

  /* This semaphore will prevent anything from running until we are ready */

  sched_lock();
  g_rr_last      = 0;
  g_rr_slices[0] = 0;
  g_rr_slices[1] = 0;
  g_rr_primes[0] = 0;
  g_rr_primes[1] = 0;
  g_rr_stop      = false;
  sem_init(&g_rrsem, 0, 0);

  /* Start the threads */

  printf("rr_test: Starting first get_primes_thread\n");

  status = pthread_create(&get_primes1_thread,
                          &attr, get_primes_thread, (FAR void *)1);
  if (status != 0)
    {
      printf("         ERROR: Thread 1 creation failed: %d\n",  status);
      ASSERT(false);
    }

  printf("         First get_primes_thread: %d\n", (int)get_primes1_thread);
  printf("rr_test: Starting second get_primes_thread\n");

  status = pthread_create(&get_primes2_thread,
                          &attr, get_primes_thread, (FAR void *)2);
  if (status != 0)
    {
      printf("         ERROR: Thread 2 creation failed: %d\n", status);
      ASSERT(false);
    }

  printf("         Second get_primes_thread: %d\n", (int)get_primes2_thread);
  printf("rr_test: Waiting for %d time slices of each thread\n",
         CONFIG_TESTING_OSTEST_RR_RUNS);
  printf("         If RR scheduling is working, the threads run the same\n");
  printf("         number of time slices\n");

  sem_post(&g_rrsem);
  sem_post(&g_rrsem);
  sched_unlock();

  /* This thread has a higher priority.  Stop the threads if RR scheduling
   * does not give them their time slices: one thread would run forever.
   */

  for (i = 0; i < 20 * CONFIG_TESTING_OSTEST_RR_RUNS; i++)
    {
      if (g_rr_slices[0] + g_rr_slices[1] >=
          2 * CONFIG_TESTING_OSTEST_RR_RUNS)
        {
          break;
        }

      usleep(CONFIG_RR_INTERVAL * 1000);
    }

  g_rr_stop = true;

  pthread_join(get_primes2_thread, &result);
  pthread_join(get_primes1_thread, &result);

  diff = g_rr_slices[0] - g_rr_slices[1];
  printf("rr_test: thread 1 ran %d time slices, thread 2 ran %d\n",
         g_rr_slices[0], g_rr_slices[1]);

  if (g_rr_slices[0] + g_rr_slices[1] >= 2 * CONFIG_TESTING_OSTEST_RR_RUNS &&
      diff >= -1 && diff <= 1)
    {
      printf("rr_test: Done\n");
    }
  else
    {
      printf("rr_test: Roundrobin Failed\n");
      ASSERT(false);
    }

  sem_destroy(&g_rrsem);
}

#endif /* CONFIG_RR_INTERVAL */
