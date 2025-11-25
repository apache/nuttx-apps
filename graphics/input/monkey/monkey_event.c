/****************************************************************************
 * apps/graphics/input/monkey/monkey_event.c
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

#include <string.h>
#include <unistd.h>
#include <graphics/input_gen.h>
#include "monkey_dev.h"
#include "monkey_event.h"
#include "monkey_log.h"
#include "monkey_utils.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: monkey_event_gen
 ****************************************************************************/

void monkey_event_gen(FAR struct monkey_s *monkey,
                      FAR struct monkey_event_param_s *param)
{
  int weight = 0;
  int total = 0;
  int i;
  int rnd;
  int duration_min;
  int duration_max;
  int x_offset;
  int y_offset;

  memset(param, 0, sizeof(struct monkey_event_param_s));

  /* Calculate total weight */

  for (i = 0; i < MONKEY_EVENT_LAST; i++)
    {
      total += monkey->config.event[i].weight;
    }

  rnd = monkey_random(0, total);

  /* Select random events based on weight */

  for (i = 0; i < MONKEY_EVENT_LAST; i++)
    {
      weight += monkey->config.event[i].weight;

      if (rnd < weight)
        {
            param->event = i;
            break;
        }
    }

  duration_min = monkey->config.event[param->event].duration_min;
  duration_max = monkey->config.event[param->event].duration_max;
  x_offset = monkey->config.screen.x_offset;
  y_offset = monkey->config.screen.y_offset;

  param->duration = monkey_random(duration_min, duration_max);
  param->x1 = monkey_random(x_offset,
                            x_offset + monkey->config.screen.hor_res - 1);
  param->y1 = monkey_random(y_offset,
                            y_offset + monkey->config.screen.ver_res - 1);
  param->x2 = monkey_random(x_offset,
                            x_offset + monkey->config.screen.hor_res - 1);
  param->y2 = monkey_random(y_offset,
                            y_offset + monkey->config.screen.ver_res - 1);

  MONKEY_LOG_INFO("event=%d(%s) duration=%d x1=%d y1=%d x2=%d y2=%d",
                  param->event,
                  monkey_event_type2name(param->event),
                  param->duration,
                  param->x1, param->y1,
                  param->x2, param->y2);
}

/****************************************************************************
 * Name: monkey_event_exec
 ****************************************************************************/

bool monkey_event_exec(FAR struct monkey_s *monkey,
                       FAR struct monkey_dev_s *dev,
                       FAR const struct monkey_event_param_s *param)
{
  bool retval = false;
  enum monkey_dev_type_e dev_type = monkey_dev_get_type(dev);
  MONKEY_LOG_INFO("dev=0x%x event=%d(%s) duration=%d"
                  " x1=%d y1=%d x2=%d y2=%d",
                  dev_type,
                  param->event,
                  monkey_event_type2name(param->event),
                  param->duration,
                  param->x1, param->y1,
                  param->x2, param->y2);

  if (dev_type & MONKEY_DEV_TYPE_TOUCH)
    {
      if (param->event == MONKEY_EVENT_DRAG)
        {
          retval = !input_gen_drag(monkey->input_gen_ctx,
                                   param->x1, param->y1,
                                   param->x2, param->y2,
                                   param->duration);
        }
      else
        {
          /* Use swipe with same start and end point to simulate click or
           * longpress.
           */

          retval = !input_gen_swipe(monkey->input_gen_ctx,
                                    param->x1, param->y1,
                                    param->x1, param->y1,
                                    param->duration);
        }
    }
  else if (dev_type & MONKEY_DEV_TYPE_BUTTON)
    {
      retval = !input_gen_button_longpress(monkey->input_gen_ctx,
                                           1 << monkey->config.btn_bit,
                                           param->duration);
    }
  else
    {
      MONKEY_LOG_WARN("unsupported device type: %d", dev_type);
    }

  return retval;
}
