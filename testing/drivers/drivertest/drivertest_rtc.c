/****************************************************************************
 * apps/testing/drivers/drivertest/drivertest_rtc.c
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
#include <syslog.h>
#include <getopt.h>
#include <nuttx/timers/rtc.h>
#include <nuttx/clock.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define RTC_DEFAULT_DEVPATH   "/dev/rtc0"
#define RTC_DEFAULT_DEVIATION 10
#define DEFAULT_TIME_OUT      5
#define SLEEPSECONDS          10
#define RTC_SIGNO             13
#define VENDOR_DELAY          0

#define OPTARG_TO_VALUE(value, type, base)                            \
  do                                                                  \
    {                                                                 \
      FAR char *ptr;                                                  \
      value = (type)strtoul(optarg, &ptr, base);                      \
      if (*ptr != '\0')                                               \
        {                                                             \
          printf("Parameter error: -%c %s\n", ch, optarg);            \
          show_usage(argv[0], rtc_state, EXIT_FAILURE);               \
        }                                                             \
    } while (0)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct rtc_state_s
{
  char devpath[PATH_MAX];
  int deviation;
  int tim;
  int vendor_delay;
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
  printf("Usage: %s -d <devpath> -a <deviation> -v <delay>\n", progname);
  printf("  [-d devpath] selects the rtc device.\n"
         "  Default: %s Current: %s\n", RTC_DEFAULT_DEVPATH,
         rtc_state->devpath);
  printf("  [-a deviation] input rtc alarm .\n"
         "  Default: %d Current: %d\n", RTC_DEFAULT_DEVIATION,
         rtc_state->deviation);
  printf("  [-v delay] delay after set rtc.\n"
         "  Default: %d Current: %d\n", VENDOR_DELAY,
         rtc_state->vendor_delay);
  exit(exitcode);
}

/****************************************************************************
 * Name: parse_commandline
 ****************************************************************************/

static void parse_commandline(FAR struct rtc_state_s *rtc_state, int argc,
                              FAR char **argv)
{
  int ch;
  int converted;

  while ((ch = getopt(argc, argv, "d:a:v:")) != ERROR)
    {
      switch (ch)
        {
          case 'd':
            strlcpy(rtc_state->devpath, optarg, sizeof(rtc_state->devpath));
            break;

          case 'a':
            OPTARG_TO_VALUE(converted, int, 10);
            rtc_state->deviation = converted;
            if (converted < 0)
              {
                printf("deviation out of range: %d\n", converted);
                show_usage(argv[0], rtc_state, EXIT_FAILURE);
              }
            break;

          case 'v':
            OPTARG_TO_VALUE(converted, int, 10);
            rtc_state->vendor_delay = converted;
            if (converted < 0)
              {
                printf("delay out of range: %d\n", converted);
                show_usage(argv[0], rtc_state, EXIT_FAILURE);
              }
            break;

          case '?':
            printf("Unsupported option: %s\n", optarg);
            show_usage(argv[0], rtc_state, EXIT_FAILURE);
        }
    }
}

/****************************************************************************
 * Name: drivertest_rtc_api
 ****************************************************************************/

static void drivertest_rtc_api(FAR void **state)
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

  memset(&set_time, 0, sizeof(set_time));
  set_time.tm_year = 2000 - TM_YEAR_BASE;
  set_time.tm_mon = TM_JANUARY;
  set_time.tm_mday = 1;
  set_time.tm_wday = TM_SATURDAY;

  ret = ioctl(fd, RTC_SET_TIME, (unsigned long)((uintptr_t)&set_time));
  assert_return_code(ret, OK);

  ret = ioctl(fd, RTC_HAVE_SET_TIME,
              (unsigned long)((uintptr_t)&have_set_time));
  assert_return_code(ret, OK);

  /* Some vendor need sleep a period of time
   * after set rtc because of hardware bug.
   */

  sleep(rtc_state->vendor_delay);

  assert_true(have_set_time);

  ret = ioctl(fd, RTC_RD_TIME, (unsigned long)((uintptr_t)&rd_time));
  assert_return_code(ret, OK);

  assert_int_equal(set_time.tm_year, rd_time.tm_year);
  assert_int_equal(set_time.tm_mon, rd_time.tm_mon);
  assert_int_equal(set_time.tm_mday, rd_time.tm_mday);
  assert_int_equal(set_time.tm_wday, rd_time.tm_wday);

  ret = strftime(timbuf, sizeof(timbuf), "%a, %b %d %H:%M:%S %Y",
                 (FAR struct tm *)&rd_time);
  assert_return_code(ret, OK);

  close(fd);
}

