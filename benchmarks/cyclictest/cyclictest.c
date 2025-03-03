/****************************************************************************
 * apps/benchmarks/cyclictest/cyclictest.c
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
 * A NuttX port of the cyclictest rt-tests utility for Linux.
 * As of writing this piece of software (Feb 2025), clock_gettime
 * and clock_nanosleep are tightly tied to systemtick by default.
 * Yes, the time resolution can be achieved by using TICKLESS, but for high
 * resolution waiting and measurement we can use other methods.
 *
 * This piece of software includes configurable waiting methods:
 *   - clock_nanosleep
 *   - systemtick hook: it is assumed your BSP supports a board_timerhook
 *       function where a sem_t g_waitsem is posted.
 *     - WARNING: only one task (thread) can wait for the semaphore.
 *   - NuttX Timer API: waiting for the timer to expire.
 *     - WARNING: only one task (thread) can wait for the timer to expire.
 *
 * Available time measuring methods:
 *  - clock_gettime
 *  - NuttX Timer API
 *
 * Authors of the NuttX port: Stepan Pressl <pressl.stepan@gmail.com>
 *                                          <pressste@fel.cvut.cz>
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sched.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <poll.h>
#include <fcntl.h>

#include <nuttx/timers/timer.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum meas_method_e
{
  M_GETTIME = 0,
  M_TIMER_API,
  M_COUNT
};

enum wait_method_e
{
  W_NANOSLEEP = 0,
  W_DEVTIMER,
  W_COUNT
};

struct cyclictest_config_s
{
  int clock;
  int distance;
  int duration;
  int histogram;
  int histofall;
  unsigned long interval;
  unsigned long loops;
  int threads;
  int policy;
  int prio;
  bool quiet;
  char *timer_dev;
  enum meas_method_e meas_method;
  enum wait_method_e wait_method;
};

struct thread_param_s
{
  int prio;
  int policy;
  unsigned long interval;
  unsigned long max_cycles;
  struct thread_stats_s *stats;
  int clock;
};

struct thread_stats_s
{
  long *hist_array;
  long hist_overflow;
  long min;
  long max;
  long act;
  double avg;
  unsigned long cycles;
  pthread_t id;
  int tid;
  bool ended;
};

static bool running;
static struct cyclictest_config_s config;
static int timerfd;
static struct pollfd polltimer[1];

static const struct option optargs[] =
{
  {"clock", optional_argument, 0, 'c'},
  {"distance", optional_argument, 0, 'd'},
  {"duration", optional_argument, 0, 'D'},
  {"help", optional_argument, 0, 'e'},
  {"histogram", optional_argument, 0, 'h'},
  {"histofall", optional_argument, 0, 'H'},
  {"interval", optional_argument, 0, 'i'},
  {"loops", optional_argument, 0, 'l'},
  {"measurement", optional_argument, 0, 'm'},
  {"nanosleep", optional_argument, 0, 'n'},
  {"prio", optional_argument, 0, 'p'},
  {"quiet", optional_argument, 0, 'q'},
  {"threads", optional_argument, 0, 't'},
  {"timer-device", optional_argument, 0, 'T'},
  {"policy", optional_argument, 0, 'y'},
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void print_help(void)
{
  puts(
    "The Cyclictest Benchmark Utility\n"
    "Usage:\n"
    "  -c --clock [CLOCK]: selects the clock: 0 selects CLOCK_REALTIME, "
    "1 selects CLOCK_MONOTONIC (default).\n"
    "  -d --distance [US]: The distance of thread intervals. "
    "Default is 500us.\n"
    "  -D --duration [TIME]: Test duration length in seconds. "
    "Default is 0 (endless).\n"
    "  -e --help: Display this help and quit.\n"
    "  -h --histogram [US]: Output the histogram data to stdout. "
    "US is the maximum value to be printed.\n"
    "  -H --histofall: Same as -h except that an additional histogram "
    "column\n"
    "     is displayed at the right that contains summary data of all thread"
    " histograms.\n"
    "     If cyclictest runs a single thread only, the -H option is "
    "equivalent to -h.\n"
    "  -i --interval [US]: The thread interval. Default is 1000us.\n"
    "  -l --loops [N]: The number of measurement loops. Default is 0 "
    "(endless).\n"
    "  -m --measurement [METHOD]: Set the time measurement method:\n"
    "     0 selects clock_gettime, 1 uses the NuttX timer API.\n"
    "     WARNING:\n"
    "       If METHOD 1 is selected, you need to specify a timer device "
    "(e.g. /dev/timer0) in -T.\n"
    "  -n --nanosleep [METHOD]: Set the waiting method: 0 selects "
    "clock_nanosleep,\n"
    "     1 waits for the POLLIN flag on a timer device. Default is 0.\n"
    "     WARNING:\n"
    "       Choosing 1 works only with one thread, "
    "the -t value is therefore set to 1.\n"
    "       If METHOD 1 is selected, you need to specify a timer device "
    "(e.g. /dev/timer0) in -T.\n"
    "  -p --prio: Set the priority of the first thread.\n"
    "  -q --quiet: Print a summary only on exit.\n"
    "  -t --threads [N]: The number of test threads to be created. "
    "Default is 1.\n"
    "  -T --timer-device [DEV]: The measuring timer device.\n"
    "     Must be specified when -m=1 or -n=1.\n"
    "  -y --policy [NAME]: Set the scheduler policy, where NAME is \n"
    "     fifo, rr, batch, idle, normal, other.\n"
  );
}

static long arg_decimal(char *arg)
{
  long ret;
  char *endptr;
  ret = strtol(arg, &endptr, 10);
  if (endptr == arg)
    {
      return -1;
    }

  return ret;
}

static bool parse_args(int argc, char * const argv[])
{
  int longindex;
  int opt;
  long decimal;
  while ((opt = getopt_long(argc, argv, "c:d:D:h:Hi:l:m:n:p:qt:T:",
                  optargs, &longindex)) != -1)
    {
      switch (opt)
        {
          case 'c':
            decimal = arg_decimal(optarg);
            if (decimal < 0)
              {
                return false;
              }
            else if (decimal == CLOCK_MONOTONIC || decimal == CLOCK_REALTIME)
              {
                config.clock = decimal;
              }
            break;
          case 'd':
            decimal = arg_decimal(optarg);
            if (decimal >= 0)
              {
                config.distance = decimal;
              }
            else
              {
                return false;
              }
            break;
          case 'D':
            decimal = arg_decimal(optarg);
            if (decimal >= 0)
              {
                config.duration = decimal;
              }
            else
              {
                return false;
              }
            break;
          case 'e':
            return true;
          case 'h':
            decimal = arg_decimal(optarg);
            if (decimal >= 0)
              {
                config.histogram = decimal;
              }
            else
              {
                return false;
              }
            break;
          case 'H':
            config.histofall = true;
            break;
          case 'i':
            decimal = arg_decimal(optarg);
            if (decimal >= 0)
              {
                config.interval = decimal;
              }
            else
              {
                return false;
              }
            break;
          case 'l':
            decimal = arg_decimal(optarg);
            if (decimal >= 0)
              {
                config.loops = decimal;
              }
            else
              {
                return false;
              }
            break;
          case 'm':
            decimal = arg_decimal(optarg);
            if (decimal >= 0)
              {
                config.meas_method = decimal;
              }
            else
              {
                return false;
              }
            break;
          case 'n':
            decimal = arg_decimal(optarg);
            if (decimal >= 0)
              {
                config.wait_method = decimal;
              }
            else
              {
                return false;
              }
            break;
          case 'p':
            decimal = arg_decimal(optarg);
            if (decimal >= 0 && decimal <= 255)
              {
                config.prio = decimal;
              }
            else
              {
                return false;
              }
            break;
          case 'q':
            config.quiet = true;
            break;
          case 't':
            decimal = arg_decimal(optarg);
            if (decimal > 0)
              {
                config.threads = decimal;
              }
            else
              {
                return false;
              }
            break;
          case 'T':
            config.timer_dev = optarg;
            break;
          case 'y':
            if (strcmp(optarg, "other") == 0)
              {
                config.policy = SCHED_OTHER;
              }
            else if (strcmp(optarg, "normal") == 0)
              {
                config.policy = SCHED_NORMAL;
              }
            else if (strcmp(optarg, "batch") == 0)
              {
                config.policy = SCHED_BATCH;
              }
            else if (strcmp(optarg, "idle") == 0)
              {
                config.policy = SCHED_IDLE;
              }
            else if (strcmp(optarg, "fifo") == 0)
              {
                config.policy = SCHED_FIFO;
              }
            else if (strcmp(optarg, "rr") == 0)
              {
                config.policy = SCHED_RR;
              }
            else
              {
                return false;
              }
            break;
          case '?':
            return false;
          default:
            break;
        }
    }

  if (optind < argc)
    {
      return false;
    }

  return true;
}

static bool check_args_logic(void)
{
  /* Check if -T option was passed */

  if (config.wait_method == W_DEVTIMER || config.meas_method == M_TIMER_API)
    {
      if (config.timer_dev == NULL)
        {
          fprintf(stderr, "Specify timer device!\n");
          return false;
        }
    }

  /* Works only with one thread */

  if (config.wait_method == W_DEVTIMER)
    {
      config.threads = 1;
    }

  /* If the priority was not loaded, default to number of threads. */

  if (config.prio == 0)
    {
      config.prio = config.threads;
    }

  if (config.wait_method >= W_COUNT)
    {
      return false;
    }

  if (config.meas_method >= M_COUNT)
    {
      return false;
    }

  return true;
}

