/****************************************************************************
 * apps/testing/sched/getprime/getprime_main.c
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
#include <nuttx/clock.h>

#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define PRIME_RANGE 10000
#define PRIME_RUNS  10
#define MAX_THREADS 8

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: get_primes
 ****************************************************************************/

static void get_primes(int *count, int *last)
{
  int num;
  int found = 0;

  *last = 0;

  for (num = 1; num < PRIME_RANGE; num++)
    {
      int div;
      bool is_prime = true;

      for (div = 2; div <= num / 2; div++)
        {
          if (num % div == 0)
            {
              is_prime = false;
              break;
            }
        }

      if (is_prime)
        {
          found++;
          *last = num;
        }
    }

  *count = found;
}

/****************************************************************************
 * Name: thread_func
 ****************************************************************************/

static FAR void *thread_func(FAR void *param)
{
  int no = *(int *)param;
  int count;
  int last;
  int i;

  printf("thread #%d started, looking for primes < %d, doing %d run(s)\n",
         no, PRIME_RANGE, PRIME_RUNS);

  for (i = 0; i < PRIME_RUNS; i++)
    {
      get_primes(&count, &last);
    }

  printf("thread #%d finished, found %d primes, last one was %d\n",
         no, count, last);

  return NULL;
}

/****************************************************************************
 * Name: get_prime_in_parallel
 ****************************************************************************/

static void get_prime_in_parallel(int n)
{
  pthread_t thread[MAX_THREADS];
  pthread_attr_t attr;
  pthread_addr_t result;
  int arg[MAX_THREADS];
  int status;
  int i;

  status = pthread_attr_init(&attr);
  ASSERT(status == OK);

  struct sched_param sparam;
  sparam.sched_priority = CONFIG_TESTING_GETPRIME_THREAD_PRIORITY;
  status = pthread_attr_setschedparam(&attr, &sparam);
  ASSERT(status == OK);

  printf("Set thread priority to %d\n",  sparam.sched_priority);

#if CONFIG_RR_INTERVAL > 0
  status = pthread_attr_setschedpolicy(&attr, SCHED_RR);
  ASSERT(status == OK);

  printf("Set thread policy to SCHED_RR\n");
#else
  status = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
  ASSERT(status == OK);

  printf("Set thread policy to SCHED_FIFO\n");
#endif

  /* Zero threads, do work from main thread instead. */

  if (n == 0)
    {
        arg[0] = n;
        thread_func((FAR void *)&arg[0]);
    }

  for (i = 0; i < n; i++)
    {
      arg[i] = i;
      printf("Start thread #%d\n", i);
      status = pthread_create(&thread[i], &attr,
                              thread_func, (FAR void *)&arg[i]);
      ASSERT(status == OK);
    }

  /* Wait for all the threads to finish */

  for (i = 0; i < n; i++)
    {
      pthread_join(thread[i], &result);
    }

  printf("Done\n");
  UNUSED(status);
}

/****************************************************************************
 * Name: usage
 ****************************************************************************/

static void usage(void)
{
    printf("\nUsage: getprime [<num_threads>]\n");
    printf("\nWhere:\n");
    printf("  <num_threads> is integer from 0 to %d (default 1).\n",
           MAX_THREADS);
    exit(0);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * getprime_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct timespec ts0;
  struct timespec ts1;
  uint64_t elapsed;
  char *endp;
  int n = 1;

  if (argc == 2)
    {
      n = (int)strtol(argv[1], &endp, 10);
      if (argv[1] == endp || n < 0 || n > MAX_THREADS)
        {
            usage();
        }
    }
  else if (argc > 2)
    {
      usage();
    }

  clock_gettime(CLOCK_REALTIME, &ts0);
  get_prime_in_parallel(n);
  clock_gettime(CLOCK_REALTIME, &ts1);

  elapsed  = (((uint64_t)ts1.tv_sec * NSEC_PER_SEC) + ts1.tv_nsec);
  elapsed -= (((uint64_t)ts0.tv_sec * NSEC_PER_SEC) + ts0.tv_nsec);
  elapsed /= NSEC_PER_MSEC; /* msec */

  printf("%s took %" PRIu64 " msec\n", argv[0], elapsed);
  return 0;
}
