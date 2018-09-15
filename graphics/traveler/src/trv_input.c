/****************************************************************************
 * apps/graphics/traveler/trv_input.c
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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

#include "trv_types.h"
#include "trv_main.h"
#include "trv_world.h"
#include "trv_trigtbl.h"
#include "trv_debug.h"
#include "trv_input.h"

#if defined(CONFIG_GRAPHICS_TRAVELER_JOYSTICK)
#  include <sys/ioctl.h>
#  include <stdio.h>
#  include <unistd.h>
#  include <fcntl.h>
#  include <signal.h>
#  include <errno.h>

#if defined(CONFIG_GRAPHICS_TRAVELER_AJOYSTICK)
#  include <fixedmath.h>
#  include <nuttx/input/ajoystick.h>
#elif defined(CONFIG_GRAPHICS_TRAVELER_DJOYSTICK)
#  include <nuttx/input/djoystick.h>
#endif

#elif defined(CONFIG_GRAPHICS_TRAVELER_NX_XYINPUT)
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define RUN_RATE     (2 * STEP_DISTANCE)
#define WALK_RATE    (STEP_DISTANCE)
#define MAX_RATE     RUN_RATE

#define SLOW_TURN    ANGLE_6
#define FAST_TURN    ANGLE_9
#define MAX_TURN     ANGLE_12

#if defined(CONFIG_GRAPHICS_TRAVELER_AJOYSTICK)
#  define BUTTON_SET (AJOY_BUTTON_SELECT_BIT | AJOY_BUTTON_FIRE_BIT)
#elif defined(CONFIG_GRAPHICS_TRAVELER_DJOYSTICK)
#  define BUTTON_SET (DJOY_BUTTON_SELECT_BIT | DJOY_BUTTON_FIRE_BIT | \
                      DJOY_BUTTON_RUN_BIT)
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct trv_joystick_s
{
#if defined(CONFIG_GRAPHICS_TRAVELER_JOYSTICK)
  int fd;                   /* Open driver descriptor */
#ifdef CONFIG_GRAPHICS_TRAVELER_AJOYSTICK
  bool lplus;               /* Left is positive */
  bool fplus;               /* Forward is positive */
  int16_t centerx;          /* Center X position */
  b16_t leftslope;          /* Slope for movement left of center (x) */
  b16_t lturnslope;         /* Slope for turning to the left (yaw) */
  b16_t rightslope;         /* Slope for movement right of center (x) */
  b16_t rturnslope;         /* Slope for turning to the right (yaw) */
  int16_t centery;          /* Center Y position */
  b16_t fwdslope;           /* Slope for movement forward from center (y) */
  b16_t uturnslope;         /* Slope for nodding upward (pitch) */
  b16_t backslope;          /* Slope for backward from center (y) */
  b16_t dturnslope;         /* Slope for nodding downward (pitch) */
#endif
#elif defined(CONFIG_GRAPHICS_TRAVELER_NX_XYINPUT)
  trv_coord_t xpos;         /* Reported X position */
  trv_coord_t ypos;         /* Reported Y position */
  uint8_t buttons;          /* Report button set */
#endif
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Report positional inputs */

struct trv_input_s g_trv_input;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_GRAPHICS_TRAVELER_JOYSTICK
static struct trv_joystick_s g_trv_joystick;
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_joystick_wait
 *
 * Description:
 *   Wait for any joystick button to be pressed
 *
 ****************************************************************************/

#ifdef CONFIG_GRAPHICS_TRAVELER_AJOYSTICK
static int trv_joystick_wait(void)
{
  sigset_t set;
  int ret;

  /* Wait for a signal */

  (void)sigemptyset(&set);
  (void)sigaddset(&set, CONFIG_GRAPHICS_TRAVELER_JOYSTICK_SIGNO);
  ret = sigwaitinfo(&set, NULL);
  if (ret < 0)
    {
      int errcode = errno;

      fprintf(stderr, "ERROR: sigwaitinfo() failed: %d\n", errcode);
      return -errcode;
    }

  return OK;
}
#endif

/****************************************************************************
 * Name: trv_joystick_read
 *
 * Description:
 *   Read one sample from the analog joystick
 *
 ****************************************************************************/