static inline void tsnorm(struct timespec *ts)
{
  while (ts->tv_nsec >= NSEC_PER_SEC)
    {
      ts->tv_nsec -= NSEC_PER_SEC;
      ts->tv_sec++;
    }
}

static inline int64_t timediff_us(struct timespec t1, struct timespec t2)
{
  int64_t ret;
  ret = 1000000 * (int64_t) ((int) t1.tv_sec - (int) t2.tv_sec);
  ret += (int64_t) ((int) t1.tv_nsec - (int) t2.tv_nsec) / 1000;
  return ret;
}

static inline int64_t timediff_us_timer(struct timer_status_s after,
                                        struct timer_status_s before)
{
  int64_t ret = 0;
  uint32_t t1;
  uint32_t t2;
  t1 = before.timeleft;
  t2 = after.timeleft;
  if (t2 < t1)
    {
      ret = (int64_t) (t1 - t2);
    }
  else
    {
      ret = (int64_t) (after.timeout - (t2 - t1));
    }

  return ret;
}

static inline int timerdev_getstatus(struct timer_status_s *st)
{
  return ioctl(timerfd, TCIOC_GETSTATUS, (unsigned long)((uintptr_t)st));
}

static void *testthread(void *arg)
{
  int ret;
  int32_t newint;
  int64_t diff = 0;
  struct timer_status_s stamp1;
  struct timer_status_s stamp2;
  struct timespec now;
  struct timespec next;
  struct timespec interval;
  struct timespec endtime;
  struct sched_param schedp;
  struct thread_param_s *param = (struct thread_param_s *)arg;
  struct thread_stats_s *stats = param->stats;

  stats->tid = gettid();
  stats->min = LONG_MAX;
  interval.tv_sec = param->interval / 1000000;
  interval.tv_nsec = (param->interval % 1000000) * 1000;

  /* Set priority and policy */

  schedp.sched_priority = param->prio;
  ret = pthread_setschedparam(pthread_self(), param->policy, &schedp);
  if (ret < 0)
    {
      goto threadend;
    }

  if (config.wait_method == W_DEVTIMER)
    {
      /* Start the timer here (we know the thread is only one) */

      ret = ioctl(timerfd, TCIOC_START);
      if (ret < 0)
        {
          perror("TCIOC_START");
          goto threadend;
        }
    }

  /* We can use clock_gettime for the endtime. */

  if ((ret = clock_gettime(param->clock, &now)) < 0)
    {
      goto threadend;
    }

  endtime.tv_sec = now.tv_sec + config.duration;
  endtime.tv_nsec = now.tv_nsec;

  while (running)
    {
      /* This inicializes the stamp1.timeout field */

      if (config.meas_method == M_TIMER_API)
        {
          ret = timerdev_getstatus(&stamp1);
          if (ret < 0)
            {
              perror("TCIOC_GETSTAUS");
              goto threadend;
            }
        }

      switch (config.wait_method)
        {
          case W_NANOSLEEP:
            if (config.meas_method == M_TIMER_API)
              {
                /* If we measure using the TIMER_API, compute
                 * the expected timestamp when the thread should wake up.
                 */

                newint = stamp1.timeleft - param->interval;
                if (newint < 0)
                  {
                    stamp1.timeleft = stamp1.timeout + newint;
                  }
              }

            next = now;
            next.tv_sec += interval.tv_sec;
            next.tv_nsec += interval.tv_nsec;
            tsnorm(&next);
            ret = clock_nanosleep(param->clock, TIMER_ABSTIME, &next, NULL);
            if (ret < 0)
              {
                goto threadend;
              }
            break;
          case W_DEVTIMER:
            if (config.meas_method == M_TIMER_API)
              {
                /* We suppose the timer resets itself when it
                 * overflows. So the first timestamp is timeout.
                 */

                stamp1.timeleft = stamp1.timeout;
              }
            else if (config.meas_method == M_GETTIME)
              {
                /* If we measure using the POSIX API, we must get the
                 * microseconds in which the currently running timer
                 * is supposed to timeout. We then convert this
                 * to the timespec struct, indicating the start.
                 */

                ret = timerdev_getstatus(&stamp1);
                if (ret < 0)
                  {
                    perror("TCIOC_GETSTATUS");
                    goto threadend;
                  }

                ret = clock_gettime(param->clock, &next);
                if (ret < 0)
                  {
                    goto threadend;
                  }

                next.tv_sec += (stamp1.timeleft) / 1000000;
                next.tv_nsec += (stamp1.timeleft % 1000000) * 1000;
                tsnorm(&next);
              }

            if ((ret = poll(polltimer, 1, -1)) < 0)
              {
                goto threadend;
              }
            break;
          default:
            break;
        }

      /* Time Stamp 2 */

      switch (config.meas_method)
        {
          case M_GETTIME:
            if ((ret = clock_gettime(param->clock, &now)) < 0)
              {
                goto threadend;
              }

            diff = timediff_us(now, next);
            break;
          case M_TIMER_API:
            ret = timerdev_getstatus(&stamp2);
            if (ret < 0)
              {
                perror("TCIOC_GETSTAUS");
                goto threadend;
              }

            diff = timediff_us_timer(stamp2, stamp1);
            break;
          default:
            break;
        }

      stats->act = diff;
      if (diff < stats->min)
        {
          stats->min = diff;
        }

      if (diff > stats->max)
        {
          stats->max = diff;
        }

      stats->avg += (double) diff;

      if (config.histogram)
        {
          if (diff < config.histogram && diff >= 0)
            {
              stats->hist_array[diff] += 1;
            }
          else
            {
              stats->hist_overflow += 1;
            }
        }

      ++stats->cycles;
      if (param->max_cycles != 0 && stats->cycles >= param->max_cycles)
        {
          stats->ended = true;
          break;
        }

      if (config.duration != 0)
        {
          if ((ret = clock_gettime(param->clock, &now)) < 0)
            {
              goto threadend;
            }

          if (timediff_us(now, endtime) >= 0)
            {
              stats->ended = true;
              break;
            }
        }
    }

threadend:
  return NULL;
}

