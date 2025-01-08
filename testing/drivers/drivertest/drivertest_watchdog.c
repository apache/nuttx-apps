/****************************************************************************
 * apps/testing/drivers/drivertest/drivertest_watchdog.c
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

#include <sys/boardctl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <debug.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <time.h>

#include <nuttx/arch.h>
#include <nuttx/timers/watchdog.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define WDG_DEFAULT_DEV_PATH "/dev/watchdog0"
#define WDG_DEFAULT_PINGTIMER 5000
#define WDG_DEFAULT_PINGDELAY 500
#define WDG_DEFAULT_TIMEOUT 2000
#define WDG_DEFAULT_TESTCASE 0
#define WDG_DEFAULT_DEVIATION 20
#define WDG_COUNT_TESTCASE 4

#define OPTARG_TO_VALUE(value, type, base)                            \
  do                                                                  \
    {                                                                 \
      FAR char *ptr;                                                  \
      value = (type)strtoul(optarg, &ptr, base);                      \
      if (*ptr != '\0')                                               \
        {                                                             \
          printf("Parameter error: -%c %s\n", ch, optarg);            \
          show_usage(argv[0], wdg_state, EXIT_FAILURE);               \
        }                                                             \
    } while (0)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct wdg_state_s
{
  char devpath[PATH_MAX];
  uint32_t pingtime;
  uint32_t pingdelay;
  uint32_t timeout;
  uint32_t deviation;
  int test_case;
  bool test_getstatus;
};

static sem_t g_semaphore;

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
 * Name: get_time_elaps
 ****************************************************************************/

static uint32_t get_time_elaps(uint32_t prev_tick)
{
  uint32_t act_time = get_timestamp();

  /* If there is no overflow in sys_time simple subtract */

  if (act_time >= prev_tick)
    {
      prev_tick = act_time - prev_tick;
    }
  else
    {
      prev_tick = UINT32_MAX - prev_tick + 1;
      prev_tick += act_time;
    }

  return prev_tick;
}

/****************************************************************************
 * Name: wdg_init
 ****************************************************************************/

static int wdg_init(FAR struct wdg_state_s *state)
{
  int dev_fd;
  int ret;

  /* Open the watchdog device for reading */

  dev_fd = open(state->devpath, O_RDONLY);
  assert_true(dev_fd > 0);

  /* Set the watchdog timeout */

  ret = ioctl(dev_fd, WDIOC_SETTIMEOUT, state->timeout);
  assert_return_code(ret, OK);

  /* Then start the watchdog timer. */

  ret = ioctl(dev_fd, WDIOC_START, 0);
  assert_return_code(ret, OK);

  return dev_fd;
}

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname,
                       FAR struct wdg_state_s *wdg_state, int exitcode)
{
  printf("Usage: %s -d <devpath> -r <test case> -t <pingtime>"
         "-l <pingdelay> -o <timeout> -a <deviation> -g\n", progname);
  printf("  [-d devpath] selects the WATCHDOG device.\n"
         "  Default: %s Current: %s\n", WDG_DEFAULT_DEV_PATH,
         wdg_state->devpath);
  printf("  [-r test_case] selects the testcase.\n"
         "  Default: %d Current: %d\n", WDG_DEFAULT_TESTCASE,
         wdg_state->test_case);
  printf("  [-t pingtime] Selects the <delay> time in milliseconds.\n"
         "  Default: %d Current: %" PRIu32 "\n",
         WDG_DEFAULT_PINGTIMER, wdg_state->pingtime);
  printf("  [-l pingdelay] Time delay between pings in milliseconds.\n"
         "  Default:  %d Current: %" PRIu32 "\n",
         WDG_DEFAULT_PINGDELAY, wdg_state->pingdelay);
  printf("  [-o timeout] Time in milliseconds that the testcase will\n"
         "  Default: %d Current: %" PRIu32 "\n",
         WDG_DEFAULT_TIMEOUT, wdg_state->timeout);
  printf("  [-a deviation] Watchdog getstatus precision.\n"
         "  Default: %d Current: %" PRIu32 "\n",
         WDG_DEFAULT_DEVIATION, wdg_state->deviation);
  printf("  [-g] don't test getstatus\n");
  printf("  [-h] = Shows this message and exits\n");

  exit(exitcode);
}

/****************************************************************************
 * Name: parse_commandline
 ****************************************************************************/