#ifdef CONFIG_GRAPHICS_TRAVELER_AJOYSTICK
static int trv_joystick_read(FAR struct ajoy_sample_s *sample)
{
  ssize_t nread;

  /* Read the joystack sample */

  nread = read(g_trv_joystick.fd, sample, sizeof(struct ajoy_sample_s));
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

  return OK;
}
#endif

/****************************************************************************
 * Name: trv_joystick_sample
 *
 * Description:
 *   Wait for a button to be pressed on the joystick
 *
 ****************************************************************************/

#ifdef CONFIG_GRAPHICS_TRAVELER_AJOYSTICK
static int trv_joystick_sample(FAR struct ajoy_sample_s *sample)
{
  int ret;

  /* Wait for a signal */

  ret = trv_joystick_wait();
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: trv_joystick_wait() failed: %d\n", ret);
      return ret;
    }

  /* Read the joystick sample */

  ret = trv_joystick_read(sample);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: trv_joystick_read() failed: %d\n", ret);
      return ret;
    }

  return OK;
}
#endif

/****************************************************************************
 * Name: trv_joystick_slope
 *
 * Description:
 *   Calculate the slope to calibrate one axis.
 *
 ****************************************************************************/

#ifdef CONFIG_GRAPHICS_TRAVELER_AJOYSTICK
static b16_t trv_joystick_slope(int16_t value, int32_t full_range)
{
  return itob16(full_range) / (int32_t)value;
}
#endif

/****************************************************************************
 * Name: trv_joystick_calibrate
 *
 * Description:
 *   Calibrate the joystick
 *
 ****************************************************************************/

#ifdef CONFIG_GRAPHICS_TRAVELER_AJOYSTICK
static int trv_joystick_calibrate(void)
{
  struct ajoy_sample_s sample;
  int ret;

  printf("Calibrating the joystick...\n");

  /* Get the center position */

  printf("Center the joystick and press any button\n");
  ret = trv_joystick_sample(&sample);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: trv_joystick_sample() failed: %d\n", errno);
      return ret;
    }

  g_trv_joystick.centerx = sample.as_x;
  g_trv_joystick.centery = sample.as_y;

  /* Get the left/right calibration data */

  printf("Move the joystick to the far RIGHT and press any button\n");
  ret = trv_joystick_sample(&sample);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: trv_joystick_sample() failed: %d\n", errno);
      return ret;
    }

  g_trv_joystick.lplus      = (sample.as_x > g_trv_joystick.centerx);
  g_trv_joystick.leftslope  = trv_joystick_slope(sample.as_x - g_trv_joystick.centerx, MAX_RATE);
  g_trv_joystick.lturnslope = trv_joystick_slope(sample.as_x - g_trv_joystick.centerx, MAX_TURN);

  printf("Move the joystick to the far LEFT and press any button\n");
  ret = trv_joystick_sample(&sample);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: trv_joystick_sample() failed: %d\n", errno);
      return ret;
    }

  g_trv_joystick.rightslope = trv_joystick_slope(sample.as_x - g_trv_joystick.centerx, -MAX_RATE);
  g_trv_joystick.rturnslope = trv_joystick_slope(sample.as_x - g_trv_joystick.centerx, -MAX_TURN);

  /* Get the forward/backward calibration data */

  printf("Move the joystick to the far FORWARD and press any button\n");
  ret = trv_joystick_sample(&sample);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: trv_joystick_sample() failed: %d\n", errno);
      return ret;
    }

  g_trv_joystick.fplus      = (sample.as_y > g_trv_joystick.centery);
  g_trv_joystick.fwdslope   = trv_joystick_slope(sample.as_y - g_trv_joystick.centery, MAX_RATE);
  g_trv_joystick.uturnslope = trv_joystick_slope(sample.as_y - g_trv_joystick.centery, MAX_TURN);

  printf("Move the joystick to the far BACKWARD and press any button\n");
  ret = trv_joystick_sample(&sample);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: trv_joystick_sample() failed: %d\n", errno);
      return ret;
    }

  g_trv_joystick.backslope  = trv_joystick_slope(sample.as_y - g_trv_joystick.centery, -MAX_RATE);
  g_trv_joystick.dturnslope = trv_joystick_slope(sample.as_y - g_trv_joystick.centery, -MAX_TURN);

  printf("Calibrated:\n");
  trv_debug("  X: center=%d, R-slope=%08lx L-slope=%08lx\n",
            g_trv_joystick.centerx, g_trv_joystick.rightslope,
            g_trv_joystick.leftslope);
  trv_debug("  Y: center=%d, F-slope=%08lx B-slope=%08lx\n",
            g_trv_joystick.centery, g_trv_joystick.fwdslope,
            g_trv_joystick.backslope);
  return OK;
}
#endif

