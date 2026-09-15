/****************************************************************************
 * apps/testing/drivers/drivertest/drivertest_sensor.c
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
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>

#include <nuttx/sensors/sensor.h>
#include <nuttx/sensors/ioctl.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SENSOR_DEVPATH      "/dev/uorb/sensor_baro0"
#define SENSOR_INTERVAL     100000
#define SENSOR_COUNT        5
#define SENSOR_READ_BUFSIZE 128

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct sensortest_state_s
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
                       FAR struct sensortest_state_s *sensor_state,
                       int exitcode)
{
  printf("Usage: %s [-d <devpath>]\n", progname);
  printf("  [-d devpath] selects the sensor device.\n"
         "  Default: %s Current: %s\n",
         SENSOR_DEVPATH, sensor_state->devpath);
  printf("  [-h] = Shows this message and exits\n");

  exit(exitcode);
}

/****************************************************************************
 * Name: parse_commandline
 ****************************************************************************/

static void parse_commandline(FAR struct sensortest_state_s *sensor_state,
                              int argc, FAR char **argv)
{
  int ch;

  while ((ch = getopt(argc, argv, "d:h")) != ERROR)
    {
      switch (ch)
        {
          case 'd':
            strlcpy(sensor_state->devpath, optarg,
                    sizeof(sensor_state->devpath));
            break;

          case '?':
            printf("Unsupported option: %s\n", optarg);
            show_usage(argv[0], sensor_state, EXIT_FAILURE);
            break;

          default:
            show_usage(argv[0], sensor_state, EXIT_FAILURE);
            break;
        }
    }
}

/****************************************************************************
 * Name: drivertest_sensor_fetch_wdog
 *
 * Description:
 *   Regression test for apache/nuttx#20145, fixed by apache/nuttx#20146.
 *   sensor_poll() arms a per-subscriber watchdog for fetch-only sensors
 *   with a finite interval, but sensor_close() used to free the
 *   subscriber without cancelling it, letting sensor_fetch_expired()
 *   run after free. The test sets a finite interval, polls so the
 *   watchdog can arm, closes, waits past the interval so any stray
 *   timer would fire, and reopens to prove teardown was clean. It does
 *   not deterministically reproduce the UAF; under KASAN or stress a
 *   missing wd_cancel in close would be caught here.
 *
 ****************************************************************************/

static void drivertest_sensor_fetch_wdog(FAR void **state)
{
  FAR struct sensortest_state_s *sensor_state;
  char buffer[SENSOR_READ_BUFSIZE];
  struct pollfd fds;
  unsigned int i;
  int fd;
  int ret;

  sensor_state = (FAR struct sensortest_state_s *)*state;

  fd = open(sensor_state->devpath, O_RDONLY);
  if (fd < 0)
    {
      if (errno == ENOENT || errno == ENODEV || errno == ENXIO)
        {
          printf("Sensor device %s not present, skipping test\n",
                 sensor_state->devpath);
          return;
        }

      assert_true(fd >= 0);
    }

  ret = ioctl(fd, SNIOC_SET_INTERVAL, SENSOR_INTERVAL);
  if (ret < 0 && (errno == ENOTSUP || errno == ENOTTY))
    {
      printf("Sensor device %s does not support intervals, "
             "skipping test\n", sensor_state->devpath);
      close(fd);
      return;
    }

  assert_return_code(ret, OK);

  for (i = 0; i < SENSOR_COUNT; i++)
    {
      /* First poll reports POLLIN once per interval for a fetch-only
       * sensor and records the timestamp the pacing is based on.
       */

      fds.fd = fd;
      fds.events = POLLIN;
      fds.revents = 0;
      ret = poll(&fds, 1, 0);
      assert_true(ret >= 0);

      if (ret > 0 && (fds.revents & POLLIN))
        {
          ret = read(fd, buffer, sizeof(buffer));
          assert_true(ret >= 0);
        }

      /* Second poll arms user->wdog (elapsed < interval). Closing
       * here exercises the lifetime fixed by #20146.
       */

      fds.fd = fd;
      fds.events = POLLIN;
      fds.revents = 0;
      ret = poll(&fds, 1, 0);
      assert_true(ret >= 0);

      ret = close(fd);
      assert_return_code(ret, OK);

      /* Let any stray timer fire before reopening. */

      usleep(SENSOR_INTERVAL * 2);

      fd = open(sensor_state->devpath, O_RDONLY);
      assert_true(fd >= 0);

      ret = ioctl(fd, SNIOC_SET_INTERVAL, SENSOR_INTERVAL);
      assert_return_code(ret, OK);
    }

  ret = close(fd);
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
  struct sensortest_state_s sensor_state =
  {
    .devpath = SENSOR_DEVPATH,
  };

  parse_commandline(&sensor_state, argc, argv);

  const struct CMUnitTest tests[] =
  {
    cmocka_unit_test_prestate(drivertest_sensor_fetch_wdog, &sensor_state),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
