/****************************************************************************
 * examplex/ajoystick/ajoy_main.c
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
#include <fixedmath.h>
#include <debug.h>

#include <nuttx/input/ajoystick.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Configuration ************************************************************/

#ifndef CONFIG_AJOYSTICK
#  error "CONFIG_AJOYSTICK is not defined in the configuration"
#endif

#ifndef CONFIG_EXAMPLES_AJOYSTICK_DEVNAME
#  define CONFIG_EXAMPLES_AJOYSTICK_DEVNAME "/dev/ajoy0"
#endif

#ifndef CONFIG_EXAMPLES_AJOYSTICK_SIGNO
#  define CONFIG_EXAMPLES_AJOYSTICK_SIGNO 13
#endif

#define FULL_RANGE 16

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

static void show_buttons(ajoy_buttonset_t oldset, ajoy_buttonset_t newset);
static void show_joystick(FAR const struct ajoy_sample_s *sample);
static int ajoy_wait(int fd, FAR const struct timespec *timeout);
static int ajoy_read(int fd, FAR struct ajoy_sample_s *sample);
static int ajoy_calibrate(int fd);

/****************************************************************************
 * Private Data
 ****************************************************************************/
/* The set of supported joystick buttons */

static ajoy_buttonset_t g_ajoysupported;

/* Last sampled button set */

static ajoy_buttonset_t g_ajoylast;

/* Calibration data */

static bool    g_calibrated;
static bool    g_lispositive;
static bool    g_fispositive;
static int16_t g_xcenter;
static int16_t g_ycenter;
static b16_t   g_xlslope;
static b16_t   g_xrslope;
static b16_t   g_yfslope;
static b16_t   g_ybslope;

/* Joystick button names */

static const char *g_ajoynames[AJOY_NBUTTONS] =
{
  "SELECT",   "FIRE",     "JUMP",     "BUTTON 4",
  "BUTTON 5", "BUTTON 6", "BUTTON 7", "BUTTON 8",
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void show_buttons(ajoy_buttonset_t oldset, ajoy_buttonset_t newset)
{
  ajoy_buttonset_t chgset = oldset ^ newset;
  int i;

  if (chgset != 0)
    {
      /* Show each button state change */

      for (i = 0; i <= AJOY_NBUTTONS; i++)
        {
          ajoy_buttonset_t mask = (1 << i);
          if ((chgset & mask) != 0)
            {
             FAR const char *state;

              /* Get the button state */

              if ((newset & mask) != 0)
                {
                  state = "selected";
                }
              else
                {
                  state = "released";
                }

              printf("  %-6s: %s\n", g_ajoynames[i], state);
            }
        }
    }
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void show_joystick(FAR const struct ajoy_sample_s *sample)
{
  printf("  RAW: X=%d Y=%d\n", sample->as_x, sample->as_y);
  if (g_calibrated)
    {
      int tmp;
      int x;
      int y;

      tmp = sample->as_x - g_xcenter;
      if ((g_lispositive && tmp >= 0) ||
          (!g_lispositive && tmp < 0))
        {
           x = tmp * g_xlslope;
        }
      else
        {
           x = tmp * g_xrslope;
        }

      tmp = sample->as_y - g_ycenter;
      if ((g_fispositive && tmp >= 0) ||
          (!g_fispositive && tmp < 0))
        {
           y = tmp * g_yfslope;
        }
      else
        {
           y = tmp * g_ybslope;
        }

      printf("  Calibrated: X=%d Y=%d\n", x, y);
    }
}

static int ajoy_wait(int fd, FAR const struct timespec *timeout)
{
  sigset_t set;
  struct siginfo value;
  ajoy_buttonset_t newset;
  int ret;

  /* Wait for a signal */

  sigemptyset(&set);
  sigaddset(&set, CONFIG_EXAMPLES_AJOYSTICK_SIGNO);
  ret = sigtimedwait(&set, &value, timeout);
  if (ret < 0)
    {
      int errcode = errno;

      if (!timeout || errcode != EAGAIN)
        {
          fprintf(stderr, "ERROR: sigtimedwait() failed: %d\n", errcode);
          return -errcode;
        }

      /* Timeout is not an error */

      printf("Timeout!\n");
      return OK;
    }

  /* Show the set of joystick buttons accompanying the signal */

  printf("Signalled button set\n");
  newset = (ajoy_buttonset_t)value.si_value.sival_int;
  show_buttons(g_ajoylast, newset);
  g_ajoylast = newset;
  return OK;
}

static int ajoy_read(int fd, FAR struct ajoy_sample_s *sample)
{
  ssize_t nread;

  /* Read the joystack sample */

  nread = read(fd, sample, sizeof(struct ajoy_sample_s));
  if (nread < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: read() failed: %d\n", errcode);
      return -errcode;
    }
  else if (nread != sizeof(struct ajoy_sample_s))
    {
      fprintf(stderr, "ERROR: read() unexpected size: %ld vs %d\n",
              (long)nread, sizeof(struct ajoy_sample_s));
      return -EIO;
    }

  /* Show the joystick position and set buttons accompanying the signal */
  /* Show the set of joystick buttons that we just read */

  printf("Read position and button set\n");
  show_joystick(sample);
  show_buttons(g_ajoylast, sample->as_buttons);
  g_ajoylast =  sample->as_buttons;
  return OK;
}

static int ajoy_waitread(int fd, FAR const struct timespec *timeout,
                         FAR struct ajoy_sample_s *sample)
{
  int ret;

  /* Wait for a signal */

  ret = ajoy_wait(fd, timeout);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ajoy_wait() failed: %d\n", ret);
      return ret;
    }

  /* Read the joystick sample */

  ret = ajoy_read(fd, sample);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ajoy_read() failed: %d\n", ret);
      return ret;
    }

  return OK;
}