/****************************************************************************
 * Name: trv_scale_input_{x,y,pitch, yaw}
 *
 * Description:
 *   Calibrate the joystick
 *
 ****************************************************************************/

#ifdef CONFIG_GRAPHICS_TRAVELER_AJOYSTICK
static int trv_scale_input_x(FAR const struct ajoy_sample_s *sample)
{
  int tmp;
  b16_t x16;
  int x;

  trv_vdebug("  RAW: X=%d\n", sample->as_x);

  tmp = sample->as_x - g_trv_joystick.centerx;
  if ((g_trv_joystick.lplus && tmp >= 0) || (!g_trv_joystick.lplus && tmp < 0))
    {
       x16 = tmp * g_trv_joystick.leftslope;
    }
  else
    {
       x16 = tmp * g_trv_joystick.rightslope;
    }

  x = b16toi(x16 + b16HALF);

  trv_vdebug("  Calibrated: X=%d\n", x);
  return x;
}

static int trv_scale_input_y(FAR const struct ajoy_sample_s *sample)
{
  int tmp;
  b16_t y16;
  int y;

  trv_vdebug("  RAW: Y=%d\n", sample->as_y);

  tmp = sample->as_y - g_trv_joystick.centery;
  if ((g_trv_joystick.fplus && tmp >= 0) || (!g_trv_joystick.fplus && tmp < 0))
    {
       y16 = tmp * g_trv_joystick.fwdslope;
    }
  else
    {
       y16 = tmp * g_trv_joystick.backslope;
    }

  y = b16toi(y16 + b16HALF);
  trv_vdebug("  Calibrated: Y=%d\n", y);
  return y;
}

static int trv_scale_input_yaw(FAR const struct ajoy_sample_s *sample)
{
  int tmp;
  b16_t yaw16;
  int yaw;

  trv_vdebug("  RAW: X=%d\n", sample->as_x);

  tmp = sample->as_x - g_trv_joystick.centerx;
  if ((g_trv_joystick.lplus && tmp >= 0) || (!g_trv_joystick.lplus && tmp < 0))
    {
       yaw16 = tmp * g_trv_joystick.lturnslope;
    }
  else
    {
       yaw16 = tmp * g_trv_joystick.rturnslope;
    }

  yaw = -b16toi(yaw16 + b16HALF);
  trv_vdebug("  Calibrated: pitch=%d\n", yaw);
  return yaw;
}

static int trv_scale_input_pitch(FAR const struct ajoy_sample_s *sample)
{
  int tmp;
  b16_t pitch16;
  int pitch;

  trv_vdebug("  RAW: Y=%d\n", sample->as_y);

  tmp = sample->as_y - g_trv_joystick.centery;
  if ((g_trv_joystick.fplus && tmp >= 0) || (!g_trv_joystick.fplus && tmp < 0))
    {
       pitch16 = tmp * g_trv_joystick.uturnslope;
    }
  else
    {
       pitch16 = tmp * g_trv_joystick.dturnslope;
    }

  pitch = b16toi(pitch16 + b16HALF);
  trv_vdebug("  Calibrated: pitch=%d\n", pitch);
  return pitch;
}
#endif


/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_input_initialize
 *
 * Description:
 *   Initialize the selected input device
 *
 ****************************************************************************/