static inline void init_thread_param(struct thread_param_s *param,
                                     unsigned long interval,
                                     unsigned long max_cycles,
                                     int policy,
                                     int prio,
                                     struct thread_stats_s *stats,
                                     int clock)
{
  stats->avg = 0.0;
  stats->cycles = 0;
  stats->max = -LONG_MAX;
  stats->min = LONG_MAX;
  stats->hist_overflow = 0;
  stats->ended = false;

  param->interval = interval;
  param->max_cycles = max_cycles;
  param->policy = policy;
  param->prio = prio;
  param->stats = stats;
  param->clock = clock;
}

/* Copied from the original rt-tests/cyclictest.
 * This way, the output is compatible with the original cyclictest.
 */

static void print_hist(struct thread_param_s *par[], int nthreads)
{
  int i;
  int j;
  unsigned long long int log_entries[nthreads + 1];
  unsigned long maxmax;
  unsigned long alloverflows;

  bzero(log_entries, sizeof(log_entries));

  printf("# Histogram\n");
  for (i = 0; i < config.histogram; i++)
    {
      unsigned long long int allthreads = 0;

      printf("%06d ", i);

      for (j = 0; j < nthreads; j++)
        {
          unsigned long curr_latency = par[j]->stats->hist_array[i];
          printf("%06lu", curr_latency);
          if (j < nthreads - 1)
            {
              printf("\t");
            }

          log_entries[j] += curr_latency;
          allthreads += curr_latency;
        }

      if (config.histofall && nthreads > 1)
        {
          printf("\t%06llu", allthreads);
          log_entries[nthreads] += allthreads;
        }

      printf("\n");
    }

  printf("# Total:");

  for (j = 0; j < nthreads; j++)
    {
      printf(" %09llu", log_entries[j]);
    }

  if (config.histofall && nthreads > 1)
    {
      printf(" %09llu", log_entries[nthreads]);
    }

  printf("\n");
  printf("# Min Latencies:");

  for (j = 0; j < nthreads; j++)
    {
      printf(" %05lu", par[j]->stats->min);
    }

  printf("\n");
  printf("# Avg Latencies:");

  for (j = 0; j < nthreads; j++)
    {
      printf(" %05lu", par[j]->stats->cycles ?
             (long)(par[j]->stats->avg / par[j]->stats->cycles) : 0);
    }

  printf("\n");
  printf("# Max Latencies:");

  maxmax = 0;
  for (j = 0; j < nthreads; j++)
    {
      printf(" %05lu", par[j]->stats->max);
      if (par[j]->stats->max > maxmax)
        {
          maxmax = par[j]->stats->max;
        }
    }

  if (config.histofall && nthreads > 1)
    {
      printf(" %05lu", maxmax);
    }

  printf("\n");
  printf("# Histogram Overflows:");

  alloverflows = 0;
  for (j = 0; j < nthreads; j++)
    {
      printf(" %05lu", par[j]->stats->hist_overflow);
      alloverflows += par[j]->stats->hist_overflow;
    }

  if (config.histofall && nthreads > 1)
    {
      printf(" %05lu", alloverflows);
    }

  printf("\n");
}

