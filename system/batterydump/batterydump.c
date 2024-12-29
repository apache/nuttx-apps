/****************************************************************************
 * apps/system/batterydump/batterydump.c
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

#include <inttypes.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <nuttx/power/battery_gauge.h>
#include <nuttx/power/battery_charger.h>
#include <nuttx/power/battery_monitor.h>
#include <nuttx/power/battery_ioctl.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum
{
    CHIP_TYPE_CHARGER = 0,
    CHIP_TYPE_GAUGE,
    CHIP_TYPE_MONITOR,
    CHIP_TYPE_MAX,
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool g_should_exit = false;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void usage(void)
{
  printf("batterydump [arguments...] <command>\n");
  printf("\t[-h      ]  batterydump commands help\n");
  printf("\t[-t   val]  0:charger 1:gauge 2:monitor\n");
  printf("\t[-f      ]  poll the battery state and dump\n");

  printf(" command:\n");
  printf("\t<batttery_node_name> ex:"
         "/dev/charge/goldfish_battery\n");
}

static void exit_handler(int signo)
{
  g_should_exit = true;
}

static void dump_charger(int fd, uint32_t mask)
{
  int status = 0;
  int health = 0;
  bool online = 0;
  int vol = 0;
  int protocol = 0;
  int ret = 0;

  ret = ioctl(fd, BATIOC_STATE, (unsigned long)&status);
  if (ret < 0)
    {
        printf("BATIOC_STATE failed %d", ret);
    }

  ret = ioctl(fd, BATIOC_ONLINE, (unsigned long)&online);
  if (ret < 0)
    {
      printf("BATIOC_ONLINE failed %d", ret);
    }

  ret = ioctl(fd, BATIOC_HEALTH, (unsigned long)&health);
  if (ret < 0)
    {
      printf("BATIOC_HEALTH failed %d", ret);
    }

  ret = ioctl(fd, BATIOC_GET_VOLTAGE, (unsigned long)&vol);
  if (ret < 0)
    {
      printf("BATIOC_GET_VOLTAGE failed %d", ret);
    }

  ret = ioctl(fd, BATIOC_GET_PROTOCOL, (unsigned long)&protocol);
  if (ret < 0)
    {
      printf("BATIOC_GET_PROTOCOL failed %d", ret);
    }

  printf("mask:%"PRIx32", state:%d, online:%d, health:%d,"
         " vol:%d, protocol:%d\n",
         mask, status, online, health, vol, protocol);
  return;
}

static void dump_gauge(int fd, uint32_t mask)
{
  int ret = 0;
  int state = 0;
  bool online = false;
  b16_t cap = 0;
  b16_t vol = 0;
  b16_t current = 0;
  b8_t temp = 0;

  ret = ioctl(fd, BATIOC_STATE, (unsigned long)&state);
  if (ret < 0)
    {
      printf("BATIOC_STATE failed %d", ret);
    }

  ret = ioctl(fd, BATIOC_ONLINE, (unsigned long)&online);
  if (ret < 0)
    {
      printf("BATIOC_ONLINE failed %d", ret);
    }

  ret = ioctl(fd, BATIOC_VOLTAGE, (unsigned long)&vol);
  if (ret < 0)
    {
      printf("BATIOC_VOLTAGE failed %d", ret);
    }

  ret = ioctl(fd, BATIOC_CAPACITY, (unsigned long)&cap);
  if (ret < 0)
    {
      printf("BATIOC_CAPACITY failed %d", ret);
    }

  ret = ioctl(fd, BATIOC_CURRENT, (unsigned long)&current);
  if (ret < 0)
    {
       printf("BATIOC_CURRENT failed %d", ret);
    }

  ret = ioctl(fd, BATIOC_TEMPERATURE, (unsigned long)&temp);
  if (ret < 0)
    {
      printf("BATIOC_TEMPERATURE failed %d", ret);
    }

  printf("mask:%"PRIx32", state:%d, online:%d, vol:%f, capacity:%"
         PRIi32"%%, current:%f, temperature:%f\n",
         mask, state, online, b16tof(vol), b16toi(cap),
         b16tof(current), b8tof(temp));
  return;
}

static void dump_monitor(int fd, uint32_t mask)
{
  int status = 0;
  int health = 0;
  bool online = 0;
  int vol = 0;
  int coulomb = 0;
  int ret = 0;
  struct battery_monitor_current_s current;

  ret = ioctl(fd, BATIOC_STATE, (unsigned long)&status);
  if (ret < 0)
    {
      printf("BATIOC_STATE failed %d", ret);
    }

  ret = ioctl(fd, BATIOC_ONLINE, (unsigned long)&online);
  if (ret < 0)
    {
      printf("BATIOC_ONLINE failed %d", ret);
    }

  ret = ioctl(fd, BATIOC_HEALTH, (unsigned long)&health);
  if (ret < 0)
    {
      printf("BATIOC_HEALTH failed %d", ret);
    }

  ret = ioctl(fd, BATIOC_VOLTAGE, (unsigned long)&vol);
  if (ret < 0)
    {
      printf("BATIOC_VOLTAGE failed %d", ret);
    }

  ret = ioctl(fd, BATIOC_COULOMBS, (unsigned long)&coulomb);
  if (ret < 0)
    {
       printf("BATIOC_COULOMBS failed %d", ret);
    }

  memset(&current, 0, sizeof(struct battery_monitor_current_s));
  ret = ioctl(fd, BATIOC_CURRENT, (unsigned long)&current);
  if (ret < 0)
    {
       printf("BATIOC_CURRENT failed %d", ret);
    }

  printf("mask:%"PRIx32", state:%d, online:%d, vol:%d, coulombs:%d, "
         "current:%"PRIi32", measurement time:%"PRIu32"\n",
         mask, status, online, vol, coulomb, current.current,
         current.time);
  return;
}

static void dump_battery(int fd, int chiptype, uint32_t mask)
{
  switch (chiptype)
  {
  case CHIP_TYPE_CHARGER:
    dump_charger(fd, mask);
    break;
  case CHIP_TYPE_GAUGE:
    dump_gauge(fd, mask);
    break;
  case CHIP_TYPE_MONITOR:
    dump_monitor(fd, mask);
    break;
  default:
    break;
  }

  return;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char *argv[])
{
  struct pollfd fds;
  int pollen = 0;
  int fd = -1;
  int chiptype = CHIP_TYPE_MAX;
  uint32_t mask = 0;
  int ret = 0;

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
  while ((ret = getopt(argc, argv, "t:fh")) != EOF)
    {
      switch (ret)
        {
          case 'f':
            pollen = 1;
            break;
          case 't':
            chiptype = strtoul(optarg, NULL, 10);
            break;
          case 'h':
          default:
            usage();
            return -EINVAL;
        }
    }

  if (optind >= argc || chiptype >= CHIP_TYPE_MAX)
    {
      usage();
      return -EINVAL;
    }

  fd = open(argv[optind], O_RDWR | O_NONBLOCK);
  if (fd < 0)
    {
      printf("Failed to open device:%s, ret:%s\n",
             argv[optind], strerror(errno));
      return -errno;
    }

  dump_battery(fd, chiptype, 0);
  fds.fd = fd;
  fds.events = POLLIN;

  while (pollen && !g_should_exit)
    {
      if (poll(&fds, 1, -1) > 0)
        {
          read(fds.fd, &mask, sizeof(uint32_t));
          dump_battery(fds.fd, chiptype, mask);
        }
    }

  return 0;
}
