/****************************************************************************
 * apps/system/monkey/monkey_utils.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "monkey_log.h"
#include "monkey_utils.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct monkey_port_dev_type_name_s
{
  enum monkey_dev_type_e type;
  FAR const char *name;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct monkey_port_dev_type_name_s g_type_name_grp[] =
{
  { MONKEY_DEV_TYPE_TOUCH,   "touch"   },
  { MONKEY_DEV_TYPE_BUTTON,  "button"  },
  { MONKEY_DEV_TYPE_UTOUCH,  "utouch"  },
  { MONKEY_DEV_TYPE_UBUTTON, "ubutton" }
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: monkey_random
 ****************************************************************************/

int monkey_random(int min, int max)
{
  if (min >= max)
    {
      return min;
    }

  long diff = max - min + 1;
  return rand() % diff + min;
}

/****************************************************************************
 * Name: monkey_tick_get
 ****************************************************************************/

uint32_t monkey_tick_get(void)
{
  struct timespec ts;
  uint32_t ms;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
  return ms;
}

/****************************************************************************
 * Name: monkey_tick_elaps
 ****************************************************************************/

uint32_t monkey_tick_elaps(uint32_t act_time, uint32_t prev_tick)
{
  /* If there is no overflow in sys_time simple subtract */

  if (act_time >= prev_tick)
    {
      prev_tick = act_time - prev_tick;
    }
  else
    {
      prev_tick = UINT32_MAX - prev_tick + 1;
      prev_tick += act_time;
    }

  return prev_tick;
}

/****************************************************************************
 * Name: monkey_get_localtime_str
 ****************************************************************************/

void monkey_get_localtime_str(FAR char *str_buf, size_t buf_size)
{
  time_t rawtime;
  FAR struct tm *timeinfo;

  time(&rawtime);
  timeinfo = localtime(&rawtime);
  snprintf(str_buf, buf_size, "%04d%02d%02d_%02d%02d%02d",
           1900 + timeinfo->tm_year,
           timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_hour,
           timeinfo->tm_min, timeinfo->tm_sec);
}

/****************************************************************************
 * Name: monkey_dir_check
 ****************************************************************************/

bool monkey_dir_check(FAR const char *dir_path)
{
  bool retval = false;
  if (access(dir_path, F_OK) == 0)
    {
      MONKEY_LOG_NOTICE("directory: %s already exists", dir_path);
      retval = true;
    }
  else
    {
      MONKEY_LOG_WARN("can't access directory: %s, creating...", dir_path);
      if (mkdir(dir_path, 0777) == 0)
        {
          MONKEY_LOG_NOTICE("OK");
          retval = true;
        }
      else
        {
          MONKEY_LOG_ERROR("create directory: %s failed", dir_path);
        }
    }

  return retval;
}

/****************************************************************************
 * Name: monkey_map
 ****************************************************************************/

int monkey_map(int x, int min_in, int max_in, int min_out, int max_out)
{
  if (max_in >= min_in && x >= max_in)
    {
      return max_out;
    }

  if (max_in >= min_in && x <= min_in)
    {
      return min_out;
    }

  if (max_in <= min_in && x <= max_in)
    {
      return max_out;
    }

  if (max_in <= min_in && x >= min_in)
    {
      return min_out;
    }

  /* The equation should be:
   *   ((x - min_in) * delta_out) / delta in) + min_out
   * To avoid rounding error reorder the operations:
   *   (x - min_in) * (delta_out / delta_min) + min_out
   */

  int delta_in = max_in - min_in;
  int delta_out = max_out - min_out;

  return ((x - min_in) * delta_out) / delta_in + min_out;
}

/****************************************************************************
 * Name: monkey_dev_type2name
 ****************************************************************************/

FAR const char *monkey_dev_type2name(enum monkey_dev_type_e type)
{
  int i;
  const int grp_len = sizeof(g_type_name_grp)
                    / sizeof(struct monkey_port_dev_type_name_s);
  for (i = 0; i < grp_len; i++)
    {
      if (type == g_type_name_grp[i].type)
        {
          return g_type_name_grp[i].name;
        }
    }

  return "unknow";
}

/****************************************************************************
 * Name: monkey_dev_name2type
 ****************************************************************************/

enum monkey_dev_type_e monkey_dev_name2type(FAR const char *name)
{
  int i;
  const int grp_len = sizeof(g_type_name_grp)
                    / sizeof(struct monkey_port_dev_type_name_s);
  if (name)
    {
      for (i = 0; i < grp_len; i++)
        {
          if (strcmp(name, g_type_name_grp[i].name) == 0)
            {
              return g_type_name_grp[i].type;
            }
        }
    }

  return MONKEY_DEV_TYPE_UNKNOW;
}

/****************************************************************************
 * Name: monkey_event_type2name
 ****************************************************************************/

FAR const char *monkey_event_type2name(enum monkey_event_e event)
{
  switch (event)
    {
      case MONKEY_EVENT_CLICK:
        return "click";
      case MONKEY_EVENT_LONG_PRESS:
        return "long-press";
      case MONKEY_EVENT_DRAG:
        return "drag";
      default:
        break;
    }

  return "unknow";
}
