/****************************************************************************
 * apps/examples/pmstress/pmstress_main.c
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
 * Walk the power management states in a fixed, timed sequence, so that a
 * logging supply can record the current of each state with nothing
 * attached to the board.  A full-speed burst separates the steps, so the
 * trace shows where each one starts.  The last step is the deepest state.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/boardctl.h>
#include <sys/ioctl.h>
#include <sys/param.h>

#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <nuttx/power/pm.h>
#include <nuttx/leds/userled.h>
#include <nuttx/timers/rtc.h>

#ifdef CONFIG_ARCH_CHIP_RP23XX
#  include <arch/chip/pm.h>
#endif

#if defined(BOARDIOC_RP23XX_SUSPEND) && defined(CONFIG_RTC_DRIVER) && \
    defined(CONFIG_RTC_ALARM)
#  define HAVE_SUSPEND
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MARKER_MS      1000    /* Length of the burst between steps */
#define PM_DOMAIN      0       /* PM_IDLE_DOMAIN */

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct pmstress_step_s
{
  FAR const char *name;
  int             state;    /* State to hold the domain at, -1 for none */
  int             seconds;  /* How long, 0 for ever */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct pmstress_step_s g_steps[] =
{
  {
    "active  (full speed, busy)", PM_NORMAL,
    CONFIG_EXAMPLES_PMSTRESS_STEP_SECS
  },
  {
    "idle    (full speed, wfi)", PM_NORMAL,
    CONFIG_EXAMPLES_PMSTRESS_STEP_SECS
  },
  {
    "standby (clocks gated)", PM_STANDBY,
    CONFIG_EXAMPLES_PMSTRESS_STEP_SECS
  },
  {
    "sleep   (deepest state)", -1, 0
  },
};

/* The state this program holds the domain at, or -1 */

static int g_held = -1;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pmstress_control
 ****************************************************************************/

static int pmstress_control(int action, int state)
{
  struct boardioc_pm_ctrl_s ctrl;

  memset(&ctrl, 0, sizeof(ctrl));
  ctrl.action = action;
  ctrl.domain = PM_DOMAIN;
  ctrl.state  = state;

  return boardctl(BOARDIOC_PM_CONTROL, (uintptr_t)&ctrl);
}

/****************************************************************************
 * Name: pmstress_hold
 *
 * Description:
 *   Hold the idle domain at a state, or release it with -1.  The governor
 *   does not go below a state that has a wakelock.
 *
 ****************************************************************************/

static void pmstress_hold(int state)
{
  if (g_held >= 0)
    {
      pmstress_control(BOARDIOC_PM_RELAX, g_held);
      g_held = -1;
    }

  if (state >= 0)
    {
      if (pmstress_control(BOARDIOC_PM_STAY, state) == OK)
        {
          g_held = state;
        }
      else
        {
          printf("pmstress: failed to hold state %d: %d\n", state, errno);
        }
    }
}

/****************************************************************************
 * Name: pmstress_busy
 *
 * Description:
 *   Run at full speed, with no idle time, for the given time.
 *
 ****************************************************************************/

static void pmstress_busy(long ms)
{
  struct timespec start;
  struct timespec now;
  volatile uint32_t sink = 0;
  long elapsed;
  int i;

  clock_gettime(CLOCK_MONOTONIC, &start);

  do
    {
      for (i = 0; i < 10000; i++)
        {
          sink = sink + i;
        }

      pmstress_control(BOARDIOC_PM_ACTIVITY, 0);

      clock_gettime(CLOCK_MONOTONIC, &now);
      elapsed = (now.tv_sec - start.tv_sec) * 1000 +
                (now.tv_nsec - start.tv_nsec) / 1000000;
    }
  while (elapsed < ms);
}

/****************************************************************************
 * Name: pmstress_leds_off
 *
 * Description:
 *   Turn the user LEDs off.  A LED draws milliamps, more than a low power
 *   state.
 *
 ****************************************************************************/

static void pmstress_leds_off(bool verbose)
{
#ifdef CONFIG_USERLED_LOWER
  userled_set_t ledset = 0;
  int fd;

  fd = open(CONFIG_EXAMPLES_PMSTRESS_LEDPATH, O_WRONLY);
  if (fd < 0)
    {
      if (verbose)
        {
          printf("pmstress: no %s, LEDs left as they are: %d\n",
                 CONFIG_EXAMPLES_PMSTRESS_LEDPATH, errno);
        }

      return;
    }

  if (ioctl(fd, ULEDIOC_SETALL, (unsigned long)ledset) < 0)
    {
      if (verbose)
        {
          printf("pmstress: could not clear the LEDs: %d\n", errno);
        }
    }
  else if (verbose)
    {
      printf("pmstress: LEDs driven off\n");
    }

  close(fd);
#else
  if (verbose)
    {
      printf("pmstress: no user LED driver, check the LED is not lit\n");
    }
#endif
}

#ifdef HAVE_SUSPEND
/****************************************************************************
 * Name: pmstress_suspend
 *
 * Description:
 *   Suspend to RAM until an RTC alarm in the given time.  The alarm always
 *   returns the board, even if no other wake source is set up.
 *
 ****************************************************************************/

static int pmstress_suspend(int seconds)
{
  struct rp23xx_suspend_s suspend;
  struct rtc_setrelative_s alarm;
  int ret;
  int fd;

  pmstress_leds_off(true);

  printf("pmstress: suspending to RAM for %d s\n", seconds);
  fflush(stdout);

  /* Let the console drain: the UART loses its registers */

  sleep(1);

  memset(&alarm, 0, sizeof(alarm));
  alarm.event.sigev_notify = SIGEV_NONE;
  alarm.reltime            = seconds;

  fd = open("/dev/rtc0", O_RDONLY);
  if (fd < 0)
    {
      printf("pmstress: cannot open /dev/rtc0: %d\n", errno);
      return EXIT_FAILURE;
    }

  ret = ioctl(fd, RTC_SET_RELATIVE, (unsigned long)&alarm);
  close(fd);

  if (ret < 0)
    {
      printf("pmstress: cannot set the RTC alarm: %d\n", errno);
      return EXIT_FAILURE;
    }

  suspend.wake_ms     = 0;
  suspend.wake_source = 0;

  ret = boardctl(BOARDIOC_RP23XX_SUSPEND, (uintptr_t)&suspend);

  printf("pmstress: back, returned %d, wake source 0x%08" PRIx32 "\n",
         ret < 0 ? -errno : ret, suspend.wake_source);

  return ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  size_t i;
  int total;

#ifdef HAVE_SUSPEND
  if (argc > 1 && strcmp(argv[1], "suspend") == 0)
    {
      return pmstress_suspend(argc > 2 ? atoi(argv[2]) : 10);
    }
#endif

  printf("\npmstress: power management current staircase\n\n");

  pmstress_leds_off(true);

  /* Print the schedule now: the console is unplugged during the run */

  total = 0;
  printf("pmstress: schedule, %d ms full-speed burst between each step\n",
         MARKER_MS);

  for (i = 0; i < nitems(g_steps); i++)
    {
      if (g_steps[i].seconds > 0)
        {
          total += g_steps[i].seconds + (MARKER_MS / 1000);
          printf("  t+%4ds  %s, %ds\n", total - g_steps[i].seconds,
                 g_steps[i].name, g_steps[i].seconds);
        }
      else
        {
          total += MARKER_MS / 1000;
          printf("  t+%4ds  %s, until power off\n", total,
                 g_steps[i].name);
        }
    }

  printf("\npmstress: starting in %d seconds, disconnect now\n\n",
         CONFIG_EXAMPLES_PMSTRESS_DISCONNECT_SECS);
  fflush(stdout);

  /* Hold PM_NORMAL during the wait: the deepest state can stop the system
   * tick, and then the sleep would not end.
   */

  pmstress_hold(PM_NORMAL);
  sleep(CONFIG_EXAMPLES_PMSTRESS_DISCONNECT_SECS);

  for (i = 0; i < nitems(g_steps); i++)
    {
      pmstress_hold(PM_NORMAL);
      pmstress_busy(MARKER_MS);

      pmstress_hold(g_steps[i].state);

      /* A board LED callback can turn the LED on again in PM_NORMAL */

      pmstress_leds_off(false);

      if (g_steps[i].seconds == 0)
        {
          for (; ; )
            {
              sleep(3600);
            }
        }
      else if (i == 0)
        {
          pmstress_busy(g_steps[i].seconds * 1000L);
        }
      else
        {
          sleep(g_steps[i].seconds);
        }
    }

  return EXIT_SUCCESS;
}
