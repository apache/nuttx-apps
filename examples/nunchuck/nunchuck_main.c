/****************************************************************************
 * examplex/nunchuck/nunchuck_main.c
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

#include <nuttx/input/nunchuck.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Configuration ************************************************************/

#ifndef CONFIG_INPUT_NUNCHUCK
#  error "CONFIG_NUNCHUCK is not defined in the configuration"
#endif

#ifndef CONFIG_EXAMPLES_NUNCHUCK_DEVNAME
#  define CONFIG_EXAMPLES_NUNCHUCK_DEVNAME "/dev/nunchuck0"
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

static void show_buttons(nunchuck_buttonset_t oldset, nunchuck_buttonset_t newset);
static void show_joystick(FAR const struct nunchuck_sample_s *sample);
static int nunchuck_read(int fd, FAR struct nunchuck_sample_s *sample);
static int nunchuck_calibrate(int fd);

/****************************************************************************
 * Private Data
 ****************************************************************************/
/* The set of supported joystick buttons */

static nunchuck_buttonset_t g_nunchucksupported;

/* Last sampled button set */

static nunchuck_buttonset_t g_nunchucklast;

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

static const char *g_nunchucknames[NUNCHUCK_NBUTTONS] =
{
  "FIRE", "SELECT",
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void show_buttons(nunchuck_buttonset_t oldset, nunchuck_buttonset_t newset)
{
  nunchuck_buttonset_t chgset = oldset ^ newset;
  int i;

  if (chgset != 0)
    {
      /* Show each button state change */

      for (i = 0; i <= NUNCHUCK_NBUTTONS; i++)
        {
          nunchuck_buttonset_t mask = (1 << i);
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

              printf("  %-6s: %s\n", g_nunchucknames[i], state);
            }
        }
    }
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void show_joystick(FAR const struct nunchuck_sample_s *sample)
{
  printf("  RAW: X=%d Y=%d\n", sample->js_x, sample->js_y);
  if (g_calibrated)
    {
      int tmp;
      int x;
      int y;

      tmp = sample->js_x - g_xcenter;
      if ((g_lispositive && tmp >= 0) ||
          (!g_lispositive && tmp < 0))
        {
           x = tmp * g_xlslope;
        }
      else
        {
           x = tmp * g_xrslope;
        }

      tmp = sample->js_y - g_ycenter;
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

static int nunchuck_read(int fd, FAR struct nunchuck_sample_s *sample)
{
  ssize_t nread;

  /* Read the joystack sample */

  nread = read(fd, sample, sizeof(struct nunchuck_sample_s));
  if (nread < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: read() failed: %d\n", errcode);
      return -errcode;
    }
  else if (nread != sizeof(struct nunchuck_sample_s))
    {
      fprintf(stderr, "ERROR: read() unexpected size: %ld vs %d\n",
              (long)nread, sizeof(struct nunchuck_sample_s));
      return -EIO;
    }

#ifdef CONFIG_DEBUG_INPUT
  /* Show the joystick position and set buttons accompanying the signal */
  /* Show the set of joystick buttons that we just read */

  printf("Read position and button set\n");
  show_joystick(sample);
  show_buttons(g_nunchucklast, sample->nck_buttons);
#endif
  usleep(10000);
  return OK;
}

static b16_t nunchuck_slope(int16_t value, int32_t full_range)
{
  return itob16(full_range) / (int32_t)value;
}

static int nunchuck_calibrate(int fd)
{
  struct nunchuck_sample_s sample;
  int ret;

  printf("Calibrating the joystick...\n");

  /* Get the center position */

  printf("Center the joystick and press any button\n");

  sample.nck_buttons = 0;

  while (sample.nck_buttons == 0)
    {
      ret = nunchuck_read(fd, &sample);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: nunchuck_read() failed: %d\n", errno);
          return ret;
        }
    }

  g_xcenter = sample.js_x;
  g_ycenter = sample.js_y;

  /* Wait release the button */

  while (sample.nck_buttons != 0)
    {
      ret = nunchuck_read(fd, &sample);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: nunchuck_read() failed: %d\n", errno);
          return ret;
        }
    }

  /* Get the left/right calibration data */

  printf("Move the joystick to the far RIGHT and press any button\n");

  while (sample.nck_buttons == 0)
    {
      ret = nunchuck_read(fd, &sample);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: nunchuck_read() failed: %d\n", errno);
          return ret;
        }
    }

  g_lispositive = (sample.js_x > g_xcenter);
  g_xlslope = nunchuck_slope(sample.js_x - g_xcenter, FULL_RANGE);

  /* Wait release the button */

  while (sample.nck_buttons != 0)
    {
      ret = nunchuck_read(fd, &sample);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: nunchuck_read() failed: %d\n", errno);
          return ret;
        }
    }

  printf("Move the joystick to the far LEFT and press any button\n");

  while (sample.nck_buttons == 0)
    {
      ret = nunchuck_read(fd, &sample);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: nunchuck_read() failed: %d\n", errno);
          return ret;
        }
    }

  g_xrslope = nunchuck_slope(sample.js_x - g_xcenter, -FULL_RANGE);

  /* Wait release the button */

  while (sample.nck_buttons != 0)
    {
      ret = nunchuck_read(fd, &sample);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: nunchuck_read() failed: %d\n", errno);
          return ret;
        }
    }

  /* Get the forward/backward calibration data */

  printf("Move the joystick to the far FORWARD and press any button\n");

  while (sample.nck_buttons == 0)
    {
      ret = nunchuck_read(fd, &sample);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: nunchuck_read() failed: %d\n", errno);
          return ret;
        }
    }

  g_fispositive = (sample.js_y > g_ycenter);
  g_yfslope = nunchuck_slope(sample.js_y - g_ycenter, FULL_RANGE);

  /* Wait release the button */

  while (sample.nck_buttons != 0)
    {
      ret = nunchuck_read(fd, &sample);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: nunchuck_read() failed: %d\n", errno);
          return ret;
        }
    }

  printf("Move the joystick to the far BACKWARD and press any button\n");

  while (sample.nck_buttons == 0)
    {
      ret = nunchuck_read(fd, &sample);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: nunchuck_read() failed: %d\n", errno);
          return ret;
        }
    }

  g_ybslope = nunchuck_slope(sample.js_y - g_ycenter, -FULL_RANGE);

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
 * nunchuck_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int fd;
  int tmp;
  int ret;
  int errcode = EXIT_FAILURE;

  /* Reset some globals that might been been left in a bad state */

  g_nunchucklast = 0;

  /* Open the nunchuck device */

  fd = open(CONFIG_EXAMPLES_NUNCHUCK_DEVNAME, O_RDONLY);
  if (fd < 0)
    {
      fprintf(stderr, "ERROR: Failed to open %s: %d\n",
              CONFIG_EXAMPLES_NUNCHUCK_DEVNAME, errno);
      return EXIT_FAILURE;
    }

  /* Get the set of supported buttons */

  ret = ioctl(fd, NUNCHUCKIOC_SUPPORTED,  (unsigned long)((uintptr_t)&tmp));
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ioctl(NUNCHUCKIOC_SUPPORTED) failed: %d\n", errno);
      goto errout_with_fd;
    }

  g_nunchucksupported = (nunchuck_buttonset_t)tmp;
  printf("Supported joystick buttons:\n");
  show_buttons(0, g_nunchucksupported);

  /* Calibrate the joystick */

  nunchuck_calibrate(fd);

  for (;;)
    {
      struct nunchuck_sample_s sample;

      /* Wait for a signal and read the sample */

      ret = nunchuck_read(fd, &sample);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: nunchuck_read() failed: %d\n", errno);
          goto errout_with_fd;
        }

      /* Show read sample */

      show_joystick(&sample);
      show_buttons(g_nunchucklast, sample.nck_buttons);
      g_nunchucklast =  sample.nck_buttons;
    }

  errcode = EXIT_SUCCESS;

errout_with_fd:
  close(fd);
  return errcode;
}
