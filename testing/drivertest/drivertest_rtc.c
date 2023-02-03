/****************************************************************************
 * apps/testing/drivertest/drivertest_rtc.c
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
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sched.h>
#include <errno.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>

#include <nuttx/timers/rtc.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define RTC_DEFAULT_DEVPATH "/dev/rtc0"

/****************************************************************************
 * Private Type
 ****************************************************************************/

struct rtc_state_s
{
  char devpath[PATH_MAX];
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname,
                       FAR struct rtc_state_s *rtc_state, int exitcode)
{
  printf("Usage: %s -d <devpath>\n", progname);
  printf("  [-d devpath] selects the PWM device.\n"
         "  Default: %s Current: %s\n", RTC_DEFAULT_DEVPATH,
         rtc_state->devpath);
  exit(exitcode);
}

/****************************************************************************
 * Name: parse_commandline
 ****************************************************************************/

static void parse_commandline(FAR struct rtc_state_s *rtc_state, int argc,
                              FAR char **argv)
{
  int ch;

  while ((ch = getopt(argc, argv, "d:")) != ERROR)
    {
      switch (ch)
        {
          case 'd':
            strncpy(rtc_state->devpath, optarg, sizeof(rtc_state->devpath));
            rtc_state->devpath[sizeof(rtc_state->devpath) - 1] = '\0';
            break;

          case '?':
            printf("Unsupported option: %s\n", optarg);
            show_usage(argv[0], rtc_state, EXIT_FAILURE);
        }
    }
}

/****************************************************************************
 * Name: test_case_rtc
 ****************************************************************************/

static void test_case_rtc(FAR void **state)
{
  int fd;
  int ret;
  bool have_set_time;
  char timbuf[64];
  struct rtc_time set_time;
  struct rtc_time rd_time;
  FAR struct rtc_state_s *rtc_state;

  rtc_state = (FAR struct rtc_state_s *)*state;

  fd = open(rtc_state->devpath, O_WRONLY);
  assert_return_code(fd, 0);

  ret = ioctl(fd, RTC_HAVE_SET_TIME,
              (unsigned long)((uintptr_t)&have_set_time));
  assert_return_code(ret, OK);
  assert_true(have_set_time);

  ret = ioctl(fd, RTC_SET_TIME, (unsigned long)((uintptr_t)&set_time));
  assert_return_code(ret, OK);

  ret = ioctl(fd, RTC_RD_TIME, (unsigned long)((uintptr_t)&rd_time));
  assert_return_code(ret, OK);
  ret = strftime(timbuf, sizeof(timbuf), "%a, %b %d %H:%M:%S %Y",
                 (FAR struct tm *)&rd_time);
  assert_return_code(ret, OK);

  close(fd);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct rtc_state_s rtc_state =
  {
    .devpath = RTC_DEFAULT_DEVPATH,
  };

  parse_commandline(&rtc_state, argc, argv);

  const struct CMUnitTest tests[] =
  {
    cmocka_unit_test_prestate(test_case_rtc, &rtc_state)
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
