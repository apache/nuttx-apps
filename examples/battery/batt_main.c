/****************************************************************************
 * apps/examples/batt/batt_main.c
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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/power/battery_ioctl.h>
#include <nuttx/power/battery_charger.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Configuration ************************************************************/

#ifndef CONFIG_EXAMPLES_BATTERY_DEVNAME
#  define CONFIG_EXAMPLES_BATTERT_DEVNAME "/dev/batt0"
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * status_report
 ****************************************************************************/

void status_report(int status)
{
  switch (status)
    {
      case BATTERY_UNKNOWN:
        {
          printf("Battery state is not known!\n");
        }
        break;

      case BATTERY_FAULT:
        {
          printf("Charger fault, look at the health info!\n");
        }
        break;

      case BATTERY_IDLE:
        {
          printf("Battery is idle, not full, not charging, not discharging!\n");
        }
        break;

      case BATTERY_FULL:
        {
          printf("Battery fully charged, not discharging!\n");
        }
        break;

      case BATTERY_CHARGING:
        {
          printf("Battery is charging, not full yet!\n");
        }
        break;

      case BATTERY_DISCHARGING:
        {
          printf("Battery discharging!\n");
        }
        break;

      default:
        printf("ERROR: the value %d is not a defined status!\n", status);
        break;
    }
}

/****************************************************************************
 * health_report
 ****************************************************************************/

void health_report(int health)
{
  switch (health)
    {
      case BATTERY_HEALTH_UNKNOWN:
        {
          printf("Battery health is not known!\n");
        }
        break;

      case BATTERY_HEALTH_GOOD:
        {
          printf("Battery is in good condiction!\n");
        }
        break;

      case BATTERY_HEALTH_DEAD:
        {
          printf("Battery is dead, nothing we can do!\n");
        }
        break;

      case BATTERY_HEALTH_OVERHEAT:
        {
          printf("Battery is over recommended temperature!\n");
        }
        break;

      case BATTERY_HEALTH_OVERVOLTAGE:
        {
          printf("Battery voltage is over recommended level!\n");
        }
        break;

      case BATTERY_HEALTH_UNSPEC_FAIL:
        {
          printf("Battery charger reported an unspected failure!\n");
        }
        break;

      case BATTERY_HEALTH_COLD:
        {
          printf("Battery is under recommended temperature!\n");
        }
        break;

      case BATTERY_HEALTH_WD_TMR_EXP:
        {
          printf("Battery WatchDog Timer Expired!\n");
        }
        break;

      case BATTERY_HEALTH_SAFE_TMR_EXP:
        {
          printf("Battery Safety Timer Expired!\n");
        }
        break;

      case BATTERY_HEALTH_DISCONNECTED:
        {
          printf("Battery is not connected!\n");
        }
        break;

      default:
        printf("ERROR: the value %d is not a defined health!\n", health);
        break;
    }
}

/****************************************************************************
 * batt_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int i;
  int fd;
  int ret;
  int status;
  int health;

  /* Open the battery charger device */

  fd = open(CONFIG_EXAMPLES_BATTERY_DEVNAME, O_RDONLY);
  if (fd < 0)
    {
      fprintf(stderr, "ERROR: Failed to open %s: %d\n",
              CONFIG_EXAMPLES_BATTERY_DEVNAME, errno);
      return EXIT_FAILURE;
    }

  printf("Going to read battery info, updated each two seconds.\n");
  printf("Try to remove the board power supply, etc, to change its status.\n");

  /* Wait the user read the information message above */

  sleep(5);

  for (i = 0; i < 10; i++)
    {
      printf("\n-----------------------------------------------------------\n");

      /* Read battery status */

      ret = ioctl(fd, BATIOC_STATE, (unsigned long)((uintptr_t) &status));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: ioctl(BATIOC_STATE) failed: %d\n", errno);
          goto errout_with_fd;
        }

      /* Show status */

      printf("STATUS: ");

      status_report(status);

      /* Read battery health */

      ret = ioctl(fd, BATIOC_HEALTH, (unsigned long)((uintptr_t) &health));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: ioctl(BATIOC_HEALTH) failed: %d\n", errno);
          goto errout_with_fd;
        }

      /* Show health */

      printf("HEALTH: ");

      health_report(health);

      /* Wait one second before reading again */

      sleep(2);
    }

  ret = OK;

errout_with_fd:
  close(fd);
  return ret;
}
