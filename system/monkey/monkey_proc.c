/****************************************************************************
 * apps/system/monkey/monkey_proc.c
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
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "monkey.h"
#include "monkey_assert.h"
#include "monkey_dev.h"
#include "monkey_event.h"
#include "monkey_log.h"
#include "monkey_recorder.h"
#include "monkey_utils.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: monkey_update_uinput_ramdom
 ****************************************************************************/

static bool monkey_update_uinput_ramdom(FAR struct monkey_s *monkey)
{
  int i;
  bool retval = false;

  for (i = 0; i < monkey->dev_num; i++)
    {
      FAR struct monkey_dev_s *dev = monkey->devs[i];
      struct monkey_event_param_s param;
      monkey_event_gen(monkey, &param);
      retval = monkey_event_exec(monkey, dev, &param);

      if (!retval)
        {
          break;
        }
    }

  return retval;
}

/****************************************************************************
 * Name: monkey_search_dev
 ****************************************************************************/

static FAR struct monkey_dev_s *monkey_search_dev(
                                              FAR struct monkey_s *monkey,
                                              enum monkey_dev_type_e type)
{
  int i;
  for (i = 0; i < monkey->dev_num; i++)
    {
      FAR struct monkey_dev_s *dev = monkey->devs[i];
      if (type == dev->type)
        {
          return dev;
        }
    }

  return NULL;
}

/****************************************************************************
 * Name: monkey_recorder_get_next
 ****************************************************************************/

static int monkey_recorder_get_next(FAR struct monkey_s *monkey,
                                FAR uint32_t *cur_time_stamp_p,
                                FAR struct monkey_dev_state_s *cur_state_p,
                                FAR uint32_t *next_time_stamp_p,
                                FAR struct monkey_dev_state_s *next_state_p)
{
  enum monkey_recorder_res_e res;

  if (monkey->playback_ctx.time_stamp == 0)
    {
      res = monkey_recorder_read(monkey->recorder,
                                 cur_state_p,
                                 cur_time_stamp_p);

      if (res != MONKEY_RECORDER_RES_OK)
        {
          MONKEY_LOG_ERROR("read first line failed: %d", res);
          return 0;
        }
    }
  else
    {
      *cur_time_stamp_p = monkey->playback_ctx.time_stamp;
      *cur_state_p = monkey->playback_ctx.state;
    }

  res = monkey_recorder_read(monkey->recorder,
                             next_state_p,
                             next_time_stamp_p);

  if (res != MONKEY_RECORDER_RES_OK)
    {
      if (res == MONKEY_RECORDER_RES_END_OF_FILE)
        {
          MONKEY_LOG_WARN("end of file");
          return 1;
        }

      MONKEY_LOG_ERROR("read error: %d", res);
      return 0;
    }

  monkey->playback_ctx.time_stamp = *next_time_stamp_p;
  monkey->playback_ctx.state = *next_state_p;
  return 2;
}

/****************************************************************************
 * Name: monkey_update_uinput_playback
 ****************************************************************************/

static bool monkey_update_uinput_playback(FAR struct monkey_s *monkey)
{
  FAR uint32_t cur_time_stamp;
  struct monkey_dev_state_s cur_state;
  FAR uint32_t next_time_stamp;
  struct monkey_dev_state_s next_state;

  uint32_t tick_elaps;
  FAR struct monkey_dev_s *dev;

  int num_of_get;

  MONKEY_ASSERT_NULL(monkey->recorder);

  num_of_get = monkey_recorder_get_next(monkey,
                                        &cur_time_stamp,
                                        &cur_state,
                                        &next_time_stamp,
                                        &next_state);

  switch (num_of_get)
    {
      case 1:
        next_time_stamp = cur_time_stamp;
        memset(&monkey->playback_ctx, 0, sizeof(monkey->playback_ctx));
        monkey_recorder_reset(monkey->recorder);
      case 2:
        dev = monkey_search_dev(monkey,
                                MONKEY_UINPUT_TYPE_MASK | cur_state.type);

        if (dev)
          {
            monkey_dev_set_state(dev, &cur_state);
          }
        else
          {
            MONKEY_LOG_WARN("unsupport device type: %d", cur_state.type);
          }

        tick_elaps = monkey_tick_elaps(next_time_stamp, cur_time_stamp);
        monkey_set_period(monkey, tick_elaps);
        break;

      default:
        MONKEY_LOG_ERROR("error num_of_get = %d", num_of_get);
        return false;
    }

  return true;
}

/****************************************************************************
 * Name: monkey_update_uinput
 ****************************************************************************/

static bool monkey_update_uinput(FAR struct monkey_s *monkey)
{
  bool retval = false;
  MONKEY_ASSERT_NULL(monkey);

  if (monkey->mode == MONKEY_MODE_RANDOM)
    {
      retval = monkey_update_uinput_ramdom(monkey);
    }
  else if(monkey->mode == MONKEY_MODE_PLAYBACK)
    {
      retval = monkey_update_uinput_playback(monkey);
    }
  else
    {
      MONKEY_LOG_ERROR("error mode: %d", monkey->mode);
    }

  return retval;
}

/****************************************************************************
 * Name: monkey_update_input
 ****************************************************************************/

static bool monkey_update_input(FAR struct monkey_s *monkey)
{
  struct monkey_dev_state_s state;
  int available;
  int i;

  MONKEY_ASSERT_NULL(monkey);

  available = monkey_dev_get_available(monkey->devs, monkey->dev_num);

  if (available <= 0)
    {
      MONKEY_LOG_WARN("no available device");
      return false;
    }

  for (i = 0; i < monkey->dev_num; i++)
    {
      FAR struct monkey_dev_s *dev = monkey->devs[i];
      if (dev->is_available)
        {
          /* try to get device state */

          if (!monkey_dev_get_state(dev, &state))
            {
              MONKEY_LOG_ERROR("can't get state");
              return false;
            }

          if (dev->type == MONKEY_DEV_TYPE_TOUCH)
            {
              MONKEY_LOG_INFO("touch %s at x = %d, y = %d",
                              state.data.touch.is_pressed
                              ? "PRESS  " : "RELEASE",
                              state.data.touch.x, state.data.touch.y);
            }
          else if (dev->type == MONKEY_DEV_TYPE_BUTTON)
            {
              MONKEY_LOG_INFO("btn = 0x%08X", state.data.button.value);
            }

          /* record state */

          if (monkey->recorder)
            {
              monkey_recorder_write(monkey->recorder, &state);
            }
        }
    }

  return true;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: monkey_update
 ****************************************************************************/

int monkey_update(FAR struct monkey_s *monkey)
{
  int wait_time;
  MONKEY_ASSERT_NULL(monkey);

  srand(monkey_tick_get());

  if (monkey->mode == MONKEY_MODE_RECORD)
    {
      if (monkey_update_input(monkey))
        {
          /* no need for sleep */

          return 0;
        }
      else
        {
          return -1;
        }
    }
  else
    {
      if (!monkey_update_uinput(monkey))
        {
          return -1;
        }
    }

  wait_time = monkey_random(monkey->config.period.min,
                            monkey->config.period.max);

  return wait_time;
}
