/****************************************************************************
 * examples/batt/batt_main.c
 *
 *   Copyright (C) 2019 Alan Carvalho de Assis. All rights reserved.
 *   Author: Alan Carvalho de Assis <acassis@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
