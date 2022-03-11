/****************************************************************************
 * apps/examples/charger/charger_main.c
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
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>

#include <nuttx/power/battery_charger.h>
#include <nuttx/power/battery_ioctl.h>

#include <arch/chip/battery_ioctl.h>
#include <arch/board/board.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_CHARGER_DEVNAME
#  define DEVPATH CONFIG_EXAMPLES_CHARGER_DEVNAME
#else
#  define DEVPATH "/dev/bat"
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int show_charge_setting(int fd)
{
  int curr;
  int vol;
  int revol;
  int compcurr;
  struct battery_temp_table_s tab;
  int ret;

  ret = ioctl(fd, BATIOC_GET_CHGVOLTAGE, (unsigned long)(uintptr_t)&vol);
  if (ret < 0)
    {
      printf("ioctl GET_CHGVOLTAGE failed. %d\n", errno);
      return -1;
    }

  ret = ioctl(fd, BATIOC_GET_CHGCURRENT, (unsigned long)(uintptr_t)&curr);
  if (ret < 0)
    {
      printf("ioctl GET_CHGCURRENT failed. %d\n", errno);
      return -1;
    }

  ret = ioctl(fd, BATIOC_GET_RECHARGEVOL, (unsigned long)(uintptr_t)&revol);
  if (ret < 0)
    {
      printf("ioctl GET_RECHARGEVOL failed. %d\n", errno);
      return -1;
    }

  ret = ioctl(fd, BATIOC_GET_COMPCURRENT,
              (unsigned long)(uintptr_t)&compcurr);
  if (ret < 0)
    {
      printf("ioctl GET_COMPCURRENT failed. %d\n", errno);
      return -1;
    }

  ret = ioctl(fd, BATIOC_GET_TEMPTABLE, (unsigned long)(uintptr_t)&tab);
  if (ret < 0)
    {
      printf("ioctl BATIOC_GET_TEMPTABLE failed. %d\n", errno);
      return -1;
    }

  printf("Charge voltage:         %d\n", vol);
  printf("Charge current limit:   %d\n", curr);
  printf("Recharge voltage:       %d (%d)\n", vol + revol, revol);
  printf("Done current threshold: %d\n", compcurr);
  printf("Temperature table:\n"
         "  60: 0x%x\n"
         "  45: 0x%x\n"
         "  10: 0x%x\n"
         "   0: 0x%x\n",
         tab.T60, tab.T45, tab.T10, tab.T00);

  return 0;
}

static int show_bat_status(int fd)
{
  enum battery_status_e status;
  enum battery_health_e health;
  const char *statestr[] =
    {
      "UNKNOWN",
      "FAULT",
      "IDLE",
      "FULL",
      "CHARGING",
      "DISCHARGING"
    };

  const char *healthstr[] =
    {
      "UNKNOWN",
      "GOOD",
      "DEAD",
      "OVERHEAT",
      "OVERVOLTAGE",
      "UNSPEC_FAIL",
      "COLD",
      "WD_TMR_EXP",
      "SAFE_TMR_EXP",
      "DISCONNECTED"
    };

  int ret;

  ret = ioctl(fd, BATIOC_STATE, (unsigned long)(uintptr_t)&status);
  if (ret < 0)
    {
      printf("ioctl BATIOC_STATE failed. %d\n", errno);
      return -1;
    }

  ret = ioctl(fd, BATIOC_HEALTH, (unsigned long)(uintptr_t)&health);
  if (ret < 0)
    {
      printf("ioctl BATIOC_HEALTH failed. %d\n", errno);
      return -1;
    }

  printf("State: %s, Health: %s\n",
         statestr[status], healthstr[health]);

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * charger_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int fd;
  int current;
  int voltage;
  int verbose = 0;
  int opt;
  struct timeval tv;
  int ret;

  while ((opt = getopt(argc, argv, "v")) != -1)
    {
      switch (opt)
        {
          case 'v':
            verbose = 1;
            break;

          default:
            printf("Usage: %s [-v]\n", argv[0]);
            return 1;
        }
    }

  argc--;
  argv++;

  /* Initialize and create battery charger device */

  board_charger_initialize(DEVPATH);

  fd = open(DEVPATH, O_RDWR);
  if (fd < 0)
    {
      printf("Device open error.\n");
      return 0;
    }

  if (verbose)
    {
      show_charge_setting(fd);
      show_bat_status(fd);
    }

  ret = ioctl(fd, BATIOC_GET_CURRENT, (unsigned long)(uintptr_t)&current);
  if (ret < 0)
    {
      printf("ioctl GET_CURRENT failed. %d\n", errno);
      return 1;
    }

  ret = ioctl(fd, BATIOC_GET_VOLTAGE, (unsigned long)(uintptr_t)&voltage);
  if (ret < 0)
    {
      printf("ioctl GET_VOLTAGE failed. %d\n", errno);
      return 1;
    }

  gettimeofday(&tv, NULL);
  printf("%ju.%06ld: %d mV, %d mA\n",
         (uintmax_t)tv.tv_sec, tv.tv_usec, voltage, current);

  close(fd);

  /* Uninitialize and remove device file */

  board_charger_uninitialize(DEVPATH);

  return 0;
}
