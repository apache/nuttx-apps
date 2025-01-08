/****************************************************************************
 * apps/testing/drivers/drivertest/drivertest_timer.c
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

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <syslog.h>
#include <nuttx/timers/timer.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TIMER_DEFAULT_DEVPATH "/dev/timer0"
#define TIMER_DEFAULT_INTERVAL 1000000
#define TIMER_DEFAULT_NSAMPLES 20
#define TIMER_DEFAULT_SIGNO 31
#define TIMER_DEFAULT_RANGE 1000

#define OPTARG_TO_VALUE(value, type, base)                            \
  do                                                                  \
    {                                                                 \
      FAR char *ptr;                                                  \
      value = (type)strtoul(optarg, &ptr, base);                      \
      if (*ptr != '\0')                                               \
        {                                                             \
          printf("Parameter error: -%c %s\n", ch, optarg);            \
          show_usage(argv[0], timer_state, EXIT_FAILURE);             \
        }                                                             \
    } while (0)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct timer_state_s
{
  char devpath[PATH_MAX];
  uint32_t interval;
  uint32_t nsamples;
  uint32_t signo;
  uint32_t range;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: get_timestamp
 ****************************************************************************/

static uint32_t get_timestamp(void)
{
  struct timespec ts;
  uint32_t ms;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
  return ms;
}

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname,
                       FAR struct timer_state_s *timer_state, int exitcode)
{
  printf("Usage: %s"
         " -d <devpath> -i <interval> -n <nsamples> -r <range> -s <signo>\n",
         progname);
  printf("  [-d devpath] selects the TIMER device.\n"
         "  Default: %s Current: %s\n",
         TIMER_DEFAULT_DEVPATH, timer_state->devpath);
  printf("  [-i interval] timer interval in microseconds.\n"
         "  Default: %d Current: %" PRIu32 "\n",
         TIMER_DEFAULT_INTERVAL, timer_state->interval);
  printf("  [-n nsamples] timer samples will be collected numbers.\n"
         "  Default: %d Current: %" PRIu32 "\n",
         TIMER_DEFAULT_NSAMPLES, timer_state->nsamples);
  printf("  [-r range] the max range of timer delay.\n"
         "  Default: %d Current: %" PRIu32 "\n",
         TIMER_DEFAULT_RANGE, timer_state->range);
  printf("  [-s signo] used to notify the test of timer expiration events.\n"
         "  Default: %d Current: %" PRIu32 "\n",
         TIMER_DEFAULT_SIGNO, timer_state->signo);
  printf("  [-h] = Shows this message and exits\n");

  exit(exitcode);
}

/****************************************************************************
 * Name: parse_commandline
 ****************************************************************************/

static void parse_commandline(FAR struct timer_state_s *timer_state,
                              int argc, FAR char **argv)
{
  int ch;
  int converted;

  while ((ch = getopt(argc, argv, "d:i:n:r:s:h")) != ERROR)
    {
      switch (ch)
        {
          case 'd':
            strlcpy(timer_state->devpath, optarg,
                    sizeof(timer_state->devpath));
            break;

          case 'i':
            OPTARG_TO_VALUE(converted, uint32_t, 10);
            if (converted < 1 || converted > INT_MAX)
              {
                printf("signal out of range: %d\n", converted);
                show_usage(argv[0], timer_state, EXIT_FAILURE);
              }

            timer_state->interval = (uint32_t)converted;
            break;

          case 'n':
            OPTARG_TO_VALUE(converted, uint32_t, 10);
            if (converted < 1 || converted > INT_MAX)
              {
                printf("signal out of range: %d\n", converted);
                show_usage(argv[0], timer_state, EXIT_FAILURE);
              }

            timer_state->nsamples = (uint32_t)converted;
            break;

          case 'r':
            OPTARG_TO_VALUE(converted, uint32_t, 10);
            if (converted < 1 || converted > INT_MAX)
              {
                printf("signal out of range: %d\n", converted);
                show_usage(argv[0], timer_state, EXIT_FAILURE);
              }

            timer_state->range = (uint32_t)converted;
            break;

          case 's':
            OPTARG_TO_VALUE(converted, uint32_t, 10);
            if (converted < 1 || converted > INT_MAX)
              {
                printf("signal out of range: %d\n", converted);
                show_usage(argv[0], timer_state, EXIT_FAILURE);
              }

            timer_state->signo = (uint32_t)converted;
            break;

          case '?':
            printf("Unsupported option: %s\n", optarg);
            show_usage(argv[0], timer_state, EXIT_FAILURE);
            break;
        }
    }
}

