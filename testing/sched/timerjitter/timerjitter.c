/****************************************************************************
 * apps/testing/sched/timerjitter/timerjitter.c
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

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DEFAULT_CLOCKID   CLOCK_REALTIME
#define DEFAULT_INTERVAL  1000
#define DEFAULT_ITERATION 1000

/* Fix compilation error for Non-NuttX OS */
#ifndef FAR
  #define FAR
#endif

#ifndef USEC_PER_SEC
  #define USEC_PER_SEC 1000000
#endif

#ifndef NSEC_PER_SEC
  #define NSEC_PER_SEC 1000000000
#endif

/****************************************************************************
 * Private Type
 ****************************************************************************/

struct timerjitter_param_s
{
  clockid_t     clockid;
  unsigned int  interval;
  unsigned long max_cnt;
  unsigned long cur_cnt;
  double        avg;
  unsigned long max;
  unsigned long min;
  int           print;
  unsigned int  missed;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Helper functions for timespec calculating */

static inline int64_t calc_diff(FAR const struct timespec *t1,
                                FAR const struct timespec *t2)
{
  int64_t diff;

  diff  = USEC_PER_SEC * (t1->tv_sec - t2->tv_sec);
  diff += (t1->tv_nsec - t2->tv_nsec) / 1000;

  return diff;
}

static inline void ts_norm(FAR struct timespec *ts)
{
  while (ts->tv_nsec >= NSEC_PER_SEC)
    {
      ts->tv_nsec -= NSEC_PER_SEC;
      ts->tv_sec++;
    }
}

static inline int ts_greater(FAR const struct timespec *a,
                             FAR const struct timespec *b)
{
  return (a->tv_sec > b->tv_sec) ||
         (a->tv_sec == b->tv_sec && a->tv_nsec > b->tv_nsec);
}

static inline void calc_next(FAR struct timespec *t,
                             FAR struct timespec *intv)
{
  t->tv_sec  += intv->tv_sec;
  t->tv_nsec += intv->tv_nsec;
  ts_norm(t);
}

/* Helper function for number parsing */

static unsigned long get_num(FAR const char *str)
{
  FAR char     *end;
  unsigned long val;

  if (!str)
    {
      return 0;
    }

  val = strtoul(str, &end, 0);
  if (!val || val == (unsigned long)-1)
    {
      return 0;
    }

  return val;
}

static FAR void *timerjitter(FAR void *arg)
{
  FAR struct timerjitter_param_s *param = arg;
  struct timespec   now;
  struct timespec   next;
  struct timespec   intv;
  struct itimerspec tspec;
  struct sigevent   sigev;
  sigset_t          sigset;
  timer_t           timer;
  int64_t           diff;
  int               sigs;
  int               ret;

  sigemptyset(&sigset);
  sigaddset(&sigset, SIGALRM);
  sigprocmask(SIG_BLOCK, &sigset, NULL);

  intv.tv_sec  = param->interval / USEC_PER_SEC;
  intv.tv_nsec = (param->interval % USEC_PER_SEC) * 1000;

  sigev.sigev_notify = SIGEV_SIGNAL;
  sigev.sigev_signo  = SIGALRM;

  timer_create(param->clockid, &sigev, &timer);
  clock_gettime(param->clockid, &now);

  next = now;
  calc_next(&next, &intv);

  /* Set cyclic timer */

  tspec.it_interval = intv;

  /* Using TIMER_ABSTIME */

  tspec.it_value = next;
  timer_settime(timer, TIMER_ABSTIME, &tspec, NULL);

  param->avg = 0;
  param->max = 0;
  param->min = (unsigned long)-1;

  while (param->cur_cnt++ < param->max_cnt)
    {
      /* Wait for SIGALRM */

      if (sigwait(&sigset, &sigs) < 0)
        {
          printf("sig wait failed\n");
          break;
        }

      ret = clock_gettime(param->clockid, &now);
      if (ret)
        {
          printf("clock_gettime failed %d\n", ret);
        }

      diff = calc_diff(&now, &next);
      if (param->print)
        {
          printf("diff %lu, now %lu.%lu\n", diff, now.tv_sec, now.tv_nsec);
        }

      if (diff > param->max)
        {
          param->max = diff;
        }

      if (diff < param->min)
        {
          param->min = diff;
        }

      param->avg += diff;

      /* Calculate next = next + intv */

      calc_next(&next, &intv);

      /* Calibrate if we miss current time frame */

      while (ts_greater(&now, &next))
        {
          calc_next(&next, &intv);
          printf("time frame missed %u\n", ++param->missed);
        }
    }

  param->avg = param->avg / param->cur_cnt;
  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * timerjitter main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct timerjitter_param_s param =
  {
    .clockid  = DEFAULT_CLOCKID,
    .interval = DEFAULT_INTERVAL,
    .max_cnt  = DEFAULT_ITERATION,
    .cur_cnt  = 0,
    .print    = 0,
    .missed   = 0
  };

  pthread_attr_t  attr;
  pthread_t       thread;
  sigset_t        sigset;
  FAR const char *arg;
  int             ret;

  /* Mask SIGALRM at first */

  sigemptyset(&sigset);
  sigaddset(&sigset, SIGALRM);
  sigprocmask(SIG_BLOCK, &sigset, NULL);

  if (argc > 1)
    {
      while ((arg = argv[1]) != NULL)
        {
          if (*arg != '-')
            {
              break;
            }

          for (; ; )
            {
              switch (*++arg)
                {
                  case 0:
                    break;
                  case 'p':
                    param.print = 1;
                    continue;
                  case 'm':
                    param.clockid = CLOCK_MONOTONIC;
                    continue;
                  case 'r':
                    param.clockid = CLOCK_REALTIME;
                    continue;
                  case 'h':
                    printf(
                    "usage: timerjitter [-pmr] [interval(us)] [iteration]\n"
                    "-p: print time diff between two iteration\n"
                    "-m: use CLOCK_MONOTONIC\n"
                    "-r: use CLOCK_REALTIME\n"
                    "");
                    return 0;
                  default:
                    printf("Unknown flag '%s'", arg);
                    return -1;
                }
              break;
            }

          /* Find next parameters */

          argv++;
          argc--;
        }

      if (argc > 1)
        {
          param.interval = get_num(argv[1]);
        }

      if (argc > 2)
        {
          param.max_cnt = get_num(argv[2]);
        }
    }

  ret = pthread_attr_init(&attr);
  if (ret)
    {
      printf("pthread_attr_init failed %d\n", ret);
    }

  ret = pthread_create(&thread, &attr, timerjitter, &param);
  if (ret)
    {
      printf("thread created failed %d\n", ret);
    }

  pthread_join(thread, NULL);

  ret = pthread_attr_destroy(&attr);
  if (ret)
    {
      printf("pthread_attr_destroy failed %d\n", ret);
    }

  printf("timer jitter in %lu run:\n", param.max_cnt);
  printf("(latency/us) min: %lu, avg: %.0lf, max %lu\n",
         param.min, param.avg, param.max);

  return 0;
}
