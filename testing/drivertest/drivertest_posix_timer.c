/****************************************************************************
 * apps/testing/drivertest/drivertest_posix_timer.c
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
#include <signal.h>
#include <setjmp.h>
#include <stdlib.h>
#include <cmocka.h>
#include <syslog.h>
#include <nuttx/timers/timer.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define RTC_DEFAULT_DEVIATION 10
#define DEFAULT_TIME_OUT      2
#define SLEEPSECONDS          10
#define COMEINCALLBACK        0

#define OPTARG_TO_VALUE(value, type, base)                            \
  do                                                                  \
    {                                                                 \
      FAR char *ptr;                                                  \
      value = (type)strtoul(optarg, &ptr, base);                      \
      if (*ptr != '\0')                                               \
        {                                                             \
          printf("Parameter error: -%c %s\n", ch, optarg);            \
          show_usage(argv[0], EXIT_FAILURE);                          \
        }                                                             \
    } while (0)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct posix_timer_state_s
{
  struct itimerspec it;
};

struct sigev_para_s
{
  int tim;
  int interval;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname, int exitcode)
{
  printf("Usage: %s"
         " -s <timeout seconds>\n",
         progname);

  exit(exitcode);
}

/****************************************************************************
 * Name: parse_commandline
 ****************************************************************************/

static void parse_commandline(
  FAR struct posix_timer_state_s *posix_timer_state,
  int argc, FAR char **argv)
{
  int ch;
  int converted;

  while ((ch = getopt(argc, argv, "s:")) != ERROR)
    {
      switch (ch)
        {
          case 's':
            OPTARG_TO_VALUE(converted, uint32_t, 10);
            if (converted < 1 || converted > INT_MAX)
              {
                printf("signal out of range: %d\n", converted);
                show_usage(argv[0], EXIT_FAILURE);
              }

            posix_timer_state->it.it_value.tv_sec = (uint32_t)converted;
            posix_timer_state->it.it_interval.tv_sec = (uint32_t)converted;
            break;

          case '?':
            printf("Unsupported option: %s\n", optarg);
            show_usage(argv[0], EXIT_FAILURE);
            break;
        }
    }
}

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
 * Name: posix_timer_callback
 ****************************************************************************/

static void posix_timer_callback(union sigval arg)
{
  FAR struct sigev_para_s *sigev_para = (struct sigev_para_s *)arg.sival_ptr;
  int range = get_timestamp() - (*sigev_para).tim;
  assert_in_range(range,
                  (*sigev_para).interval * 1000 - RTC_DEFAULT_DEVIATION,
                  (*sigev_para).interval * 1000 + RTC_DEFAULT_DEVIATION);
  syslog(LOG_DEBUG, "callback trigger!!!\n");
  (*sigev_para).tim = get_timestamp();
}

/****************************************************************************
 * Name: test_case_posix_timer
 ****************************************************************************/

static void test_case_posix_timer(FAR void **state)
{
  int ret;
  timer_t timerid;
  struct sigevent event;
  struct sigev_para_s sigev_para;

  FAR struct posix_timer_state_s *posix_timer_state;
  struct itimerspec it;

  memset(&it, 0, sizeof(it));
  posix_timer_state = (FAR struct posix_timer_state_s *)*state;

  event.sigev_notify = SIGEV_THREAD;
  event.sigev_notify_function = posix_timer_callback;
  event.sigev_notify_attributes = NULL;
  event.sigev_value.sival_ptr = (struct sigev_para_s *)&sigev_para;

  /* Create the timer */

  ret = timer_create(CLOCK_MONOTONIC, &event, &timerid);
  assert_return_code(ret, OK);

  /* Start the timer */

  ret = timer_settime(timerid, 0, &(posix_timer_state->it), NULL);
  assert_return_code(ret, OK);

  sigev_para.interval = posix_timer_state->it.it_interval.tv_sec;
  sigev_para.tim = get_timestamp();

  /* Get the timer status */

  ret = timer_gettime(timerid, &it);
  assert_return_code(ret, OK);
  assert_in_range(it.it_value.tv_sec, 0,
                  posix_timer_state->it.it_value.tv_sec);
  assert_return_code(it.it_interval.tv_sec,
                     posix_timer_state->it.it_interval.tv_sec);

  sleep(SLEEPSECONDS);

  /* Delete the timer */

  ret = timer_delete(timerid);
  assert_return_code(ret, OK);
}

int main(int argc, FAR char *argv[])
{
  struct posix_timer_state_s posix_timer_state =
  {
    .it.it_value.tv_sec     = DEFAULT_TIME_OUT,
    .it.it_value.tv_nsec    = 0,
    .it.it_interval.tv_sec  = DEFAULT_TIME_OUT,
    .it.it_interval.tv_nsec = 0
  };

  const struct CMUnitTest tests[] =
  {
    cmocka_unit_test_prestate(test_case_posix_timer, &posix_timer_state)
  };

  parse_commandline(&posix_timer_state, argc, argv);

  return cmocka_run_group_tests(tests, NULL, NULL);
}
