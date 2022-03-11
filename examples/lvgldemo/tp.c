/****************************************************************************
 * apps/examples/lvgldemo/tp.c
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

#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>

#ifdef CONFIG_EXAMPLES_LVGLDEMO_MOUSE
#  include <nuttx/input/mouse.h>
#endif

#include <nuttx/input/touchscreen.h>

#include "tp.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int fd;
static bool calibrated = false;
static int x_range;
static int y_range;
static int x_offset;
static int y_offset;
static bool xy_inv;
static bool x_inv;
static bool y_inv;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tp_init
 *
 * Description:
 *   Initialize The Touch pad
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   Zero (OK) on success; a positive error code on failure.
 *
 ****************************************************************************/

int tp_init(void)
{
  int errval = 0;

  /* Open the touchscreen device for reading */

  printf("tp_init: Opening %s\n", CONFIG_EXAMPLES_LVGLDEMO_DEVPATH);
  fd = open(CONFIG_EXAMPLES_LVGLDEMO_DEVPATH, O_RDONLY | O_NONBLOCK);
  if (fd < 0)
    {
      printf("tp_init: open %s failed: %d\n",
               CONFIG_EXAMPLES_LVGLDEMO_DEVPATH, errno);
      errval = 2;
      goto errout;
    }

  return OK;

errout:
  printf("Terminating!\n");
  fflush(stdout);
  return errval;
}

/****************************************************************************
 * Name: tp_read
 *
 * Description:
 *   Read a TP data and store in 'data' argument
 *
 * Input Parameters:
 *   indev_drv - Input device handler
 *   data - Store the x, y and state information here
 *
 * Returned Value:
 *   false: no more data to read; true: there are more data to read.
 *
 ****************************************************************************/

bool tp_read(struct _lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
  struct touch_sample_s sample;
  int nbytes;
  static int last_x = 0;
  static int last_y = 0;
  static lv_indev_state_t last_state = LV_INDEV_STATE_REL;

  /* Be sure at least the previous state is set */

  data->point.x = last_x;
  data->point.y = last_y;
  data->state = last_state;

  /* Read one sample */

  nbytes = read(fd, &sample, sizeof(struct touch_sample_s));

  /* Handle unexpected return values */

  if (nbytes < 0 || nbytes != sizeof(struct touch_sample_s))
    {
      return false;
    }

  if (sample.point[0].flags & TOUCH_DOWN
      || sample.point[0].flags & TOUCH_MOVE)
    {
      if (calibrated)
        {
          if (xy_inv)
            {
              last_x = sample.point[0].y;
              last_y = sample.point[0].x;
            }
          else
            {
              last_x = sample.point[0].x;
              last_y = sample.point[0].y;
            }

          /* Remove offset */

          last_x -= x_offset;
          last_y -= y_offset;

          last_x = (int)((int)last_x * LV_HOR_RES) / x_range;
          last_y = (int)((int)last_y * LV_VER_RES) / y_range;

          if (x_inv)
            {
              last_x = LV_HOR_RES - last_x;
            }

          if (y_inv)
            {
              last_y = LV_VER_RES - last_y;
            }
        }
      else
        {
          last_x = sample.point[0].x;
          last_y = sample.point[0].y;
        }

      last_state = LV_INDEV_STATE_PR;
    }
  else if (sample.point[0].flags & TOUCH_UP)
    {
      last_state = LV_INDEV_STATE_REL;
    }
  else if (sample.point[0].flags & TOUCH_UP)
    {
      last_state = LV_INDEV_STATE_REL;
    }

  /* Update touchpad data */

  data->point.x = last_x;
  data->point.y = last_y;
  data->state = last_state;

  fflush(stdout);
  return false;
}

/****************************************************************************
 * Name: tp_read
 *
 * Description:
 *   Set calibration data
 *
 * Input Parameters:
 *   ul - Upper left hand corner TP value
 *   ur - Upper right hand corner TP value
 *   lr - Lower right hand corner TP value
 *   ll - Lower left hand corner TP value
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void tp_set_cal_values(FAR lv_point_t *ul, FAR lv_point_t *ur,
                       FAR lv_point_t *lr, FAR lv_point_t *ll)
{
  /* Is x/y inverted? */

  if (abs(ul->x - ur->x) < LV_HOR_RES / 2)
    {
      /* No real change in x horizontally */

      xy_inv = true;
    }

  if (xy_inv)
    {
      /* Is x inverted */

      if (ur->y < ul->y)
        {
          x_inv = true;
        }

      /* Is y inverted */

      if (ll->x < ul->x)
        {
          y_inv = true;
        }

      x_range  = abs(ul->y - ur->y);
      y_range  = abs(ul->x - ll->x);
      x_offset = x_inv ? ur->y : ul->y;
      y_offset = y_inv ? ll->x : ul->x;
    }
  else
    {
      /* Is x inverted */

      if (ur->x < ul->x)
        {
          x_inv = true;
        }

      /* Is y inverted */

      if (ll->y < ul->y)
        {
          y_inv = true;
        }

      x_range  = abs(ul->x - ur->x);
      y_range  = abs(ul->y - ll->y);
      x_offset = x_inv ? ur->x : ul->x;
      y_offset = y_inv ? ll->y : ul->y;
    }

  calibrated = true;

  printf("tp_cal result\n");
  printf("offset x:%d, y:%d\n", x_offset, y_offset);
  printf("range x:%d, y:%d\n", x_range, y_range);
  printf("invert x/y:%d, x:%d, y:%d\n\n", xy_inv, x_inv, y_inv);
}
