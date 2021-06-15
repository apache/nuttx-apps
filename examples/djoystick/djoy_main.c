/****************************************************************************
 * examplex/djoystick/djoy_main.c
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
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/input/djoystick.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

#ifndef CONFIG_INPUT_DJOYSTICK
#  error "CONFIG_INPUT_DJOYSTICK is not defined in the configuration"
#endif

#ifndef CONFIG_EXAMPLES_DJOYSTICK_DEVNAME
#  define CONFIG_EXAMPLES_DJOYSTICK_DEVNAME "/dev/djoy0"
#endif

#ifndef CONFIG_EXAMPLES_DJOYSTICK_SIGNO
#  define CONFIG_EXAMPLES_DJOYSTICK_SIGNO 13
#endif

/* Helpers ******************************************************************/

#ifndef MIN
#  define MIN(a,b) (a < b ? a : b)
#endif
#ifndef MAX
#  define MAX(a,b) (a > b ? a : b)
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void show_joystick(djoy_buttonset_t oldset, djoy_buttonset_t newset);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* The set of supported joystick discretes */

static djoy_buttonset_t g_djoysupported;

/* Last sampled discrete set */

static djoy_buttonset_t g_djoylast;

/* Joystick discrete names */

static const char *g_djoynames[DJOY_NDISCRETES] =
{
  "UP", "DOWN", "LEFT", "RIGHT", "SELECT", "FIRE", "JUMP", "RUN"
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void show_joystick(djoy_buttonset_t oldset, djoy_buttonset_t newset)
{
  djoy_buttonset_t chgset = oldset ^ newset;
  int i;

  /* Show each discrete state change */

  for (i = 0; i <= DJOY_NDISCRETES; i++)
    {
      djoy_buttonset_t mask = (1 << i);
      if ((chgset & mask) != 0)
        {
          FAR const char *state;

          /* Get the discrete state */

          if ((newset & mask) != 0)
            {
              state = "selected";
            }
          else
            {
              state = "released";
            }

          printf("  %-6s: %s\n", g_djoynames[i], state);
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * djoy_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct djoy_notify_s notify;
  int fd;
  int tmp;
  int ret;
  int errcode = EXIT_FAILURE;

  /* Reset some globals that might been been left in a bad state */

  g_djoylast = 0;

  /* Open the djoystick device */

  fd = open(CONFIG_EXAMPLES_DJOYSTICK_DEVNAME, O_RDONLY);
  if (fd < 0)
    {
      fprintf(stderr, "ERROR: Failed to open %s: %d\n",
              CONFIG_EXAMPLES_DJOYSTICK_DEVNAME, errno);
      return EXIT_FAILURE;
    }

  /* Get the set of supported discretes */

  ret = ioctl(fd, DJOYIOC_SUPPORTED,  (unsigned long)((uintptr_t)&tmp));
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ioctl(DJOYIOC_SUPPORTED) failed: %d\n", errno);
      goto errout_with_fd;
    }

  g_djoysupported = (djoy_buttonset_t)tmp;
  printf("Supported joystick discretes:\n");
  show_joystick(0, g_djoysupported);

  /* Register to receive a signal on any change in the joystick discretes. */

  notify.dn_press   = g_djoysupported;
  notify.dn_release = g_djoysupported;

  notify.dn_event.sigev_notify = SIGEV_SIGNAL;
  notify.dn_event.sigev_signo  = CONFIG_EXAMPLES_DJOYSTICK_SIGNO;

  ret = ioctl(fd, DJOYIOC_REGISTER, (unsigned long)((uintptr_t)&notify));
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ioctl(DJOYIOC_REGISTER) failed: %d\n", errno);
      goto errout_with_fd;
    }

  /* Ignore the default signal action */

  signal(CONFIG_EXAMPLES_DJOYSTICK_SIGNO, SIG_IGN);

  /* Then loop, receiving signals indicating joystick events. */

  for (; ; )
    {
      struct siginfo value;
      sigset_t set;
      djoy_buttonset_t newset;
      ssize_t nread;

      /* Wait for a signal */

      sigemptyset(&set);
      sigaddset(&set, CONFIG_EXAMPLES_DJOYSTICK_SIGNO);
      ret = sigwaitinfo(&set, &value);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: sigwaitinfo() failed: %d\n", errno);
          goto errout_with_fd;
        }

      /* Show the set of joystick discretes accompanying the signal */

      printf("Signalled set\n");
      newset = (djoy_buttonset_t)value.si_value.sival_int;
      show_joystick(g_djoylast, newset);
      g_djoylast = newset;

      /* Read the signal set and compare */

      nread = read(fd, &newset, sizeof(djoy_buttonset_t));
      if (nread < 0)
        {
          fprintf(stderr, "ERROR: read() failed: %d\n", errno);
          goto errout_with_fd;
        }
      else if (nread != sizeof(djoy_buttonset_t))
        {
          fprintf(stderr, "ERROR: read() unexpected size: %ld vs %d\n",
                  (long)nread, sizeof(djoy_buttonset_t));
          goto errout_with_fd;
        }

      /* Show the set of joystick discretes that we just read */

      printf("Read set\n");
      show_joystick(g_djoylast, newset);
      g_djoylast = newset;
    }

  errcode = EXIT_SUCCESS;

errout_with_fd:
  close(fd);
  return errcode;
}
