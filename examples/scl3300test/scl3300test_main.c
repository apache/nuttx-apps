/****************************************************************************
 * apps/examples/scl3300test/scl3300test_main.c
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

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <nuttx/sensors/ioctl.h>
#include <nuttx/sensors/scl3300.h>
#include <nuttx/uorb.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_SENSORS_USE_B16
#  define TODOUBLE(v)     ((double)(v) / 65536.0)
#else
#  define TODOUBLE(v)     ((double)(v))
#endif

#define SCL3300TEST_WHOAMI     0xc1
#define SCL3300TEST_NSAMPLES   200      /* Samples per noise measurement */
#define SCL3300TEST_NSKIP      20       /* Samples dropped after a change */
#define SCL3300TEST_NCYCLES    100      /* Power-down/wake-up cycles */
#define SCL3300TEST_INTERVAL   10000    /* Sampling interval in us */
#define SCL3300TEST_TIMEOUT    1000     /* Read timeout in ms */

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_devno;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: open_topic
 ****************************************************************************/

static int open_topic(FAR const char *name)
{
  char path[64];
  int fd;

  snprintf(path, sizeof(path), "/dev/uorb/sensor_%s%d", name, g_devno);
  fd = open(path, O_RDONLY | O_NONBLOCK);
  if (fd < 0)
    {
      printf("Failed to open %s: %d\n", path, errno);
    }

  return fd;
}

/****************************************************************************
 * Name: read_sample
 ****************************************************************************/

static int read_sample(int fd, FAR void *buf, size_t len)
{
  struct pollfd pfd;
  int ret;

  pfd.fd     = fd;
  pfd.events = POLLIN;

  ret = poll(&pfd, 1, SCL3300TEST_TIMEOUT);
  if (ret <= 0)
    {
      return -ETIMEDOUT;
    }

  ret = read(fd, buf, len);
  return ret == (int)len ? OK : -EIO;
}

/****************************************************************************
 * Name: test_whoami
 ****************************************************************************/

static int test_whoami(void)
{
  uint8_t id = 0;
  int ret;
  int fd;

  fd = open_topic("inclinometer");
  if (fd < 0)
    {
      return fd;
    }

  ret = ioctl(fd, SNIOC_WHO_AM_I, (unsigned long)((uintptr_t)&id));
  close(fd);

  ret = (ret == 0 && id == SCL3300TEST_WHOAMI) ? OK : -EIO;
  printf("WHOAMI: 0x%02x -> %s\n", id, ret == OK ? "PASS" : "FAIL");
  return ret;
}

/****************************************************************************
 * Name: test_selftest
 ****************************************************************************/

static int test_selftest(void)
{
  int ret;
  int fd;

  fd = open_topic("inclinometer");
  if (fd < 0)
    {
      return fd;
    }

  ret = ioctl(fd, SNIOC_SELFTEST, 0);
  close(fd);

  printf("SELFTEST: %d -> %s\n", ret, ret == 0 ? "PASS" : "FAIL");
  return ret == 0 ? OK : -EIO;
}

/****************************************************************************
 * Name: get_info
 ****************************************************************************/

static int get_info(FAR const char *name,
                    FAR struct sensor_device_info_s *info)
{
  int ret;
  int fd;

  fd = open_topic(name);
  if (fd < 0)
    {
      return fd;
    }

  ret = ioctl(fd, SNIOC_GET_INFO, (unsigned long)((uintptr_t)info));
  close(fd);
  return ret < 0 ? -errno : OK;
}

/****************************************************************************
 * Name: print_info
 ****************************************************************************/

static int print_info(FAR const char *name)
{
  struct sensor_device_info_s info;
  int ret;

  ret = get_info(name, &info);
  if (ret < 0)
    {
      printf("INFO %s: failed %d\n", name, ret);
      return ret;
    }

  printf("INFO %s: %s %s v%" PRIu32 ", %.2f mA, range %.4f, "
         "resolution %.7f, interval %" PRId32 "..%" PRId32 " us\n",
         name, info.vendor, info.name, info.version,
         TODOUBLE(info.power), TODOUBLE(info.max_range),
         TODOUBLE(info.resolution), info.min_delay, info.max_delay);
  return OK;
}

/****************************************************************************
 * Name: test_info
 ****************************************************************************/

static int test_info(void)
{
  int ret;

  ret = print_info("inclinometer");
#ifdef CONFIG_SENSORS_SCL3300_ACCEL
  if (ret == OK)
    {
      ret = print_info("accel");
    }
#endif

  return ret;
}