/* Copied from the original rt-tests/cyclictest.
 * This way, the output is compatible with the original cyclictest.
 */

static void print_stat(struct thread_param_s *par, int index)
{
  struct thread_stats_s *stat = par->stats;
  char *fmt;
  fmt = "T:%2d (%5d) P:%2d I:%ld C:%7lu "
        "Min:%7ld Act:%5ld Avg:%5ld Max:%8ld\n";
  printf(fmt, index, stat->tid, par->prio, par->interval, stat->cycles,
         stat->min, stat->act,
         stat->cycles ? (long)(stat->avg / stat->cycles) : 0, stat->max);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char *argv[])
{
  int i;
  int ret;
  struct thread_param_s **params = NULL;
  struct thread_stats_s **stats = NULL;
  struct sigevent event;
  struct timer_notify_s tnotify;
  uint32_t maxtimeout_timer;
  uint32_t reqtimeout_timer;

  running = true;
  config.clock = CLOCK_MONOTONIC;
  config.distance = 500;
  config.duration = 0;
  config.histogram = 0;
  config.histofall = 0;
  config.interval = 1000;
  config.loops = 0;
  config.threads = 1;
  config.prio = 0;
  config.policy = SCHED_FIFO;
  config.meas_method = M_GETTIME;
  config.wait_method = W_NANOSLEEP;
  config.timer_dev = NULL;
  config.quiet = false;

  if (!parse_args(argc, argv))
    {
      print_help();
      return ERROR;
    }

  if (!check_args_logic())
    {
      print_help();
      return ERROR;
    }

  /* Timer must be configured */

  if (config.wait_method == W_DEVTIMER || config.meas_method == M_TIMER_API)
    {
      timerfd = open(config.timer_dev, O_RDWR);
      if (timerfd < 0)
        {
          perror("Failed to open the device timer");
          return ERROR;
        }

      /* Configure the timer notification */

      polltimer[0].fd     = timerfd;
      polltimer[0].events = POLLIN;

      /* Fill in the notify struct
       * We do not want any signalling. But we must configure it,
       * because without it the timer will not start.
       */

      memset(&event, 0, sizeof(event));
      event.sigev_notify = SIGEV_NONE;

      tnotify.periodic = true;
      tnotify.pid      = getpid();
      tnotify.event    = event;

      /* Now set timeout of the timer.
       * This depends on several factors.
       *
       * If wait_method == W_DEVTIMER, the timeout is set to config.interval
       * (to achieve periodic operation). The extra time is measured by
       * NANOSLEEP or the timer itself. If the timer is used, the timer
       * zeroes itself when the timeout is reached, so we just get
       * the timer value after poll has stopped blocking.
       *
       * If wait_method != W_DEVTIMER, we must set the timeout to at least
       * the double of the maximum of all thread intervals
       * (if you're not sure, please consult Claude Shannon).
       *
       * This raises the question: what if wait_method == W_DEVTIMER
       * and meas_method == W_TIMER_API and the thread wakes up later
       * then the timer's timeout? The solution is to have a different
       * timer which runs slower and can measure overruns.
       * But this would overcomplicate things.
       */

      if (config.wait_method == W_DEVTIMER)
        {
          reqtimeout_timer = config.interval;
        }
      else if (config.wait_method == W_NANOSLEEP)
        {
          /* Multiply by 3 instead of 2, just to be sure */

          reqtimeout_timer = 3 * (config.interval +
                             (config.threads - 1) * config.distance);
        }

      ret = ioctl(timerfd, TCIOC_MAXTIMEOUT,
                  (unsigned long)((uintptr_t)&maxtimeout_timer));
      if (ret < 0)
        {
          perror("TCIOC_MAXTIMEOUT");
          goto errtimer;
        }

      if (reqtimeout_timer > maxtimeout_timer)
        {
          fprintf(stderr, "The timer cannot measure such periods!\n");
          goto errtimer;
        }

      ret = ioctl(timerfd, TCIOC_SETTIMEOUT,
                  (unsigned long)reqtimeout_timer);
      if (ret < 0)
        {
          perror("TCIOC_SETTIMEOUT");
          goto errtimer;
        }

      ret = ioctl(timerfd, TCIOC_NOTIFICATION,
                  (unsigned long)((uintptr_t)&tnotify));
      if (ret < 0)
        {
          perror("TCIOC_NOTIFICATION");
          goto errtimer;
        }

      /*  If the timer is used only for measurement, start it here, otherwise
       *  start it only in one thread.
       */

      if (config.wait_method != W_DEVTIMER)
        {
          ret = ioctl(timerfd, TCIOC_START);
          if (ret < 0)
            {
              perror("TCIOC_START");
              goto errtimer;
            }
        }
    }

  params = calloc(config.threads, sizeof(struct thread_param_s *));
  if (params == NULL)
    {
      perror("params");
      ret = ERROR;
      goto main_error;
    }

  stats  = calloc(config.threads, sizeof(struct thread_stats_s *));
  if (stats == NULL)
    {
      perror("stats");
      ret = ERROR;
      goto main_error;
    }

  for (i = 0; i < config.threads; ++i)
    {
      params[i] = malloc(sizeof(struct thread_param_s));
      if (params == NULL)
        {
          perror("params[i]");
          ret = ERROR;
          goto main_error;
        }

      stats[i]  = malloc(sizeof(struct thread_stats_s));
      if (params == NULL)
        {
          perror("stats[i]");
          ret = ERROR;
          goto main_error;
        }

      stats[i]->hist_array = calloc(config.histogram, sizeof(long));
      if (stats[i]->hist_array == NULL)
        {
          perror("hist_array");
          ret = ERROR;
          goto main_error;
        }

      init_thread_param(params[i], config.interval, config.loops,
                 config.policy, config.prio, stats[i], config.clock);

      pthread_create(&stats[i]->id, NULL, testthread, params[i]);
      config.interval += config.distance;
      if (config.prio > 1)
        {
          config.prio--;
        }
    }

  while (running)
    {
      /* Periodically update the output */

      usleep(100 * 1000);
      int ended = 0;
      for (i = 0; i < config.threads; ++i)
        {
          if (!config.quiet)
            {
              print_stat(params[i], i);
            }

          if (stats[i]->ended)
            {
              ended += 1;
            }
        }

      if (ended == config.threads)
        {
          running = false;
        }
      else if (!config.quiet)
        {
          printf("\x1B[%dA", config.threads);
        }
    }

  for (i = 0; i < config.threads; ++i)
    {
      pthread_join(stats[i]->id, NULL);
    }

  if (config.histogram)
    {
      print_hist(params, config.threads);
    }

  ret = OK;
  if (config.wait_method == W_DEVTIMER || config.meas_method == M_TIMER_API)
    {
      ret = ioctl(timerfd, TCIOC_STOP);
      if (ret < 0)
        {
          perror("TCIOC_STOP");
          ret = ERROR;
        }

      close(timerfd);
    }

main_error:
  if (stats != NULL)
    {
      for (i = 0; i < config.threads; ++i)
        {
          if (params[i] != NULL)
            {
              free(params[i]);
            }

          if (stats[i] != NULL)
            {
              if (stats[i]->hist_array != NULL)
                {
                  free(stats[i]->hist_array);
                }

              free(stats[i]);
            }
        }
    }

  free(stats);
  return ret;

errtimer:
  close(timerfd);
  return ERROR;
}