static b16_t ajoy_slope(int16_t value, int32_t full_range)
{
  return itob16(full_range) / (int32_t)value;
}

static int ajoy_calibrate(int fd)
{
  struct ajoy_sample_s sample;
  int ret;

  printf("Calibrating the joystick...\n");

  /* Get the center position */

  printf("Center the joystick and press any button\n");
  ret = ajoy_waitread(fd, NULL, &sample);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ajoy_waitread() failed: %d\n", errno);
      return ret;
    }

  g_xcenter = sample.as_x;
  g_ycenter = sample.as_y;

  /* Get the left/right calibration data */

  printf("Move the joystick to the far RIGHT and press any button\n");
  ret = ajoy_waitread(fd, NULL, &sample);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ajoy_waitread() failed: %d\n", errno);
      return ret;
    }

  g_lispositive = (sample.as_x > g_xcenter);
  g_xlslope = ajoy_slope(sample.as_x - g_xcenter, FULL_RANGE);

  printf("Move the joystick to the far LEFT and press any button\n");
  ret = ajoy_waitread(fd, NULL, &sample);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ajoy_waitread() failed: %d\n", errno);
      return ret;
    }

  g_xrslope = ajoy_slope(sample.as_x - g_xcenter, -FULL_RANGE);

  /* Get the forward/backward calibration data */

  printf("Move the joystick to the far FORWARD and press any button\n");
  ret = ajoy_waitread(fd, NULL, &sample);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ajoy_waitread() failed: %d\n", errno);
      return ret;
    }

  g_fispositive = (sample.as_y > g_ycenter);
  g_yfslope = ajoy_slope(sample.as_y - g_ycenter, FULL_RANGE);

  printf("Move the joystick to the far BACKWARD and press any button\n");
  ret = ajoy_waitread(fd, NULL, &sample);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ajoy_waitread() failed: %d\n", errno);
      return ret;
    }

  g_ybslope = ajoy_slope(sample.as_y - g_ycenter, -FULL_RANGE);

  printf("Calibrated:\n");
  g_calibrated = true;

  printf("  X: center=%d, R-slope=%08lx L-slope=%08lx\n",
         g_xcenter, g_xrslope, g_xlslope);
  printf("  Y: center=%d, F-slope=%08lx B-slope=%08lx\n",
         g_ycenter, g_yfslope, g_ybslope);
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * ajoy_main
 ****************************************************************************/

int main(int argc, char *argv[])
{
  struct timespec timeout;
  struct ajoy_notify_s notify;
  int fd;
  int tmp;
  int ret;
  int errcode = EXIT_FAILURE;

  /* Reset some globals that might been been left in a bad state */

  g_ajoylast = 0;

  /* Open the ajoystick device */

  fd = open(CONFIG_EXAMPLES_AJOYSTICK_DEVNAME, O_RDONLY);
  if (fd < 0)
    {
      fprintf(stderr, "ERROR: Failed to open %s: %d\n",
              CONFIG_EXAMPLES_AJOYSTICK_DEVNAME, errno);
      return EXIT_FAILURE;
    }

  /* Get the set of supported buttons */

  ret = ioctl(fd, AJOYIOC_SUPPORTED,  (unsigned long)((uintptr_t)&tmp));
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ioctl(AJOYIOC_SUPPORTED) failed: %d\n", errno);
      goto errout_with_fd;
    }

  g_ajoysupported = (ajoy_buttonset_t)tmp;
  printf("Supported joystick buttons:\n");
  show_buttons(0, g_ajoysupported);

  /* Register to receive a signal on any change in the joystick buttons. */

  notify.an_press   = g_ajoysupported;
  notify.an_release = g_ajoysupported;

  notify.an_event.sigev_notify = SIGEV_SIGNAL;
  notify.an_event.sigev_signo  = CONFIG_EXAMPLES_AJOYSTICK_SIGNO;

  ret = ioctl(fd, AJOYIOC_REGISTER, (unsigned long)((uintptr_t)&notify));
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ioctl(AJOYIOC_REGISTER) failed: %d\n", errno);
      goto errout_with_fd;
    }

  /* Calibrate the joystick */

  ajoy_calibrate(fd);

  /* Then loop, receiving signals indicating joystick events. */

  timeout.tv_sec  = 0;
  timeout.tv_nsec = 600*1000*1000;

  for (;;)
    {
      struct ajoy_sample_s sample;

      /* Wait for a signal and read the sample */

      ret = ajoy_waitread(fd, &timeout, &sample);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: ajoy_waitread() failed: %d\n", errno);
          goto errout_with_fd;
        }
    }

  errcode = EXIT_SUCCESS;

errout_with_fd:
  close(fd);
  return errcode;
}
