/****************************************************************************
 * apps/testing/drivertest/drivertest_watchdog.c
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

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/boardctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>
#include <inttypes.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>

#include <time.h>
#include <nuttx/timers/watchdog.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define WDG_DEFAULT_DEV_PATH "/dev/watchdog0"
#define WDG_DEFAULT_INFO_PATH "/data/wdg_info"
#define WDG_DEFAULT_PINGTIMER 5000
#define WDG_DEFAULT_PINGDELAY 500
#define WDG_DEFAULT_TIMEOUT 2000

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

enum wdg_reset_cause_e
{
  WDG_RESET_CAUSE_NONE,
  WDG_RESET_CAUSE_NO_KEEP_ALIVE,
  WDG_RESET_CAUSE_CRITICAL_SECTION_BUSY_LOOP,
  WDG_RESET_CAUSE_IRQ_BUSY_LOOP
};

struct wdg_state_s
{
  char devpath[PATH_MAX];
  char infopath[PATH_MAX];
  uint32_t pingtime;
  uint32_t pingdelay;
  uint32_t timeout;
  enum wdg_reset_cause_e reset_cause;
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
 * Name: wdg_write_reset_cause
 ****************************************************************************/

static void wdg_write_reset_cause(FAR struct wdg_state_s *state)
{
  int fd;
  ssize_t size;

  fd = open(state->infopath, O_WRONLY | O_CREAT);
  assert_true(fd > 0);

  size = write(fd, &state->reset_cause, sizeof(state->reset_cause));
  assert_int_equal(size, sizeof(state->reset_cause));

  close(fd);
}

/****************************************************************************
 * Name: wdg_read_reset_cause
 ****************************************************************************/

static void wdg_read_reset_cause(FAR struct wdg_state_s *state)
{
  int fd;
  ssize_t size;

  fd = open(state->infopath, O_RDONLY);
  if (fd < 0)
    {
      state->reset_cause = WDG_RESET_CAUSE_NONE;
      return;
    }

  size = read(fd, &state->reset_cause, sizeof(state->reset_cause));
  assert_int_equal(size, sizeof(state->reset_cause));

  close(fd);
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
  printf("Usage: %s -d <devpath> -p <infopath> -t <pingtime>"
         "-l <pingdelay> -o <timeout>\n", progname);
  printf("  [-d devpath] selects the WATCHDOG device.\n"
         "  Default: %s Current: %s\n", WDG_DEFAULT_DEV_PATH,
         wdg_state->devpath);
  printf("  [-p infopath] selects the WATCHDOG reset cause path.\n"
         "  Default: %s Current: %s\n", WDG_DEFAULT_INFO_PATH,
         wdg_state->infopath);
  printf("  [-t pingtime] Selects the <delay> time in milliseconds.\n"
         "  Default: %d Current: %" PRIu32 "\n",
         WDG_DEFAULT_PINGTIMER, wdg_state->pingtime);
  printf("  [-l pingdelay] Time delay between pings in milliseconds.\n"
         "  Default:  %d Current: %" PRIu32 "\n",
         WDG_DEFAULT_PINGDELAY, wdg_state->pingdelay);
  printf("  [-o timeout] Time in milliseconds that the testcase will\n"
         "  Default: %d Current: %" PRIu32 "\n",
         WDG_DEFAULT_TIMEOUT, wdg_state->timeout);
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

  while ((ch = getopt(argc, argv, "d:p:t:l:o:h")) != ERROR)
    {
      switch (ch)
        {
          case 'd':
            strncpy(wdg_state->devpath, optarg, sizeof(wdg_state->devpath));
            wdg_state->devpath[sizeof(wdg_state->devpath) - 1] = '\0';
            break;

          case 'p':
            strncpy(wdg_state->infopath, optarg,
                    sizeof(wdg_state->infopath));
            wdg_state->infopath[sizeof(wdg_state->infopath) - 1] = '\0';
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
                printf("signal out of range: %d", converted);
                show_usage(argv[0], wdg_state, EXIT_FAILURE);
              }

            wdg_state->timeout = (uint32_t)converted;
            break;

          case '?':
            printf("Unsupported option: %s\n", optarg);
            show_usage(argv[0], wdg_state, EXIT_FAILURE);
            break;
        }
    }
}

/****************************************************************************
 * Name: test_case_wdog_01
 *
 * Description:
 *   This function is used to test whether the watchdog can take effect after
 *   timeout without relying on automatic feeding
 ****************************************************************************/

