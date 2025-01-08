/****************************************************************************
 * apps/testing/drivers/drivertest/drivertest_oneshot.c
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
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <cmocka.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <nuttx/clock.h>
#include <nuttx/timers/oneshot.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DEFAULT_TIME_OUT 5
#define ONESHOT_SIGNO 13
#define ONESHOT_DEFAULT_NSAMPLES 5

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

struct oneshot_state_s
{
  int maxdelay;
  int fre;
  char devpath[PATH_MAX];
  struct oneshot_start_s oneshot;
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
         " -s <seconds> -d <path>\n",
         progname);

  exit(exitcode);
}

/****************************************************************************
 * Name: parse_commandline
 ****************************************************************************/

static void parse_commandline(FAR struct oneshot_state_s *oneshot_state,
                              int argc, FAR char **argv)
{
  int ch;
  time_t converted;

  while ((ch = getopt(argc, argv, "s:d:")) != ERROR)
    {
      switch (ch)
        {
          case 's':
            OPTARG_TO_VALUE(converted, time_t, 10);
            if (converted < 1 || converted > INT_MAX)
              {
                printf("signal out of range:%lld\n", (long long)converted);
                show_usage(argv[0], EXIT_FAILURE);
              }

            oneshot_state->oneshot.ts.tv_sec = converted;
            break;

          case 'd':
            strlcpy(oneshot_state->devpath, optarg,
                                sizeof(oneshot_state->devpath));
            break;

          case '?':
            printf("Unsupported option: %s\n", optarg);
            show_usage(argv[0], EXIT_FAILURE);
            break;
        }
    }
}

/****************************************************************************
 * Name: drivertest_oneshot
 ****************************************************************************/

static void drivertest_oneshot(FAR void **state)
{
  int ret;
  int fd;
  int i;
  struct timespec trigger_before;
  struct timespec ts;
  struct timespec timespec_sub;
  sigset_t set;
  FAR struct oneshot_state_s *oneshot_state =
    (FAR struct oneshot_state_s *)*state;

  oneshot_state->oneshot.pid = getpid();

  signal(ONESHOT_SIGNO, SIG_IGN);
  sigemptyset(&set);
  sigaddset(&set, ONESHOT_SIGNO);
  oneshot_state->oneshot.event.sigev_notify = SIGEV_SIGNAL;
  oneshot_state->oneshot.event.sigev_signo = ONESHOT_SIGNO;
  oneshot_state->oneshot.event.sigev_value.sival_ptr = NULL;

  fd = open(oneshot_state->devpath, O_RDONLY);
  assert_true(fd > 0);

  /* Get MaxDelay */

  ret = ioctl(fd, OSIOC_MAXDELAY, &ts);
  assert_return_code(ret, OK);

  syslog(LOG_DEBUG, "maxdelay sec:%lld\n", (long long)ts.tv_sec);
  syslog(LOG_DEBUG, "maxdelay nsec:%ld\n", ts.tv_nsec);

  for (i = 0; i < ONESHOT_DEFAULT_NSAMPLES; i++)
    {
      /* Start the oneshot */

      ret = ioctl(fd, OSIOC_START, &oneshot_state->oneshot);
      assert_return_code(ret, OK);

      /* Get current ts */

      ret = ioctl(fd, OSIOC_CURRENT, &ts);
      assert_return_code(ret, OK);
      trigger_before = ts;

      ret = sigwaitinfo(&set, NULL);
      assert_return_code(ret, ONESHOT_SIGNO);

      ret = ioctl(fd, OSIOC_CURRENT, &ts);
      assert_return_code(ret, OK);

      clock_timespec_subtract(&ts, &trigger_before, &timespec_sub);
      assert_in_range(timespec_sub.tv_sec,
                     oneshot_state->oneshot.ts.tv_sec - 1,
                     oneshot_state->oneshot.ts.tv_sec);
      assert_in_range(timespec_sub.tv_nsec, 0, NSEC_PER_SEC);
    }

  ret = ioctl(fd, OSIOC_START, &oneshot_state->oneshot);
  assert_return_code(ret, OK);

  /* Cancel the oneshot */

  ret = ioctl(fd, OSIOC_CANCEL, &ts);
  assert_return_code(ret, OK);

  clock_timespec_subtract(&oneshot_state->oneshot.ts,
                          &ts, &timespec_sub);
  assert_int_equal(0, timespec_sub.tv_sec);

  close(fd);
}

int main(int argc, FAR char *argv[])
{
  struct oneshot_state_s oneshot_state =
  {
    .devpath = "/dev/oneshot",
    .oneshot.ts.tv_sec = DEFAULT_TIME_OUT,
    .oneshot.ts.tv_nsec = 0
  };

  const struct CMUnitTest tests[] =
  {
    cmocka_unit_test_prestate(drivertest_oneshot, &oneshot_state)
  };

  parse_commandline(&oneshot_state, argc, argv);

  return cmocka_run_group_tests(tests, NULL, NULL);
}
