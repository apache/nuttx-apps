/****************************************************************************
 * apps/testing/sched/pthread_mutex_perf/pthread_mutex_perf.c
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
#include <stdio.h>
#include <pthread.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

static void timespec_diff(const struct timespec *start,
                          const struct timespec *end,
                          struct timespec *diff)
{
  diff->tv_sec = end->tv_sec - start->tv_sec;
  diff->tv_nsec = end->tv_nsec - start->tv_nsec;

  if (diff->tv_nsec < 0)
    {
      diff->tv_sec--;
        diff->tv_nsec += 1000000000;
    }
}

static void timespec_add(struct timespec *total, const struct timespec *diff)
{
  total->tv_sec += diff->tv_sec;
  total->tv_nsec += diff->tv_nsec;

  if (total->tv_nsec >= 1000000000)
    {
      total->tv_sec += total->tv_nsec / 1000000000;
      total->tv_nsec = total->tv_nsec % 1000000000;
    }
}

static void timespec_avg(const struct timespec *total, int count,
                         struct timespec *avg)
{
  uint64_t total_ns = (uint64_t)total->tv_sec * 1000000000 + total->tv_nsec;
  uint64_t avg_ns = total_ns / count;

  avg->tv_sec = avg_ns / 1000000000;
  avg->tv_nsec = avg_ns % 1000000000;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * pmp_main
 ****************************************************************************/

int main(int argc, char *argv[])
{
  struct timespec start;
  struct timespec end;
  struct timespec diff;
  struct timespec total = {
                            0, 0
                          };

  struct timespec avg;
  int i;
  int j = 0;
  const int loop_count = 10;

  while (j < loop_count)
    {
      i = 0;
      j++;

      /* Get the starting time */

      clock_gettime(CLOCK_BOOTTIME, &start);

      /* Do 1 million interactions trying to lock an already locked mutex */

      pthread_mutex_lock(&g_mutex);

      while (i < 1000 * 1000)
        {
          i++;
          pthread_mutex_trylock(&g_mutex);
        }

      pthread_mutex_unlock(&g_mutex);

      /* Get the finished time */

      clock_gettime(CLOCK_BOOTTIME, &end);

      /* Get the calculated elapsed time */

      timespec_diff(&start, &end, &diff);

      /* Add it to total time for each loop pass */

      timespec_add(&total, &diff);

      /* Get the average time */

      timespec_avg(&total, j, &avg);

      printf("%d: diff = %lu.%09lu s | avg = %lu.%09lu s\n", j,
             (unsigned long)diff.tv_sec, (unsigned long)diff.tv_nsec,
             (unsigned long)avg.tv_sec, (unsigned long)avg.tv_nsec);
    }

  printf("\n===== result =====\n");
  printf("count: %d\n", loop_count);
  printf("total: %lu.%09lu s\n", (unsigned long)total.tv_sec,
         (unsigned long)total.tv_nsec);
  printf("avg: %lu.%09lu s\n", (unsigned long)avg.tv_sec,
         (unsigned long)avg.tv_nsec);

  return 0;
}