static void parse_commandline(FAR struct wdg_state_s *wdg_state, int argc,
                              FAR char **argv)
{
  int ch;
  int converted;

  while ((ch = getopt(argc, argv, "d:r:t:l:o:a:gh")) != ERROR)
    {
      switch (ch)
        {
          case 'd':
            strlcpy(wdg_state->devpath, optarg, sizeof(wdg_state->devpath));
            break;

          case 'r':
            OPTARG_TO_VALUE(converted, uint32_t, 10);
            if (converted < WDG_DEFAULT_TESTCASE ||
                converted >= WDG_COUNT_TESTCASE)
              {
                printf("signal out of range: %d\n", converted);
                show_usage(argv[0], wdg_state, EXIT_FAILURE);
              }

            wdg_state->test_case = converted;
            break;

          case 't':
            OPTARG_TO_VALUE(converted, uint32_t, 10);
            if (converted < 1 || converted > INT_MAX)
              {
                printf("signal out of range: %d\n", converted);
                show_usage(argv[0], wdg_state, EXIT_FAILURE);
              }

            wdg_state->pingtime = (uint32_t)converted;
            break;

          case 'l':
            OPTARG_TO_VALUE(converted, uint32_t, 10);
            if (converted < 1 || converted > INT_MAX)
              {
                printf("signal out of range: %d\n", converted);
                show_usage(argv[0], wdg_state, EXIT_FAILURE);
              }

            wdg_state->pingdelay = (uint32_t)converted;
            break;

          case 'o':
            OPTARG_TO_VALUE(converted, uint32_t, 10);
            if (converted < 1 || converted > INT_MAX)
              {
                printf("signal out of range: %d\n", converted);
                show_usage(argv[0], wdg_state, EXIT_FAILURE);
              }

            wdg_state->timeout = (uint32_t)converted;
            break;

          case 'a':
            OPTARG_TO_VALUE(converted, uint32_t, 10);
            if (converted < 1 || converted > INT_MAX)
              {
                printf("signal out of range: %d\n", converted);
                show_usage(argv[0], wdg_state, EXIT_FAILURE);
              }

            wdg_state->deviation = (uint32_t)converted;

          case 'g':
            wdg_state->test_getstatus = false;
            break;

          case '?':
            printf("Unsupported option: %s\n", optarg);
            show_usage(argv[0], wdg_state, EXIT_FAILURE);
            break;
        }
    }
}

/****************************************************************************
 * Name: capture_callback
 ****************************************************************************/

static int capture_callback(int irq, FAR void *context, FAR void *arg)
{
  sem_post(&g_semaphore);
  return OK;
}

/****************************************************************************
 * Name: drivertest_watchdog_feeding
 *
 * Description:
 *   This function is used to test whether the watchdog can take effect after
 *   timeout without relying on automatic feeding
 ****************************************************************************/

static void drivertest_watchdog_feeding(FAR void **state)
{
  int dev_fd;
  int ret;
  uint32_t start_ms;
  FAR struct wdg_state_s *wdg_state;
  struct boardioc_reset_cause_s reset_cause;

  wdg_state = (FAR struct wdg_state_s *)*state;

  /* If it's not the first step, skip... */

  if (wdg_state->test_case != 0)
    {
      return;
    }

  boardctl(BOARDIOC_RESET_CAUSE, (uintptr_t)&reset_cause);
  assert_int_equal(reset_cause.cause, BOARDIOC_RESETCAUSE_SYS_CHIPPOR);

  dev_fd = wdg_init(wdg_state);

  /* Get the starting time */

  start_ms = get_timestamp();

  /* Then ping */

  while (get_time_elaps(start_ms) < wdg_state->pingtime)
    {
      /* Sleep for the requested amount of time */

      up_udelay(wdg_state->pingdelay * 1000);

      /* Then ping */

      ret = ioctl(dev_fd, WDIOC_KEEPALIVE, 0);
      assert_return_code(ret, OK);
    }

  /* Then stop pinging */

  /* Sleep for the requested amount of time, use up_udelay prevent system
   * into low power mode and watchdog be pause cannot trigger restart.
   */

  up_udelay(2 * wdg_state->timeout * 1000);

  assert_true(false);
}

/****************************************************************************
 * Name: drivertest_watchdog_interrupts
 *
 * Description:
 *   This function is used to test whether the watchdog takes effect when
 *   the interrupt is turned off.
 ****************************************************************************/

static void drivertest_watchdog_interrupts(FAR void **state)
{
  FAR struct wdg_state_s *wdg_state;
  struct boardioc_reset_cause_s reset_cause;

  wdg_state = (FAR struct wdg_state_s *)*state;

  if (wdg_state->test_case != 1)
    {
      return;
    }

  boardctl(BOARDIOC_RESET_CAUSE, (uintptr_t)&reset_cause);
  assert_int_equal(reset_cause.cause, BOARDIOC_RESETCAUSE_SYS_RWDT);

  wdg_init(wdg_state);

  enter_critical_section();

  /* Busy loop when disable interrupts */

  while (1);
}

/****************************************************************************
 * Name: wdg_wdentry
 ****************************************************************************/

static void wdg_wdentry(wdparm_t arg)
{
  /* Busy loop in interrupts */

  while (1);
}

