/****************************************************************************
 * apps/games/brickmatch/bm_input_touch.h
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

#ifndef __APPS_GAMES_BRICKMATCH_BM_INPUT_TOUCH_H
#define __APPS_GAMES_BRICKMATCH_BM_INPUT_TOUCH_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <nuttx/input/touchscreen.h>

#include "bm_inputs.h"

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

/* Tracks the in-progress drag so each dev_read_input() call only has to
 * look at the latest sample, not replay history.  Reset on TOUCH_UP so a
 * finger lift always starts the next swipe's baseline fresh; re-baselined
 * (rather than requiring a lift) after firing a direction, so a single
 * continued drag can fire more than one swipe.
 */

static bool g_bm_touch_active;
static int32_t g_bm_touch_origin_x;
static int32_t g_bm_touch_origin_y;

/****************************************************************************
 * Name: dev_input_init
 *
 * Description:
 *   Initialize input method.
 *
 * Parameters:
 *   dev - Input state data
 *
 * Returned Value:
 *   Zero (OK) is returned on success. A negated errno value is returned on
 *   failure.
 *
 ****************************************************************************/

int dev_input_init(FAR struct input_state_s *dev)
{
  dev->fd_touch = open(CONFIG_GAMES_BRICKMATCH_TOUCH_DEVPATH,
                       O_RDONLY | O_NONBLOCK);
  if (dev->fd_touch < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: Failed to open %s: %d\n",
              CONFIG_GAMES_BRICKMATCH_TOUCH_DEVPATH, errcode);
      return -errcode;
    }

  g_bm_touch_active = false;

  return OK;
}

/****************************************************************************
 * Name: dev_read_input
 *
 * Description:
 *   Read inputs and returns result in input state data.  Interprets a
 *   drag/swipe gesture rather than an absolute tap position, since
 *   brickmatch's board layout has no natural on-screen buttons to tap -
 *   this mirrors how the gesture-sensor backend (bm_input_gesture.h)
 *   reports discrete directions, just derived from a touchscreen instead
 *   of a dedicated gesture sensor.
 *
 * Parameters:
 *   dev - Input state data
 *
 * Returned Value:
 *   Zero (OK)
 *
 ****************************************************************************/

int dev_read_input(FAR struct input_state_s *dev)
{
  struct touch_sample_s sample;
  int32_t dx;
  int32_t dy;
  ssize_t nbytes;

  dev->dir = DIR_NONE;

  nbytes = read(dev->fd_touch, &sample, sizeof(sample));
  if (nbytes < 0)
    {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
          usleep(10000);
          return OK;
        }

      return -errno;
    }

  if (nbytes != sizeof(sample) || sample.npoints < 1)
    {
      return OK;
    }

  if (sample.point[0].flags & TOUCH_UP)
    {
      g_bm_touch_active = false;
      return OK;
    }

  if ((sample.point[0].flags & TOUCH_POS_VALID) == 0)
    {
      return OK;
    }

  if (sample.point[0].flags & TOUCH_DOWN)
    {
      g_bm_touch_active = true;
      g_bm_touch_origin_x = sample.point[0].x;
      g_bm_touch_origin_y = sample.point[0].y;
      return OK;
    }

  if (!g_bm_touch_active)
    {
      return OK;
    }

  dx = sample.point[0].x - g_bm_touch_origin_x;
  dy = sample.point[0].y - g_bm_touch_origin_y;

  if (abs(dx) < CONFIG_GAMES_BRICKMATCH_TOUCH_THRESHOLD &&
      abs(dy) < CONFIG_GAMES_BRICKMATCH_TOUCH_THRESHOLD)
    {
      return OK;
    }

  if (abs(dx) > abs(dy))
    {
      dev->dir = dx > 0 ? DIR_RIGHT : DIR_LEFT;
    }
  else
    {
      dev->dir = dy > 0 ? DIR_DOWN : DIR_UP;
    }

  g_bm_touch_origin_x = sample.point[0].x;
  g_bm_touch_origin_y = sample.point[0].y;

  return OK;
}

#endif /* __APPS_GAMES_BRICKMATCH_BM_INPUT_TOUCH_H */