/****************************************************************************
 * Name: measure
 *
 * Description:
 *   Read SCL3300TEST_NSAMPLES inclinometer samples and print the mean and
 *   the standard deviation of each axis.
 *
 ****************************************************************************/

static int measure(int fd)
{
  struct sensor_inclinometer data;
  double sum[3];
  double sq[3];
  double val[3];
  double mean;
  double temp = 0.0;
  int got = 0;
  int i;
  int k;

  memset(sum, 0, sizeof(sum));
  memset(sq, 0, sizeof(sq));

  ioctl(fd, SNIOC_SET_INTERVAL, SCL3300TEST_INTERVAL);

  /* Drop the samples taken while the output settles */

  for (i = 0; i < SCL3300TEST_NSKIP; i++)
    {
      read_sample(fd, &data, sizeof(data));
    }

  for (i = 0; i < SCL3300TEST_NSAMPLES; i++)
    {
      if (read_sample(fd, &data, sizeof(data)) < 0)
        {
          continue;
        }

      val[0] = TODOUBLE(data.x);
      val[1] = TODOUBLE(data.y);
      val[2] = TODOUBLE(data.z);
      for (k = 0; k < 3; k++)
        {
          sum[k] += val[k];
          sq[k]  += val[k] * val[k];
        }

      temp = TODOUBLE(data.temperature);
      got++;
    }

  if (got == 0)
    {
      printf("  No samples received\n");
      return -ETIMEDOUT;
    }

  printf("  %d/%d samples, temperature %.2f C\n", got,
         SCL3300TEST_NSAMPLES, temp);
  for (k = 0; k < 3; k++)
    {
      mean = sum[k] / got;
      printf("  %c: mean %9.4f deg, stddev %.5f deg\n", 'X' + k, mean,
             sqrt(fabs(sq[k] / got - mean * mean)));
    }

  return got == SCL3300TEST_NSAMPLES ? OK : -EIO;
}

/****************************************************************************
 * Name: test_read
 ****************************************************************************/

static int test_read(void)
{
  int ret;
  int fd;

  fd = open_topic("inclinometer");
  if (fd < 0)
    {
      return fd;
    }

  printf("READ:\n");
  ret = measure(fd);
  close(fd);
  return ret;
}

/****************************************************************************
 * Name: test_modes
 *
 * Description:
 *   Go through Modes 1, 2, 3, 4 and back to 1, printing the device info
 *   and the noise in each.  Then check that an invalid mode is rejected.
 *
 ****************************************************************************/

static int test_modes(void)
{
  static const int modes[] =
  {
    SCL3300_MODE_1, SCL3300_MODE_2, SCL3300_MODE_3, SCL3300_MODE_4,
    SCL3300_MODE_1
  };

  int result = OK;
  int ret;
  int fd;
  int i;

  fd = open_topic("inclinometer");
  if (fd < 0)
    {
      return fd;
    }

  for (i = 0; i < sizeof(modes) / sizeof(modes[0]); i++)
    {
      printf("MODE %d:\n", modes[i]);
      ret = ioctl(fd, SNIOC_SET_OPERATIONAL_MODE, modes[i]);
      if (ret < 0)
        {
          printf("  SNIOC_SET_OPERATIONAL_MODE failed: %d -> FAIL\n",
                 errno);
          result = -EIO;
          continue;
        }

      test_info();
      if (measure(fd) < 0)
        {
          result = -EIO;
        }
    }

  ret = ioctl(fd, SNIOC_SET_OPERATIONAL_MODE, SCL3300_MODE_4 + 1);
  if (ret < 0 && errno == EINVAL)
    {
      printf("MODE %d: rejected -> PASS\n", SCL3300_MODE_4 + 1);
    }
  else
    {
      printf("MODE %d: accepted -> FAIL\n", SCL3300_MODE_4 + 1);
      result = -EIO;
    }

  close(fd);
  return result;
}

/****************************************************************************
 * Name: test_power
 *
 * Description:
 *   Select Mode 4 and the power-down-when-idle policy, then activate and
 *   deactivate the inclinometer SCL3300TEST_NCYCLES times, reading one
 *   sample each time.  The mode must survive every power-down.  The
 *   defaults (Mode 1, always on) are restored at the end.
 *
 ****************************************************************************/

