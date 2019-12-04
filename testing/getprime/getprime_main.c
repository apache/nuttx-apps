/****************************************************************************
 * apps/testing/getprime_main.c
 *
 *   Copyright 2019 Sony Home Entertainment & Sound Products Inc.
 *   Author: Masayuki Ishikawa <Masayuki.Ishikawa@jp.sony.com>
 *
 * Based on apps/testing/ostest/roundrobin.c
 *
 *   Copyright (C) 2016 Gregory Nutt. All rights reserved.
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
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
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

  pthread_exit(NULL);
  return NULL; /* To keep some compilers happy */
}

/****************************************************************************
 * Name: get_prime_in_parallel
 ****************************************************************************/

static void get_prime_in_parallel(int n)
{
  pthread_t thread[MAX_THREADS];
  struct sched_param sparam;
  pthread_attr_t attr;
  pthread_addr_t result;
  int status;
  int i;

  status = pthread_attr_init(&attr);
  ASSERT(status == OK);

  sparam.sched_priority = sched_get_priority_min(SCHED_FIFO);
  status = pthread_attr_setschedparam(&attr, &sparam);
  ASSERT(status == OK);

  printf("Set thread priority to %d\n",  sparam.sched_priority);

  status = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
  ASSERT(status == OK);

  printf("Set thread policy to SCHED_FIFO \n");

  for (i = 0; i < n; i++)
    {
      printf("Start thread #%d \n", i);
      status = pthread_create(&thread[i], &attr,
                              thread_func, (FAR void *)&i);
      ASSERT(status == OK);
    }

  /* Wait for finishing the last thread */

  pthread_join(thread[n - 1], &result);

  printf("Done\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * smp_main
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
      ASSERT(argv[1] != endp);

      ASSERT(0 < n && n <= MAX_THREADS);
    }

  (void)clock_gettime(CLOCK_REALTIME, &ts0);
  get_prime_in_parallel(n);
  (void)clock_gettime(CLOCK_REALTIME, &ts1);

  elapsed  = (((uint64_t)ts1.tv_sec * NSEC_PER_SEC) + ts1.tv_nsec);
  elapsed -= (((uint64_t)ts0.tv_sec * NSEC_PER_SEC) + ts0.tv_nsec);
  elapsed /= NSEC_PER_MSEC; /* msec */

  printf("%s took %ld msec \n", argv[0], elapsed);
  return 0;
}
