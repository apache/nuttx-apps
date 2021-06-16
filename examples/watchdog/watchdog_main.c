/****************************************************************************
 * apps/examples/watchdog/watchdog_main.c
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/timers/watchdog.h>

#include "watchdog.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DEVNAME_SIZE            16

/* Number of timeout expirations to change mode to reset the chip */

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct wdog_example_s
{
  uint32_t pingtime;
  uint32_t pingdelay;
  uint32_t timeout;
  char devname[DEVNAME_SIZE];
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: wdog_help
 ****************************************************************************/

static void wdog_help(void)
{
  printf("Usage: wdog [-h] [-d <pingtime] [-p <pingdelay>]\
  [-t <timeout>]\n");
  printf("\nInitialize the watchdog to the <timeout>. Start the watchdog\n");
  printf("timer.  Ping for the watchdog for <pingtime> seconds\n");
  printf("then let it expire.\n");
  printf("\nOptions include:\n");
  printf("  [-d <pingtime>] = Selects the <delay> time in milliseconds.\n");
  printf("Default: %d\n", CONFIG_EXAMPLES_WATCHDOG_PINGTIME);
  printf("  [-i </dev/watchdogx>] = Selects the watchdog timer instance.\n");
  printf("Default: %s\n", CONFIG_EXAMPLES_WATCHDOG_DEVPATH);
  printf("  [-p <pingdelay] = Time delay between pings in milliseconds.\n");
  printf("Default: %d\n", CONFIG_EXAMPLES_WATCHDOG_PINGDELAY);
  printf("  [-t timeout] = Time in milliseconds that the example will\n");
  printf("ping the watchdog before letting the watchdog expire.\n");
  printf("Default: %d\n", CONFIG_EXAMPLES_WATCHDOG_TIMEOUT);
  printf("  [-h] = Shows this message and exits\n");
}

/****************************************************************************
 * Name: arg_string
 ****************************************************************************/

static int arg_string(FAR char **arg, FAR char **value)
{
  FAR char *ptr = *arg;

  if (ptr[2] == '\0')
    {
      *value = arg[1];
      return 2;
    }
  else
    {
      *value = &ptr[2];
      return 1;
    }
}

/****************************************************************************
 * Name: arg_decimal
 ****************************************************************************/

static int arg_decimal(FAR char **arg, FAR long *value)
{
  FAR char *string;
  int ret;

  ret = arg_string(arg, &string);
  *value = strtol(string, NULL, 10);
  return ret;
}

/****************************************************************************
 * Name: parse_args
 ****************************************************************************/

static void parse_args(FAR struct wdog_example_s *wdog, int argc,
                       FAR char **argv)
{
  FAR char *ptr;
  FAR char *string;
  long value;
  int index;
  int nargs;

  wdog->pingtime  = CONFIG_EXAMPLES_WATCHDOG_PINGTIME;
  wdog->pingdelay = CONFIG_EXAMPLES_WATCHDOG_PINGDELAY;
  wdog->timeout   = CONFIG_EXAMPLES_WATCHDOG_TIMEOUT;
  strcpy(wdog->devname, CONFIG_EXAMPLES_WATCHDOG_DEVPATH);

  for (index = 1; index < argc; )
    {
      ptr = argv[index];
      if (ptr[0] != '-')
        {
          printf("Invalid options format: %s\n", ptr);
          exit(EXIT_SUCCESS);
        }

      switch (ptr[1])
        {
          case 'd':
            nargs = arg_decimal(&argv[index], &value);
            if (value < 1)
              {
                printf("Ping delay out of range: %ld\n", value);
                exit(EXIT_FAILURE);
              }

            wdog->pingdelay = (uint32_t)value;
            index += nargs;
            break;

          case 'i':
            nargs = arg_string(&argv[index], &string);
            strcpy(wdog->devname, string);
            index += nargs;
            break;

          case 'p':
            nargs = arg_decimal(&argv[index], &value);
            if (value < 1 || value > INT_MAX)
              {
                printf("Ping time out of range: %ld\n", value);
                exit(EXIT_FAILURE);
              }

            wdog->pingtime = (uint32_t)value;
            index += nargs;
            break;

          case 't':
            nargs = arg_decimal(&argv[index], &value);
            if (value < 1 || value > INT_MAX)
              {
                printf("Duration out of range: %ld\n", value);
                exit(EXIT_FAILURE);
              }

            wdog->timeout = (int)value;
            index += nargs;
            break;

          case 'h':
            wdog_help();
            exit(EXIT_SUCCESS);

          default:
            printf("Unsupported option: %s\n", ptr);
            wdog_help();
            exit(EXIT_FAILURE);
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: wdog_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct wdog_example_s wdog;
#ifdef CONFIG_DEBUG_WATCHDOG
  struct watchdog_status_s status;
#endif
  int fd;
  int ret;
  uint64_t elapsed;
  uint64_t start_ms;
  uint64_t current_time_ms;
  struct timespec tstart;
  struct timespec tnow;

  /* Parse the command line */

  parse_args(&wdog, argc, argv);

  /* Open the watchdog device for reading */

  fd = open(wdog.devname, O_RDONLY);
  if (fd < 0)
    {
      printf("wdog_main: open %s failed: %d\n",
             wdog.devname, errno);
      goto errout;
    }

  /* Set the watchdog timeout */

  ret = ioctl(fd, WDIOC_SETTIMEOUT, (unsigned long)wdog.timeout);
  if (ret < 0)
    {
      printf("wdog_main: ioctl(WDIOC_SETTIMEOUT) failed: %d\n", errno);
      goto errout_with_dev;
    }

  /* Then start the watchdog timer. */

  ret = ioctl(fd, WDIOC_START, 0);
  if (ret < 0)
    {
      printf("wdog_main: ioctl(WDIOC_START) failed: %d\n", errno);
      goto errout_with_dev;
    }

  /* Get the starting time */

  clock_gettime(CLOCK_REALTIME, &tstart);
  start_ms = (tstart.tv_sec * 1000) + (tstart.tv_nsec / 1000000);

  /* Then ping */

  for (elapsed = 0; elapsed < wdog.pingtime;
       elapsed = current_time_ms - start_ms)
    {
      /* Sleep for the requested amount of time */

      usleep((wdog.pingdelay * 1000) - CONFIG_USEC_PER_TICK);

      /* Show watchdog status.  Only if debug is enabled because this
       * could interfere with the timer.
       */

#ifdef CONFIG_DEBUG_WATCHDOG
     ret = ioctl(fd, WDIOC_GETSTATUS, (unsigned long)&status);
      if (ret < 0)
        {
          printf("wdog_main: ioctl(WDIOC_GETSTATUS) failed: %d\n", errno);
          goto errout_with_dev;
        }

      printf("wdog_main:"
             " flags=%08" PRIu32
             " timeout=%" PRIu32
             " timeleft=%" PRIu32 "\n",
             status.flags, status.timeout, status.timeleft);
#endif

      /* Then ping */

     ret = ioctl(fd, WDIOC_KEEPALIVE, 0);
      if (ret < 0)
        {
          printf("wdog_main: ioctl(WDIOC_KEEPALIVE) failed: %d\n", errno);
          goto errout_with_dev;
        }

      printf("  ping elapsed=%" PRIu64 "\n", elapsed);
      fflush(stdout);

      /* Get current time to calculate the elapsed time */

      clock_gettime(CLOCK_REALTIME, &tnow);
      current_time_ms = (uint64_t)((tnow.tv_sec * 1000)
                                    + (tnow.tv_nsec / 1000000));
    }

  /* Then stop pinging */

  for (; ; elapsed = current_time_ms - start_ms)
    {
      /* Sleep for the requested amount of time */

      usleep((wdog.pingdelay * 1000) - CONFIG_USEC_PER_TICK);

      /* Show watchdog status.  Only if debug is enabled because this
       * could interfere with the timer.
       */

#ifdef CONFIG_DEBUG_WATCHDOG
     ret = ioctl(fd, WDIOC_GETSTATUS, (unsigned long)&status);
      if (ret < 0)
        {
          printf("wdog_main: ioctl(WDIOC_GETSTATUS) failed: %d\n", errno);
          goto errout_with_dev;
        }

      printf("wdog_main:"
             " flags=%08" PRIu32
             " timeout=%" PRIu32
             " timeleft=%" PRIu32 "\n",
             status.flags, status.timeout, status.timeleft);
#endif

      printf("  NO ping elapsed=%" PRIu64 "\n", elapsed);
      fflush(stdout);

      /* Get current time to calculate the elapsed time */

      clock_gettime(CLOCK_REALTIME, &tnow);
      current_time_ms = (uint64_t)((tnow.tv_sec * 1000)
                                    + (tnow.tv_nsec / 1000000));
    }

  /* We should not get here */

  ret = ioctl(fd, WDIOC_STOP, 0);
  if (ret < 0)
    {
      /* NOTE:  This may not be an error.  Some watchdog hardware does not
       * support stopping the watchdog once it has been started.
       */

      printf("wdog_main: ioctl(WDIOC_STOP) failed: %d\n", errno);
      goto errout_with_dev;
    }

  close(fd);
  fflush(stdout);
  return OK;

errout_with_dev:
  close(fd);
errout:
  fflush(stdout);
  return ERROR;
}