static void test_case_wdog_01(FAR void **state)
{
  int dev_fd;
  int ret;
  uint32_t start_ms;
  FAR struct wdg_state_s *wdg_state;

  wdg_state = (FAR struct wdg_state_s *)*state;

  wdg_read_reset_cause(wdg_state);

  /* If it's not the first step, skip... */

  if (wdg_state->reset_cause != WDG_RESET_CAUSE_NONE)
    {
      return;
    }

  dev_fd = wdg_init(wdg_state);

  /* Get the starting time */

  start_ms = get_timestamp();

  /* Then ping */

  while (get_time_elaps(start_ms) < wdg_state->pingtime)
    {
      /* Sleep for the requested amount of time */

      usleep(wdg_state->pingdelay * 1000);

      /* Then ping */

      ret = ioctl(dev_fd, WDIOC_KEEPALIVE, 0);
      assert_return_code(ret, OK);
    }

  /* write the reset cause */

  wdg_state->reset_cause = WDG_RESET_CAUSE_NO_KEEP_ALIVE;
  wdg_write_reset_cause(wdg_state);

  /* Then stop pinging */

  /* Sleep for the requested amount of time */

  usleep(2 * wdg_state->timeout * 1000);

  assert_true(false);
}

/****************************************************************************
 * Name: test_case_wdog_02
 *
 * Description:
 *   This function is used to test whether the watchdog takes effect when
 *   the interrupt is turned off.
 ****************************************************************************/

static void test_case_wdog_02(FAR void **state)
{
  FAR struct wdg_state_s *wdg_state;

  wdg_state = (FAR struct wdg_state_s *)*state;

  wdg_read_reset_cause(wdg_state);

  if (wdg_state->reset_cause != WDG_RESET_CAUSE_NO_KEEP_ALIVE)
    {
      return;
    }

  wdg_init(wdg_state);

  wdg_state->reset_cause = WDG_RESET_CAUSE_CRITICAL_SECTION_BUSY_LOOP;
  wdg_write_reset_cause(wdg_state);

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
 * Name: test_case_wdog_03
 *
 * Description:
 *   This function is used to test whether the infinite loop will start
 *   watchdog after opening the interrupt.
 ****************************************************************************/

static void test_case_wdog_03(FAR void **state)
{
  int ret;
  static struct wdog_s wdog;
  FAR struct wdg_state_s *wdg_state;

  wdg_state = (FAR struct wdg_state_s *)*state;

  wdg_read_reset_cause(wdg_state);

  if (wdg_state->reset_cause != WDG_RESET_CAUSE_CRITICAL_SECTION_BUSY_LOOP)
    {
      return;
    }

  wdg_init(wdg_state);

  wdg_state->reset_cause = WDG_RESET_CAUSE_IRQ_BUSY_LOOP;
  wdg_write_reset_cause(wdg_state);

  ret = wd_start(&wdog, 1, wdg_wdentry, (wdparm_t)0);
  assert_return_code(ret, OK);

  /* Waiting for the reset... */

  while (1)
    {
      sleep(1);
    }
}

/****************************************************************************
 * Name: test_case_wdog_04
 *
 * Description:
 *   This function is used to test the watchdog driver interface.
 ****************************************************************************/

static void test_case_wdog_04(FAR void **state)
{
  int dev_fd;
  int ret;
  uint32_t start_ms;
  FAR struct wdg_state_s *wdg_state;

  wdg_state = (FAR struct wdg_state_s *)*state;

  wdg_read_reset_cause(wdg_state);
  assert_int_equal(wdg_state->reset_cause, WDG_RESET_CAUSE_IRQ_BUSY_LOOP);

  remove(wdg_state->infopath);

  dev_fd = wdg_init(wdg_state);

  /* Get the starting time */

  start_ms = get_timestamp();

  /* Then ping */

  while (get_time_elaps(start_ms) < wdg_state->pingtime)
    {
      /* Sleep for the requested amount of time */

      usleep(wdg_state->pingdelay * 1000);

      /* Then ping */

      ret = ioctl(dev_fd, WDIOC_KEEPALIVE, 0);
      assert_return_code(ret, OK);
    }

  /* Then stop pinging */

  ret = ioctl(dev_fd, WDIOC_STOP, 0);
  assert_return_code(ret, OK);

  close(dev_fd);
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
    .infopath = WDG_DEFAULT_INFO_PATH,
    .pingtime = WDG_DEFAULT_PINGTIMER,
    .pingdelay = WDG_DEFAULT_PINGDELAY,
    .timeout = WDG_DEFAULT_TIMEOUT
  };

  parse_commandline(&wdg_state, argc, argv);

  const struct CMUnitTest tests[] =
  {
    cmocka_unit_test_prestate(test_case_wdog_01, &wdg_state),
    cmocka_unit_test_prestate(test_case_wdog_02, &wdg_state),
    cmocka_unit_test_prestate(test_case_wdog_03, &wdg_state),
    cmocka_unit_test_prestate(test_case_wdog_04, &wdg_state)
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
