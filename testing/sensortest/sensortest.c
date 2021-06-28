/****************************************************************************
 * apps/testing/sensortest/sensortest.c
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

#include <nuttx/sensors/sensor.h>
#include <nuttx/config.h>
#include <sys/ioctl.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <poll.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ARRAYSIZE(a)       (sizeof(a) / sizeof(a)[0])
#define DEVNAME_FMT        "/dev/sensor/%s"
#define DEVNAME_MAX        64

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef void (*data_print)(FAR const char *buffer, FAR const char *name);
struct sensor_info
{
  data_print     print;
  const uint8_t  esize;
  FAR const char *name;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void print_vec3(FAR const char *buffer, FAR const char *name);
static void print_valf3(FAR const char *buffer, FAR const char *name);
static void print_valf2(FAR const char *buffer, FAR const char *name);
static void print_valf(FAR const char *buffer, FAR const char *name);
static void print_valb(FAR const char *buffer, FAR const char *name);
static void print_gps(FAR const char *buffer, FAR const char *name);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool g_should_exit = false;

static const struct sensor_info g_sensor_info[] =
{
  {print_vec3,  sizeof(struct sensor_event_accel), "accel"},
  {print_vec3,  sizeof(struct sensor_event_mag),   "mag"},
  {print_vec3,  sizeof(struct sensor_event_gyro),  "gyro"},
  {print_valf2, sizeof(struct sensor_event_baro),  "baro"},
  {print_valf,  sizeof(struct sensor_event_light), "light"},
  {print_valf,  sizeof(struct sensor_event_prox),  "prox"},
  {print_valf,  sizeof(struct sensor_event_humi),  "humi"},
  {print_valf,  sizeof(struct sensor_event_temp),  "temp"},
  {print_valf3, sizeof(struct sensor_event_rgb),   "rgb"},
  {print_valb,  sizeof(struct sensor_event_hall),  "hall"},
  {print_valf,  sizeof(struct sensor_event_ir),    "ir"},
  {print_gps,   sizeof(struct sensor_event_gps),   "gps"},
  {print_valf,  sizeof(struct sensor_event_uv),    "uv"},
  {print_valf,  sizeof(struct sensor_event_noise), "noise"},
  {print_valf,  sizeof(struct sensor_event_pm25),  "pm25"},
  {print_valf,  sizeof(struct sensor_event_pm1p0), "pm1p0"},
  {print_valf,  sizeof(struct sensor_event_pm10),  "pm10"},
  {print_valf,  sizeof(struct sensor_event_co2),   "co2"},
  {print_valf,  sizeof(struct sensor_event_hcho),  "hcho"},
  {print_valf,  sizeof(struct sensor_event_tvoc),  "tvoc"},
  {print_valf,  sizeof(struct sensor_event_ph),    "ph"},
  {print_valf,  sizeof(struct sensor_event_dust),  "dust"},
  {print_valf,  sizeof(struct sensor_event_hrate), "hrate"},
  {print_valf,  sizeof(struct sensor_event_hbeat), "hbeat"},
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void print_vec3(const char *buffer, const char *name)
{
  struct sensor_event_accel *event = (struct sensor_event_accel *)buffer;
  printf("%s: timestamp:%" PRIu64 " x:%.2f y:%.2f z:%.2f, "
         "temperature:%.2f\n",
         name, event->timestamp, event->x, event->y,
         event->z, event->temperature);
}

static void print_valb(const char *buffer, const char *name)
{
  struct sensor_event_hall *event = (struct sensor_event_hall *)buffer;
  printf("%s: timestamp:%" PRIu64 " value:%d\n",
         name, event->timestamp, event->hall);
}

static void print_valf(const char *buffer, const char *name)
{
  struct sensor_event_prox *event = (struct sensor_event_prox *)buffer;
  printf("%s: timestamp:%" PRIu64 " value:%.2f\n",
         name, event->timestamp, event->proximity);
}

static void print_valf2(const char *buffer, const char *name)
{
  struct sensor_event_baro *event = (struct sensor_event_baro *)buffer;
  printf("%s: timestamp:%" PRIu64 " value1:%.2f value2:%.2f\n",
         name, event->timestamp, event->pressure, event->temperature);
}

static void print_valf3(const char *buffer, const char *name)
{
  struct sensor_event_rgb *event = (struct sensor_event_rgb *)buffer;
  printf("%s: timestamp:%" PRIu64 " value1:%.2f value2:%.2f, value3:%.2f\n",
         name, event->timestamp, event->r, event->g, event->b);
}

static void print_gps(const char *buffer, const char *name)
{
  struct sensor_event_gps *event = (struct sensor_event_gps *)buffer;

  printf("%s: year: %d month: %d day: %d hour: %d min: %d sec: %d "
         "msec: %d\n", name, event->year, event->month, event->day,
         event->hour, event->min, event->sec, event->msec);
  printf("%s: yaw: %.4f height: %.4f speed: %.4f latitude: %.4f "
         "longitude: %.4f\n", name, event->yaw, event->height, event->speed,
         event->latitude, event->longitude);
}

static void usage(void)
{
  printf("sensortest [arguments...] <command>\n");
  printf("\t[-h      ]  sensotest commands help\n");
  printf("\t[-i <val>]  The output data period of sensor in us\n");
  printf("\t            default: 1000000\n");
  printf("\t[-b <val>]  The maximum report latency of sensor in us\n");
  printf("\t            default: 0\n");
  printf("\t[-n <val>]  The specify number of output data\n");
  printf("\t            default: 0\n");

  printf(" Commands:\n");
  printf("\t<sensor_node_name> ex, accel0(/dev/sensor/accel0)\n");
}

static void exit_handler(int signo)
{
  g_should_exit = true;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * sensortest_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  unsigned int interval = 1000000;
  unsigned int received = 0;
  unsigned int latency = 0;
  unsigned int count = 0;
  char devname[PATH_MAX];
  struct pollfd fds;
  FAR char *buffer;
  FAR char *name;
  int len = 0;
  int fd;
  int idx;
  int ret;

  if (argc <= 1)
    {
      usage();
      return -EINVAL;
    }

  if (signal(SIGINT, exit_handler) == SIG_ERR)
    {
      return -errno;
    }

  g_should_exit = false;
  while ((ret = getopt(argc, argv, "i:b:n:h")) != EOF)
    {
      switch (ret)
        {
          case 'i':
            interval = strtoul(optarg, NULL, 0);
            break;

          case 'b':
            latency = strtoul(optarg, NULL, 0);
            break;

          case 'n':
            count = strtoul(optarg, NULL, 0);
            break;

          case 'h':
          default:
            usage();
            goto name_err;
        }
    }

  if (optind < argc)
    {
      name = argv[optind];
      for (idx = 0; idx < ARRAYSIZE(g_sensor_info); idx++)
        {
          if (!strncmp(name, g_sensor_info[idx].name,
              strlen(g_sensor_info[idx].name)))
            {
              len = g_sensor_info[idx].esize;
              buffer = calloc(1, len);
              break;
            }
        }

      if (!len)
        {
          printf("The sensor node name:%s is invaild\n", name);
          usage();
          ret = -EINVAL;
          goto name_err;
        }

      if (!buffer)
        {
          ret = -ENOMEM;
          goto name_err;
        }
    }
  else
    {
      usage();
      ret = -EINVAL;
      goto name_err;
    }

  snprintf(devname, PATH_MAX, DEVNAME_FMT, name);
  fd = open(devname, O_RDONLY | O_NONBLOCK);
  if (fd < 0)
    {
      ret = -errno;
      printf("Failed to open device:%s, ret:%s\n",
             devname, strerror(errno));
      goto open_err;
    }

  ret = ioctl(fd, SNIOC_SET_INTERVAL, &interval);
  if (ret < 0)
    {
      ret = -errno;
      if (ret != -ENOTTY)
        {
          printf("Failed to set interval for sensor:%s, ret:%s\n",
                 devname, strerror(errno));
          goto ctl_err;
        }
    }

  ret = ioctl(fd, SNIOC_BATCH, &latency);
  if (ret < 0)
    {
      ret = -errno;
      if (ret != -ENOTTY)
        {
          printf("Failed to batch for sensor:%s, ret:%s\n",
                 devname, strerror(errno));
          goto ctl_err;
        }
    }

  ret = ioctl(fd, SNIOC_ACTIVATE, 1);
  if (ret < 0)
    {
      ret = -errno;
      if (ret != -ENOTTY)
        {
          printf("Failed to enable sensor:%s, ret:%s\n",
                 devname, strerror(errno));
          goto ctl_err;
        }
    }

  printf("SensorTest: Test %s with interval(%uus), latency(%uus)\n",
         devname, interval, latency);

  fds.fd = fd;
  fds.events = POLLIN;

  while ((!count || received < count) && !g_should_exit)
    {
      if (poll(&fds, 1, -1) > 0)
        {
          if (read(fd, buffer, len) >= len)
            {
              received++;
              g_sensor_info[idx].print(buffer, name);
            }
        }
    }

  printf("SensorTest: Received message: %s, number:%d/%d\n",
         name, received, count);

  ret = ioctl(fd, SNIOC_ACTIVATE, 0);
  if (ret < 0)
    {
      ret = -errno;
      printf("Failed to disable sensor:%s, ret:%s\n",
             devname, strerror(errno));
      goto ctl_err;
    }

ctl_err:
  close(fd);
open_err:
  free(buffer);
name_err:
  optind = 0;
  return ret;
}