void trv_input_initialize(void)
{
#if defined(CONFIG_GRAPHICS_TRAVELER_DJOYSTICK)
  struct djoy_notify_s notify;

  /* Open the joy stick device */

  g_trv_joystick.fd = open(CONFIG_GRAPHICS_TRAVELER_JOYDEV, O_RDONLY);
  if (g_trv_joystick.fd < 0)
    {
      trv_abort("ERROR: Failed to open %s: %d\n",
                CONFIG_GRAPHICS_TRAVELER_JOYDEV, errno);
    }

#elif defined(CONFIG_GRAPHICS_TRAVELER_AJOYSTICK)
  struct ajoy_notify_s notify;
  int ret;

  /* Open the joy stick device */

  g_trv_joystick.fd = open(CONFIG_GRAPHICS_TRAVELER_JOYDEV, O_RDONLY);
  if (g_trv_joystick.fd < 0)
    {
      trv_abort("ERROR: Failed to open %s: %d\n",
                CONFIG_GRAPHICS_TRAVELER_JOYDEV, errno);
    }

  /* Register to receive a signal on any change in the joystick buttons. */

  notify.an_press   = BUTTON_SET;
  notify.an_release = 0;
  notify.an_signo   = CONFIG_GRAPHICS_TRAVELER_JOYSTICK_SIGNO;

  ret = ioctl(g_trv_joystick.fd, AJOYIOC_REGISTER, (unsigned long)((uintptr_t)&notify));
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ioctl(AJOYIOC_REGISTER) failed: %d\n", errno);
      goto errout_with_fd;
    }

  /* Calibrate the analog joystick device */

  ret = trv_joystick_calibrate();
  if (ret < 0)
    {
      trv_abort("ERROR: Failed to calibrate joystick: %d\n", ret);
      goto errout_with_fd;
    }

  /* Disable any further button events. */

  notify.an_press   = 0;
  notify.an_release = 0;
  notify.an_signo   = CONFIG_GRAPHICS_TRAVELER_JOYSTICK_SIGNO;

  ret = ioctl(g_trv_joystick.fd, AJOYIOC_REGISTER, (unsigned long)((uintptr_t)&notify));
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ioctl(AJOYIOC_REGISTER) failed: %d\n", errno);
      goto errout_with_fd;
    }

  return;

errout_with_fd:
  close(g_trv_joystick.fd);
  g_trv_joystick.fd = -1;

#elif defined(CONFIG_GRAPHICS_TRAVELER_NX_XYINPUT)
  /* Set the position to the center of the display at eye-level */
#warning Missing logic
#endif
}

/****************************************************************************
 * Name: trv_input_read
 *
 * Description:
 *   Read the next input from the input device
 *
 ****************************************************************************/