/****************************************************************************
 * Name: drivertest_timer
 ****************************************************************************/

static void drivertest_timer(FAR void **state)
{
  int i;
  int fd;
  int ret;
  uint32_t range;
  uint32_t tim;
  uint32_t max_timeout;
  struct sigaction act;
  struct timer_notify_s notify;
  struct timer_status_s timer_status;
  FAR struct timer_state_s *timer_state;

  timer_state = (FAR struct timer_state_s *)*state;

  /* Open the timer device */

  fd = open(timer_state->devpath, O_RDONLY);
  assert_true(fd > 0);

  /* Show the timer status before setting the timer interval */

  ret = ioctl(fd, TCIOC_SETTIMEOUT, timer_state->interval);
  assert_return_code(ret, OK);

  act.sa_sigaction = NULL;
  act.sa_flags     = SA_SIGINFO;
  sigfillset(&act.sa_mask);
  sigdelset(&act.sa_mask, timer_state->signo);

  ret = sigaction(timer_state->signo, &act, NULL);
  assert_in_range(ret, 0, timer_state->signo);

  /* Register a callback for notifications using the configured signal.
   * NOTE: If no callback is attached, the timer stop at the first interrupt.
   */

  notify.pid      = getpid();
  notify.periodic = true;
  notify.event.sigev_notify = SIGEV_SIGNAL;
  notify.event.sigev_signo  = timer_state->signo;
  notify.event.sigev_value.sival_ptr = NULL;

  ret = ioctl(fd, TCIOC_NOTIFICATION, (unsigned long)((uintptr_t)&notify));
  assert_return_code(ret, OK);

  /* Start the timer */

  ret = ioctl(fd, TCIOC_START, 0);
  assert_return_code(ret, OK);

  /* Get status */

  ret = ioctl(fd, TCIOC_GETSTATUS, &timer_status);
  assert_return_code(ret, OK);
  assert_int_equal(timer_state->interval, timer_status.timeout);
  assert_in_range(timer_status.timeleft,
                  0, timer_state->interval);

  /* Get max timeout */

  ret = ioctl(fd, TCIOC_MAXTIMEOUT, &max_timeout);
  assert_return_code(ret, OK);
  syslog(LOG_DEBUG, "max timeout:%ld\n", max_timeout);

  /* Set the timer interval */

  for (i = 0; i < timer_state->nsamples; i++)
    {
      tim = get_timestamp();
      usleep(2 * timer_state->interval);
      tim = get_timestamp() - tim;
      range = abs(timer_state->interval / 1000 - tim);
      assert_in_range(range, 0, timer_state->range);
    }

  /* Stop the timer */

  ret = ioctl(fd, TCIOC_STOP, 0);
  assert_return_code(ret, OK);

  /* Detach the signal handler */

  act.sa_handler = SIG_DFL;
  sigaction(timer_state->signo, &act, NULL);

  /* Close the timer driver */

  close(fd);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: drivertest_timer_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct timer_state_s timer_state =
  {
    .devpath = TIMER_DEFAULT_DEVPATH,
    .interval = TIMER_DEFAULT_INTERVAL,
    .nsamples = TIMER_DEFAULT_NSAMPLES,
    .range = TIMER_DEFAULT_RANGE,
    .signo = TIMER_DEFAULT_SIGNO
  };

  parse_commandline(&timer_state, argc, argv);

  const struct CMUnitTest tests[] =
  {
    cmocka_unit_test_prestate(drivertest_timer, &timer_state)
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