/****************************************************************************
 * Name: drivertest_watchdog_loop
 *
 * Description:
 *   This function is used to test whether the infinite loop will start
 *   watchdog after opening the interrupt.
 ****************************************************************************/

static void drivertest_watchdog_loop(FAR void **state)
{
  int ret;
  static struct wdog_s wdog;
  FAR struct wdg_state_s *wdg_state;
  struct boardioc_reset_cause_s reset_cause;

  wdg_state = (FAR struct wdg_state_s *)*state;

  if (wdg_state->test_case != 2)
    {
      return;
    }

  boardctl(BOARDIOC_RESET_CAUSE, (uintptr_t)&reset_cause);
  assert_int_equal(reset_cause.cause, BOARDIOC_RESETCAUSE_SYS_RWDT);

  wdg_init(wdg_state);

  ret = wd_start(&wdog, 1, wdg_wdentry, (wdparm_t)0);
  assert_return_code(ret, OK);

  /* Waiting for the reset... */

  while (1)
    {
      sleep(1);
    }
}

/****************************************************************************
 * Name: drivertest_watchdog_api
 *
 * Description:
 *   This function is used to test the watchdog driver interface.
 ****************************************************************************/

static void drivertest_watchdog_api(FAR void **state)
{
  int dev_fd;
  int ret;
  uint32_t start_ms;
  FAR struct wdg_state_s *wdg_state;
  struct watchdog_status_s status;
  struct boardioc_reset_cause_s reset_cause;
  struct watchdog_capture_s watchdog_capture;

  wdg_state = (FAR struct wdg_state_s *)*state;

  assert_int_equal(wdg_state->test_case, 3);

  boardctl(BOARDIOC_RESET_CAUSE, (uintptr_t)&reset_cause);
  assert_int_equal(reset_cause.cause, BOARDIOC_RESETCAUSE_SYS_RWDT);

  dev_fd = wdg_init(wdg_state);

  /* Get the starting time */

  start_ms = get_timestamp();

  /* Then ping */

  while (get_time_elaps(start_ms) < wdg_state->pingtime)
    {
      /* Sleep for the requested amount of time, use up_udelay prevent
       * system into low power mode and watchdog be pause stop count.
       */

      up_udelay(wdg_state->pingdelay * 1000);

      if (wdg_state->test_getstatus)
        {
          /* Get Status */

          ret = ioctl(dev_fd, WDIOC_GETSTATUS, &status);
          assert_return_code(ret, OK);

          assert_int_equal(status.timeout, wdg_state->timeout);
          assert_in_range(
            status.timeout - status.timeleft,
            wdg_state->pingdelay - wdg_state->deviation,
            wdg_state->pingdelay + wdg_state->deviation);
        }

      /* Then ping */

      ret = ioctl(dev_fd, WDIOC_KEEPALIVE, 0);
      assert_return_code(ret, OK);
    }

  /* Test capture. */

  ret = sem_init(&g_semaphore, 0, 0);
  assert_return_code(ret, OK);

  watchdog_capture.newhandler = capture_callback;
  ret = ioctl(dev_fd, WDIOC_CAPTURE, &watchdog_capture);
  assert_return_code(ret, OK);

  /* Prevent the os entering pm then turn off watchdog. */

  up_udelay(2 * wdg_state->timeout * 1000);

  sem_wait(&g_semaphore);
  sem_destroy(&g_semaphore);

  watchdog_capture.newhandler = watchdog_capture.oldhandler;
  ret = ioctl(dev_fd, WDIOC_CAPTURE, &watchdog_capture);
  assert_return_code(ret, OK);

  /* Then stop pinging */

  ret = ioctl(dev_fd, WDIOC_STOP, 0);
  assert_return_code(ret, OK);

  ret = close(dev_fd);
  assert_return_code(ret, OK);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct wdg_state_s wdg_state =
  {
    .devpath = WDG_DEFAULT_DEV_PATH,
    .pingtime = WDG_DEFAULT_PINGTIMER,
    .pingdelay = WDG_DEFAULT_PINGDELAY,
    .timeout = WDG_DEFAULT_TIMEOUT,
    .test_case = WDG_DEFAULT_TESTCASE,
    .deviation = WDG_DEFAULT_DEVIATION,
    .test_getstatus = true
  };

  parse_commandline(&wdg_state, argc, argv);

  const struct CMUnitTest tests[] =
  {
    cmocka_unit_test_prestate(drivertest_watchdog_feeding, &wdg_state),
    cmocka_unit_test_prestate(drivertest_watchdog_interrupts, &wdg_state),
    cmocka_unit_test_prestate(drivertest_watchdog_loop, &wdg_state),
    cmocka_unit_test_prestate(drivertest_watchdog_api, &wdg_state)
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
