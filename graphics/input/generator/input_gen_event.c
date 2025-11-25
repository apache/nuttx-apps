/****************************************************************************
 * apps/graphics/input/generator/input_gen_event.c
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

#include <debug.h>
#include <stdint.h>
#include <unistd.h>

#include "input_gen_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define INPUT_GEN_DEFAULT_PRESSURE       42

#define INPUT_GEN_DEFAULT_PRESS_DURATION 50  /* ms */
#define INPUT_GEN_DEFAULT_HOLD_DURATION  5   /* ms */

#define INPUT_GEN_DEFAULT_CLICK_DURATION 100 /* ms */

#define INPUT_GEN_MAX_FINGERS            2

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Structure to hold parameters for single finger operations, we can support
 * multiple fingers by using multiple instances of this structure.
 */

struct input_gen_single_finger_s
{
  int16_t  x1;
  int16_t  y1;
  int16_t  x2;
  int16_t  y2;
  uint32_t press_duration;
  uint32_t move_duration;
  uint32_t hold_duration;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: input_gen_tick_get
 *
 * Description:
 *   Get the current tick count in milliseconds.
 *
 * Returned Value:
 *   The current tick count in milliseconds.
 *
 ****************************************************************************/

static uint32_t input_gen_tick_get(void)
{
  struct timespec ts;
  uint32_t        ms;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

  return ms;
}

/****************************************************************************
 * Name: input_gen_tick_elapsed
 *
 * Description:
 *   Calculate the elapsed time in milliseconds since the last tick.
 *
 * Input Parameters:
 *   act_time   - The current tick count.
 *   prev_tick  - The previous tick count.
 *
 * Returned Value:
 *   The elapsed time in milliseconds.
 *
 ****************************************************************************/

static uint32_t input_gen_tick_elapsed(uint32_t act_time, uint32_t prev_tick)
{
  /* If there is no overflow in sys_time simple subtract */

  if (act_time >= prev_tick)
    {
      prev_tick = act_time - prev_tick;
    }
  else
    {
      prev_tick  = UINT32_MAX - prev_tick + 1;
      prev_tick += act_time;
    }

  return prev_tick;
}

/****************************************************************************
 * Name: input_gen_single_finger
 *
 * Description:
 *   Calculate the position of a single finger based on the elapsed time and
 *   the parameters.
 *
 * Input Parameters:
 *   point   - The touch point structure to fill in.
 *   finger  - The parameters for the single finger operation.
 *   elapsed - The elapsed time since the start of the operation.
 *
 * Returned Value:
 *   True if the operation is finished, false otherwise.
 *
 ****************************************************************************/

static bool
input_gen_single_finger(FAR struct touch_point_s *point,
                        FAR const struct input_gen_single_finger_s *finger,
                        uint32_t elapsed)
{
  if (elapsed < finger->press_duration)
    {
      /* Press */

      input_gen_fill_point(point, finger->x1, finger->y1, TOUCH_DOWN);
      return false;
    }

  elapsed -= finger->press_duration;
  if (elapsed < finger->move_duration)
    {
      int16_t x = ((int64_t)(finger->x2 - finger->x1) * elapsed) /
                  finger->move_duration + finger->x1;
      int16_t y = ((int64_t)(finger->y2 - finger->y1) * elapsed) /
                  finger->move_duration + finger->y1;
      input_gen_fill_point(point, x, y, TOUCH_MOVE);

      return false;
    }

  elapsed -= finger->move_duration;
  if (elapsed < finger->hold_duration)
    {
      /* Hold */

      input_gen_fill_point(point, finger->x2, finger->y2, TOUCH_DOWN);
      return false;
    }

  /* Release */

  input_gen_fill_point(point, finger->x2, finger->y2, TOUCH_UP);
  return true;
}

/****************************************************************************
 * Name: input_gen_write_motion
 *
 * Description:
 *   Write a motion event to the device.
 *
 * Input Parameters:
 *   fd       - The file descriptor of the device.
 *   fingers  - The parameters for the motion operation.
 *   nfingers - The number of fingers.
 *   elapsed  - The elapsed time since the start of the operation.
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  On failure, a negated errno value is
 *   returned.
 *
 ****************************************************************************/

static int input_gen_write_motion(int fd,
                         FAR const struct input_gen_single_finger_s *fingers,
                         size_t nfingers, uint32_t elapsed)
{
  size_t finished = 0;
  size_t i;
  int ret;
  union /* A union with enough space to make compiler / checker happy. */
  {
    struct touch_sample_s sample;
    struct
      {
        uint8_t placeholder[offsetof(struct touch_sample_s, point)];
        struct touch_point_s point[INPUT_GEN_MAX_FINGERS];
      } points;
  } buffer;

  /* Check the number of fingers */

  if (nfingers < 1 || nfingers > INPUT_GEN_MAX_FINGERS)
    {
      return -EINVAL;
    }

  /* Fill the touch sample structure */

  buffer.sample.npoints = nfingers;

  for (i = 0; i < nfingers; i++)
    {
      FAR struct touch_point_s *point = &buffer.points.point[i];

      point->id = i;
      if (input_gen_single_finger(point, &fingers[i], elapsed))
        {
          finished++;
        }

      ginfo("Finger %zu: x = %d, y = %d, flags = %02X\n",
            i, point->x, point->y, point->flags);
    }

  /* Write the sample to the device */

  ret = input_gen_utouch_write(fd, &buffer.sample);
  if (ret < 0)
    {
      return ret;
    }

  return finished == nfingers ? OK : -EAGAIN;
}

/****************************************************************************
 * Name: input_gen_motion
 *
 * Description:
 *   Perform a motion operation.
 *
 * Input Parameters:
 *   ctx      - The input generator context.
 *   fingers  - The parameters for the motion operation.
 *   nfingers - The number of fingers.
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  On failure, a negated errno value is
 *   returned.
 *
 ****************************************************************************/

static int input_gen_motion(input_gen_ctx_t ctx,
                         FAR const struct input_gen_single_finger_s *fingers,
                         size_t nfingers)
{
  FAR struct input_gen_dev_s *dev;
  uint32_t start;
  int ret;

  dev = input_gen_search_dev(ctx, INPUT_GEN_DEV_UTOUCH);
  if (dev == NULL)
    {
      return -ENODEV;
    }

  ret = input_gen_grab(dev);
  if (ret < 0)
    {
      return ret;
    }

  start = input_gen_tick_get();

  do
    {
      uint32_t elapsed = input_gen_tick_elapsed(input_gen_tick_get(), start);
      ret = input_gen_write_motion(dev->fd, fingers, nfingers, elapsed);
      if (ret == -EAGAIN && usleep(8 * 1000) < 0)
        {
          nwarn("WARNING: Maybe interrupted by signal\n");
          input_gen_ungrab(dev);
          return -errno;
        }
    }
  while (ret == -EAGAIN);

  input_gen_ungrab(dev);
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: input_gen_tap
 *
 * Description:
 *   Perform a tap operation.
 *
 * Input Parameters:
 *   ctx - The input generator context.
 *   x   - The x coordinate.
 *   y   - The y coordinate.
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  On failure, a negated errno value is
 *   returned.
 *
 ****************************************************************************/

int input_gen_tap(input_gen_ctx_t ctx, int16_t x, int16_t y)
{
  struct input_gen_single_finger_s finger =
    {
      .x1 = x,
      .y1 = y,
      .x2 = x,
      .y2 = y,
      .press_duration = INPUT_GEN_DEFAULT_PRESS_DURATION,
      .move_duration  = 0,
      .hold_duration  = 0,
    };

  return input_gen_motion(ctx, &finger, 1);
}

/****************************************************************************
 * Name: input_gen_drag / input_gen_swipe
 *
 * Description:
 *   Perform a drag or swipe operation.
 *
 * Input Parameters:
 *   ctx      - The input generator context.
 *   x1       - The start x coordinate.
 *   y1       - The start y coordinate.
 *   x2       - The end x coordinate.
 *   y2       - The end y coordinate.
 *   duration - The duration of the operation.
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  On failure, a negated errno value is
 *   returned.
 *
 ****************************************************************************/

int input_gen_drag(input_gen_ctx_t ctx, int16_t x1, int16_t y1,
                   int16_t x2, int16_t y2, uint32_t duration)
{
  struct input_gen_single_finger_s finger =
    {
      .x1 = x1,
      .y1 = y1,
      .x2 = x2,
      .y2 = y2,
      .press_duration = INPUT_GEN_DEFAULT_PRESS_DURATION,
      .move_duration  = duration,
      .hold_duration  = INPUT_GEN_DEFAULT_HOLD_DURATION,
    };

  return input_gen_motion(ctx, &finger, 1);
}

int input_gen_swipe(input_gen_ctx_t ctx, int16_t x1, int16_t y1,
                    int16_t x2, int16_t y2, uint32_t duration)
{
  struct input_gen_single_finger_s finger =
    {
      .x1 = x1,
      .y1 = y1,
      .x2 = x2,
      .y2 = y2,
      .press_duration = 0,
      .move_duration  = duration,
      .hold_duration  = 0,
    };

  return input_gen_motion(ctx, &finger, 1);
}

/****************************************************************************
 * Name: input_gen_pinch
 *
 * Description:
 *   Perform a pinch operation.
 *
 * Input Parameters:
 *   ctx      - The input generator context.
 *   x1_start - The start x coordinate of the first finger.
 *   y1_start - The start y coordinate of the first finger.
 *   x2_start - The start x coordinate of the second finger.
 *   y2_start - The start y coordinate of the second finger.
 *   x1_end   - The end x coordinate of the first finger.
 *   y1_end   - The end y coordinate of the first finger.
 *   x2_end   - The end x coordinate of the second finger.
 *   y2_end   - The end y coordinate of the second finger.
 *   duration - The duration of the operation.
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  On failure, a negated errno value is
 *   returned.
 *
 ****************************************************************************/

int input_gen_pinch(input_gen_ctx_t ctx, int16_t x1_start, int16_t y1_start,
                    int16_t x2_start, int16_t y2_start, int16_t x1_end,
                    int16_t y1_end, int16_t x2_end, int16_t y2_end,
                    uint32_t duration)
{
  struct input_gen_single_finger_s fingers[2] =
    {
      {
        .x1 = x1_start,
        .y1 = y1_start,
        .x2 = x1_end,
        .y2 = y1_end,
        .press_duration = INPUT_GEN_DEFAULT_PRESS_DURATION,
        .move_duration  = duration,
        .hold_duration  = INPUT_GEN_DEFAULT_HOLD_DURATION,
      },
      {
        .x1 = x2_start,
        .y1 = y2_start,
        .x2 = x2_end,
        .y2 = y2_end,
        .press_duration = INPUT_GEN_DEFAULT_PRESS_DURATION,
        .move_duration  = duration,
        .hold_duration  = INPUT_GEN_DEFAULT_HOLD_DURATION,
      },
    };

  return input_gen_motion(ctx, fingers, 2);
}

/****************************************************************************
 * Name: input_gen_button_click / input_gen_button_longpress
 *
 * Description:
 *   Perform a button click or long press operation.
 *
 * Input Parameters:
 *   ctx      - The input generator context.
 *   mask     - The button mask.
 *   duration - The duration of the operation.
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  On failure, a negated errno value is
 *   returned.
 *
 ****************************************************************************/

int input_gen_button_click(input_gen_ctx_t ctx, btn_buttonset_t mask)
{
  return input_gen_button_longpress(ctx, mask,
                                    INPUT_GEN_DEFAULT_CLICK_DURATION);
}

int input_gen_button_longpress(input_gen_ctx_t ctx, btn_buttonset_t mask,
                               uint32_t duration)
{
  FAR struct input_gen_dev_s *dev;
  int ret;

  dev = input_gen_search_dev(ctx, INPUT_GEN_DEV_UBUTTON);
  if (dev == NULL)
    {
      return -ENODEV;
    }

  ret = input_gen_grab(dev);
  if (ret < 0)
    {
      return ret;
    }

  ret = input_gen_ubutton_write(dev->fd, mask);
  if (ret < 0)
    {
      goto out;
    }

  usleep(duration * 1000);

  ret = input_gen_ubutton_write(dev->fd, 0);
  if (ret < 0)
    {
      goto out;
    }

out:
  input_gen_ungrab(dev);
  return ret;
}

/****************************************************************************
 * Name: input_gen_mouse_wheel
 *
 * Description:
 *   Perform a mouse wheel operation.
 *
 * Input Parameters:
 *   ctx   - The input generator context.
 *   wheel - The wheel value.
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  On failure, a negated errno value is
 *   returned.
 *
 ****************************************************************************/

#ifdef CONFIG_INPUT_MOUSE_WHEEL
int input_gen_mouse_wheel(input_gen_ctx_t ctx, int16_t wheel)
{
  FAR struct input_gen_dev_s *dev;

  dev = input_gen_search_dev(ctx, INPUT_GEN_DEV_UMOUSE);
  if (dev == NULL)
    {
      return -ENODEV;
    }

  return input_gen_umouse_write(dev->fd, wheel);
}
#endif

/****************************************************************************
 * Name: input_gen_fill_point
 *
 * Description:
 *   Fill the touch point structure.
 *
 * Input Parameters:
 *   point - The touch point structure.
 *   x     - The x coordinate.
 *   y     - The y coordinate.
 *   flags - The TOUCH_DOWN, TOUCH_MOVE, TOUCH_UP flag.
 *
 ****************************************************************************/

void input_gen_fill_point(FAR struct touch_point_s *point,
                          int16_t x, int16_t y, uint8_t flags)
{
  point->x = x;
  point->y = y;
  point->h = 1;
  point->w = 1;

  if (flags & TOUCH_UP)
    {
      point->pressure = 0;
      point->flags    = flags | TOUCH_ID_VALID;
    }
  else
    {
      point->pressure = INPUT_GEN_DEFAULT_PRESSURE;
      point->flags    = flags | TOUCH_ID_VALID | TOUCH_POS_VALID |
                        TOUCH_PRESSURE_VALID;
    }
}