static int test_power(void)
{
#ifdef CONFIG_SENSORS_SCL3300_ACCEL
  struct sensor_device_info_s before;
  struct sensor_device_info_s after;
#endif
  struct sensor_inclinometer data;
  int fails = 0;
  int ret;
  int fd;
  int i;

  /* Opening the topic activates it, so the device powers down when this
   * descriptor is closed.
   */

  fd = open_topic("inclinometer");
  if (fd < 0)
    {
      return fd;
    }

  ret = ioctl(fd, SNIOC_SET_OPERATIONAL_MODE, SCL3300_MODE_4);
  if (ret >= 0)
    {
      ret = ioctl(fd, SNIOC_SET_POWER_MODE, SCL3300_POWER_DOWN_IDLE);
    }

  close(fd);
  if (ret < 0)
    {
      printf("POWER: ioctl failed: %d -> FAIL\n", errno);
      return -EIO;
    }

#ifdef CONFIG_SENSORS_SCL3300_ACCEL
  get_info("accel", &before);
#endif

  printf("POWER: %d power-down/wake-up cycles\n", SCL3300TEST_NCYCLES);
  for (i = 0; i < SCL3300TEST_NCYCLES; i++)
    {
      fd = open_topic("inclinometer");
      if (fd < 0)
        {
          fails++;
          continue;
        }

      ioctl(fd, SNIOC_SET_INTERVAL, SCL3300TEST_INTERVAL);
      if (read_sample(fd, &data, sizeof(data)) < 0)
        {
          fails++;
        }

      close(fd);
    }

#ifdef CONFIG_SENSORS_SCL3300_ACCEL
  /* The accelerometer resolution depends on the mode */

  get_info("accel", &after);
  if (after.resolution != before.resolution)
    {
      printf("  Mode not preserved\n");
      fails++;
    }
#endif

  fd = open_topic("inclinometer");
  if (fd >= 0)
    {
      ioctl(fd, SNIOC_SET_POWER_MODE, SCL3300_POWER_ALWAYS_ON);
      ioctl(fd, SNIOC_SET_OPERATIONAL_MODE, SCL3300_MODE_1);
      close(fd);
    }

  printf("POWER: %d failures -> %s\n", fails, fails == 0 ? "PASS" : "FAIL");
  return fails == 0 ? OK : -EIO;
}

/****************************************************************************
 * Name: test_reset
 ****************************************************************************/

static int test_reset(void)
{
  int ret;
  int fd;

  fd = open_topic("inclinometer");
  if (fd < 0)
    {
      return fd;
    }

  ret = ioctl(fd, SNIOC_RESET, 0);
  close(fd);

  printf("RESET: %d -> %s\n", ret, ret == 0 ? "PASS" : "FAIL");
  if (ret < 0)
    {
      return -EIO;
    }

  return test_read();
}

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname)
{
  printf("Usage: %s [-n devno] <command>\n", progname);
  printf("  -n devno  Topic instance (default 0)\n");
  printf("Commands:\n");
  printf("  whoami    Read WHOAMI\n");
  printf("  selftest  Run the self-test\n");
  printf("  info      Print the device info of each topic\n");
  printf("  read      Print mean and noise of %d samples\n",
         SCL3300TEST_NSAMPLES);
  printf("  modes     Measure in Modes 1, 2, 3, 4 and 1\n");
  printf("  power     Cycle the power-down-when-idle policy\n");
  printf("  reset     Reset the device and read again\n");
  printf("  all       All of the above\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR const char *cmd;
  int ret = OK;
  int opt;

  g_devno = 0;
  optind  = 1;

  while ((opt = getopt(argc, argv, "n:h")) != ERROR)
    {
      switch (opt)
        {
          case 'n':
            g_devno = atoi(optarg);
            break;

          default:
            show_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

  if (optind >= argc)
    {
      show_usage(argv[0]);
      return EXIT_FAILURE;
    }

  cmd = argv[optind];
  if (strcmp(cmd, "whoami") == 0)
    {
      ret = test_whoami();
    }
  else if (strcmp(cmd, "selftest") == 0)
    {
      ret = test_selftest();
    }
  else if (strcmp(cmd, "info") == 0)
    {
      ret = test_info();
    }
  else if (strcmp(cmd, "read") == 0)
    {
      ret = test_read();
    }
  else if (strcmp(cmd, "modes") == 0)
    {
      ret = test_modes();
    }
  else if (strcmp(cmd, "power") == 0)
    {
      ret = test_power();
    }
  else if (strcmp(cmd, "reset") == 0)
    {
      ret = test_reset();
    }
  else if (strcmp(cmd, "all") == 0)
    {
      ret |= test_whoami();
      ret |= test_selftest();
      ret |= test_info();
      ret |= test_read();
      ret |= test_modes();
      ret |= test_power();
      ret |= test_reset();
      printf("ALL: %s\n", ret == OK ? "PASS" : "FAIL");
    }
  else
    {
      show_usage(argv[0]);
      return EXIT_FAILURE;
    }

  return ret == OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