#if defined(CONFIG_RTC_ALARM) || defined(CONFIG_RTC_PERIODIC)
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
#endif

#ifdef CONFIG_RTC_ALARM
/****************************************************************************
 * Name: add_timeout
 ****************************************************************************/

static void add_timeout(struct rtc_time * rtc_tm, const int delay)
{
  time_t timesp;
  FAR struct tm *tm;

  timesp = timegm((struct tm *)rtc_tm);
  timesp += delay;

  tm = gmtime(&timesp);
  rtc_tm->tm_sec = tm->tm_sec;
  rtc_tm->tm_min = tm->tm_min;
  rtc_tm->tm_hour = tm->tm_hour;
  rtc_tm->tm_mday = tm->tm_mday;
  rtc_tm->tm_mon = tm->tm_mon;
  rtc_tm->tm_year = tm->tm_year;
  rtc_tm->tm_wday = tm->tm_wday;
  rtc_tm->tm_yday = tm->tm_yday;
  rtc_tm->tm_isdst = tm->tm_isdst;
}

/****************************************************************************
 * Name: drivertest_rtc_alarm
 ****************************************************************************/

static void drivertest_rtc_alarm(FAR void **state)
{
  int fd;
  int ret;
  int alarmid = 0;
  struct rtc_setalarm_s rtc_setalarm;
  struct rtc_rdalarm_s rtc_rdalarm;
  struct rtc_setrelative_s rtc_setrelative;
  struct rtc_time set_time;
  uint32_t before_timestamp;
  uint32_t range;
  sigset_t set;
  FAR struct rtc_state_s *rtc_state =
                  (FAR struct rtc_state_s *)*state;

  signal(RTC_SIGNO, SIG_IGN);
  sigemptyset(&set);
  sigaddset(&set, RTC_SIGNO);

  fd = open(rtc_state->devpath, O_RDWR);
  assert_return_code(fd, 0);

  memset(&set_time, 0, sizeof(set_time));
  set_time.tm_year = 2000 - TM_YEAR_BASE;
  set_time.tm_mon = TM_JANUARY;
  set_time.tm_mday = 1;
  set_time.tm_wday = TM_SATURDAY;

  /* Set rtc alarm */

  rtc_setalarm.id = 0;
  rtc_setalarm.pid = getpid();
  rtc_setalarm.event.sigev_notify = SIGEV_SIGNAL;
  rtc_setalarm.event.sigev_signo = RTC_SIGNO;
  rtc_setalarm.event.sigev_value.sival_ptr = NULL;
  rtc_setalarm.time = set_time;

  add_timeout(&rtc_setalarm.time, DEFAULT_TIME_OUT);

  ret = ioctl(fd, RTC_SET_TIME, (unsigned long)((uintptr_t)&set_time));
  assert_return_code(ret, OK);

  sleep(rtc_state->vendor_delay);
  add_timeout(&rtc_setalarm.time, rtc_state->vendor_delay);

  ret = ioctl(fd, RTC_SET_ALARM, &rtc_setalarm);
  assert_return_code(ret, OK);

  before_timestamp = get_timestamp();
  sleep(rtc_state->vendor_delay);

  /* Read alarm */

  rtc_rdalarm.id = 0;
  ret = ioctl(fd, RTC_RD_ALARM, &rtc_rdalarm);
  assert_return_code(ret, OK);

  assert_int_equal(timegm((struct tm *)&rtc_rdalarm.time),
                   timegm((struct tm *)&rtc_setalarm.time));

  ret = sigwaitinfo(&set, NULL);
  assert_return_code(ret, RTC_SIGNO);

  range = abs(get_timestamp() - before_timestamp);
  assert_in_range(range, DEFAULT_TIME_OUT * 1000 - rtc_state->deviation,
                  DEFAULT_TIME_OUT * 1000 + rtc_state->deviation);

  /* Cancel rtc alarm */

  ret = ioctl(fd, RTC_SET_TIME, (unsigned long)((uintptr_t)&set_time));
  assert_return_code(ret, OK);

  sleep(rtc_state->vendor_delay);

  rtc_setalarm.time = set_time;
  add_timeout(&rtc_setalarm.time, DEFAULT_TIME_OUT);

  ret = ioctl(fd, RTC_SET_ALARM, &rtc_setalarm);
  assert_return_code(ret, OK);

  ret = ioctl(fd, RTC_CANCEL_ALARM, alarmid);
  assert_return_code(ret, OK);

  /* Set relative */

  rtc_setrelative.id = 0;
  rtc_setrelative.pid = getpid();
  rtc_setrelative.event.sigev_notify = SIGEV_SIGNAL;
  rtc_setrelative.event.sigev_signo = RTC_SIGNO;
  rtc_setrelative.event.sigev_value.sival_ptr = NULL;
  rtc_setrelative.reltime = DEFAULT_TIME_OUT;

  ret = ioctl(fd, RTC_SET_RELATIVE, &rtc_setrelative);
  assert_return_code(ret, OK);

  before_timestamp = get_timestamp();

  ret = sigwaitinfo(&set, NULL);
  assert_return_code(ret, RTC_SIGNO);

  range = abs(get_timestamp() - before_timestamp);
  assert_in_range(range, DEFAULT_TIME_OUT * 1000 - rtc_state->deviation,
                  DEFAULT_TIME_OUT * 1000 + rtc_state->deviation);
  close(fd);
}
#endif

