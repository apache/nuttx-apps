/****************************************************************************
 * apps/system/monkey/monkey_type.h
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

#ifndef __APPS_SYSTEM_MONKEY_MONKEY_TYPE_H
#define __APPS_SYSTEM_MONKEY_MONKEY_TYPE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stdbool.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MONKEY_UINPUT_TYPE_MASK     (0x10)
#define MONKEY_IS_UINPUT_TYPE(type) (!!((type) & MONKEY_UINPUT_TYPE_MASK))
#define MONKEY_GET_DEV_TYPE(type)   ((type) & ~MONKEY_UINPUT_TYPE_MASK)
#define MONKEY_DEV_MAX_NUM          2

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct monkey_dev_s;
struct monkey_recorder_s;

enum monkey_mode_e
{
  MONKEY_MODE_RANDOM,
  MONKEY_MODE_RECORD,
  MONKEY_MODE_PLAYBACK,
};

enum monkey_dev_type_e
{
  MONKEY_DEV_TYPE_UNKNOW  = 0x00,
  MONKEY_DEV_TYPE_TOUCH   = 0x01,
  MONKEY_DEV_TYPE_BUTTON  = 0x02,
  MONKEY_DEV_TYPE_UTOUCH  = MONKEY_UINPUT_TYPE_MASK | MONKEY_DEV_TYPE_TOUCH,
  MONKEY_DEV_TYPE_UBUTTON = MONKEY_UINPUT_TYPE_MASK | MONKEY_DEV_TYPE_BUTTON,
};

enum monkey_event_e
{
  MONKEY_EVENT_CLICK,
  MONKEY_EVENT_LONG_PRESS,
  MONKEY_EVENT_DRAG,
  MONKEY_EVENT_LAST
};

struct monkey_dev_state_s
{
  enum monkey_dev_type_e type;
  union
  {
    struct
    {
      int x;
      int y;
      int is_pressed;
    } touch;

    struct
    {
      uint32_t value;
    } button;
  } data;
};

struct monkey_event_config_s
{
  int weight;
  int duration_min;
  int duration_max;
};

struct monkey_config_s
{
  struct
  {
    int x_offset;
    int y_offset;
    int hor_res;
    int ver_res;
  } screen;

  struct
  {
    uint32_t min;
    uint32_t max;
  } period;

  uint8_t btn_bit;

  struct monkey_event_config_s event[MONKEY_EVENT_LAST];
};

struct monkey_s
{
  struct monkey_config_s config;
  enum monkey_mode_e mode;
  FAR struct monkey_dev_s *devs[MONKEY_DEV_MAX_NUM];
  int dev_num;
  FAR struct monkey_recorder_s *recorder;
  struct
  {
    struct monkey_dev_state_s state;
    uint32_t time_stamp;
  } playback_ctx;
};

#endif /* __APPS_SYSTEM_MONKEY_MONKEY_TYPE_H */