void trv_input_read(void)
{
#if defined(CONFIG_GRAPHICS_TRAVELER_JOYSTICK)
#if defined(CONFIG_GRAPHICS_TRAVELER_AJOYSTICK)
  struct ajoy_sample_s sample;
  int ret;

  /* Read data from the analog joystick */

  ret = trv_joystick_read(&sample);
  if (ret < 0)
    {
      trv_abort("ERROR: trv_joystick_read() failed: %d\n", ret);
    }

  /* Determine the input data to return to the POV logic */
  /* Assuming moving slowing so we cannot step over tall things */

  g_trv_input.stepheight = g_walk_stepheight;

  /* Move forward or backward OR look up or down */

  g_trv_input.leftrate  = 0;
  g_trv_input.yawrate   = 0;
  g_trv_input.pitchrate = 0;
  g_trv_input.fwdrate   = 0;

  if ((sample.as_buttons & AJOY_BUTTON_FIRE_BIT) != 0)
    {
      /* Move left/rignt */

      g_trv_input.leftrate = trv_scale_input_x(&sample);

      /* Look upward/downward */

      g_trv_input.pitchrate = trv_scale_input_pitch(&sample);

      /* If we are moving faster than a walk, we can jump higher */

      if (g_trv_input.leftrate < -WALK_RATE || g_trv_input.leftrate > WALK_RATE)
        {
          g_trv_input.stepheight = g_run_stepheight;
        }
    }
   else
    {
      /* Turn left/right */

      g_trv_input.yawrate = trv_scale_input_yaw(&sample);

      /* Move forward/backward */

      g_trv_input.fwdrate = trv_scale_input_y(&sample);

      /* If we are moving faster than a walk, we can jump higher */

      if (g_trv_input.fwdrate < -WALK_RATE || g_trv_input.fwdrate > WALK_RATE)
        {
          g_trv_input.stepheight = g_run_stepheight;
        }
    }

  g_trv_input.dooropen = ((sample.as_buttons & AJOY_BUTTON_SELECT_BIT) != 0);

#elif defined(CONFIG_GRAPHICS_TRAVELER_DJOYSTICK)
  struct djoy_buttonset_t buttonset;
  ssize_t nread;
  int16_t move_rate;
  int16_t turn_rate;

  /* Read data from the discrete joystick */

  nread = read(g_trv_joystick.fd, &buttonset, sizeof(djoy_buttonset_t));
  if (nread < 0)
    {
      trv_abort("ERROR: Joystick read error: %d\n", errno);
    }
  else if (nread != sizeof(struct djoy_buttonset_t))
    {
      trv_abort("ERROR: Unexpected joystick read size: %ld\n", (long)nread);
    }

  /* Determine the input data to return to the POV logic */

  if ((buttonset & DJOY_BUTTON_RUN_BIT) != 0)
    {
      /* Run faster/step higher */

      g_trv_joystick.stepheight = g_run_stepheight;
      move_rate              = RUN_RATE;
      turn_rate              = ANGLE_9;
    }
  else
    {
      /* Normal walking rate and stepping height */

      g_trv_joystick.stepheight = g_walk_stepheight;
      move_rate              = WALK_RATE;
      turn_rate              = ANGLE_6;
    }

  /* Move forward or backward OR look up or down */

  g_trv_joystick.pitchrate = 0;
  g_trv_joystick.fwdrate   = 0;

  switch (buttonset & (DJOY_UP_BIT | DJOY_DOWN_BIT))
    {
      case 0:
      case (DJOY_UP_BIT | DJOY_DOWN_BIT):
        /* Don't move, don't nod */

        break;

      case DJOY_UP_BIT:
        if ((buttonset & DJOY_BUTTON_FIRE_BIT) != 0)
          {
            /* Look upward */

            g_trv_input.pitchrate = turn_rate;
          }
        else
          {
            /* Move forward */

            g_trv_input.fwdrate   = move_rate;
          }
        break;

      case DJOY_DOWN_BIT:
        if ((buttonset & DJOY_BUTTON_FIRE_BIT) != 0)
          {
            /* Look downward */

            g_trv_input.pitchrate = -turn_rate;
          }
        else
          {
            /* Move Backward */

            g_trv_input.fwdrate = -move_rate;
          }
        break;
    }

  /* Move or loook left or right */

  g_trv_input.yawrate  = 0;
  g_trv_input.leftrate = 0;

  switch (buttonset & (DJOY_LEFT_BIT | DJOY_RIGHT_BIT))
    {
      case 0:
      case (DJOY_LEFT_BIT | DJOY_RIGHT_BIT):
        /* Don't move, don't nod */

        break;

      case DJOY_LEFT_BIT:
        if ((buttonset & DJOY_BUTTON_FIRE_BIT) != 0)
          {
            /* Turn left */

            g_trv_input.yawrate = turn_rate;
          }
        else
          {
            /* Move left */

            g_trv_input.leftrate   = move_rate;
          }
        break;

      case DJOY_RIGHT_BIT:
        if ((buttonset & DJOY_BUTTON_FIRE_BIT) != 0)
          {
            /* Turn right */

            g_trv_input.yawrate = -turn_rate;
          }
        else
          {
            /* Move right */

            g_trv_input.leftrate = -move_rate;
          }
        break;
    }

  g_trv_input.dooropen = ((buttonset & DJOY_BUTTON_SELECT_BIT) != 0);

#endif /* CONFIG_GRAPHICS_TRAVELER_DJOYSTICK */
#elif defined(CONFIG_GRAPHICS_TRAVELER_NX_XYINPUT)
  /* Make position decision based on last sampled X/Y input data */
#warning Missing logic
#endif
}

/****************************************************************************
 * Name: trv_input_terminate
 *
 * Description:
 *   Terminate input and free resources
 *
 ****************************************************************************/

void trv_input_terminate(void)
{
#ifdef CONFIG_GRAPHICS_TRAVELER_JOYSTICK
  if (g_trv_joystick.fd > 0)
    {
      close(g_trv_joystick.fd);
    }
#endif
}

/****************************************************************************
 * Name: trv_input_xyinput
 *
 * Description:
 *   Receive X/Y input from NX
 *
 ****************************************************************************/

#ifdef CONFIG_GRAPHICS_TRAVELER_NX_XYINPUT
void trv_input_xyinput(trv_coord_t xpos, trv_coord_t xpos, uint8_t buttons)
{
  /* Just save the positional data and button presses for now.  We will
   * decide what to do with the data when we are polled for new input.
   */

  g_trv_joystick.xpos    = xpos;
  g_trv_joystick.ypos    = ypos;
  g_trv_joystick.buttons = buttons;
}
#endif