#ifdef CONFIG_RTC_PERIODIC
/****************************************************************************
 * Name: rtc_periodic_callback
 ****************************************************************************/

static void rtc_periodic_callback(union sigval arg)
{
  FAR struct rtc_state_s * rtc_state =
    (FAR struct rtc_state_s *)arg.sival_ptr;
  int range = get_timestamp() - rtc_state->tim;
  assert_in_range(range, DEFAULT_TIME_OUT * 1000 - rtc_state->deviation,
                  DEFAULT_TIME_OUT * 1000 + rtc_state->deviation);
  syslog(LOG_DEBUG, "rtc periodic callback trigger!!!\n");
  rtc_state->tim = get_timestamp();
}

/****************************************************************************
 * Name: drivertest_rtc_periodic
 ****************************************************************************/

static void drivertest_rtc_periodic(FAR void **state)
{
  int alarmid = 0;
  int ret;
  int fd;
  struct rtc_setperiodic_s rtc_setperiodic;
  FAR struct rtc_state_s *rtc_state =
                  (FAR struct rtc_state_s *)*state;

  fd = open(rtc_state->devpath, O_RDWR);
  assert_return_code(fd, OK);

  /* Set periodic */

  rtc_setperiodic.id = 0;
  rtc_setperiodic.pid = getpid();
  rtc_setperiodic.event.sigev_notify = SIGEV_THREAD;
  rtc_setperiodic.event.sigev_notify_function = rtc_periodic_callback;
  rtc_setperiodic.event.sigev_notify_attributes = NULL;
  rtc_setperiodic.event.sigev_value.sival_ptr = rtc_state;
  rtc_setperiodic.period.tv_sec = DEFAULT_TIME_OUT;
  rtc_setperiodic.period.tv_nsec = 0;

  ret = ioctl(fd, RTC_SET_PERIODIC, &rtc_setperiodic);
  assert_return_code(ret, OK);
  rtc_state->tim = get_timestamp();
  sleep(SLEEPSECONDS);

  /* Cancel periodic */

  ret = ioctl(fd, RTC_CANCEL_PERIODIC, alarmid);
  assert_return_code(ret, OK);

  sleep(SLEEPSECONDS);
  close(fd);
}
#endif

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
    .deviation = RTC_DEFAULT_DEVIATION,
    .vendor_delay = VENDOR_DELAY,
  };

  parse_commandline(&rtc_state, argc, argv);

  const struct CMUnitTest tests[] =
  {
    cmocka_unit_test_prestate(drivertest_rtc_api, &rtc_state),
#ifdef CONFIG_RTC_ALARM
    cmocka_unit_test_prestate(drivertest_rtc_alarm, &rtc_state),
#endif
#ifdef CONFIG_RTC_PERIODIC
    cmocka_unit_test_prestate(drivertest_rtc_periodic, &rtc_state),
#endif
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
