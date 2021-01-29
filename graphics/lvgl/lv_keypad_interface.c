/****************************************************************************
 * graphics/lvgl/lv_keypad_interface.c
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

#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>
#include <nuttx/input/buttons.h>
#include "lv_keypad_interface.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BUTTON_DEVICEPATH           CONFIG_LV_KEYPAD_INTERFACE_BUTTON_DEVICEPATH

#define LV_KEY_UP_MAP_BIT           CONFIG_LV_KEYPAD_INTERFACE_KEY_UP_MAP_BIT
#define LV_KEY_DOWN_MAP_BIT         CONFIG_LV_KEYPAD_INTERFACE_KEY_DOWN_MAP_BIT
#define LV_KEY_RIGHT_MAP_BIT        CONFIG_LV_KEYPAD_INTERFACE_KEY_RIGHT_MAP_BIT
#define LV_KEY_LEFT_MAP_BIT         CONFIG_LV_KEYPAD_INTERFACE_KEY_LEFT_MAP_BIT

#define LV_KEY_ESC_MAP_BIT          CONFIG_LV_KEYPAD_INTERFACE_KEY_ESC_MAP_BIT
#define LV_KEY_DEL_MAP_BIT          CONFIG_LV_KEYPAD_INTERFACE_KEY_DEL_MAP_BIT
#define LV_KEY_BACKSPACE_MAP_BIT    CONFIG_LV_KEYPAD_INTERFACE_KEY_BACKSPACE_MAP_BIT
#define LV_KEY_ENTER_MAP_BIT        CONFIG_LV_KEYPAD_INTERFACE_KEY_ENTER_MAP_BIT

#define LV_KEY_NEXT_MAP_BIT         CONFIG_LV_KEYPAD_INTERFACE_KEY_NEXT_MAP_BIT
#define LV_KEY_PREV_MAP_BIT         CONFIG_LV_KEYPAD_INTERFACE_KEY_PREV_MAP_BIT
#define LV_KEY_HOME_MAP_BIT         CONFIG_LV_KEYPAD_INTERFACE_KEY_HOME_MAP_BIT
#define LV_KEY_END_MAP_BIT          CONFIG_LV_KEYPAD_INTERFACE_KEY_END_MAP_BIT

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

struct button_map_s
{
  const lv_key_t key;
  int bit;
};

struct button_drv_s
{
  int fd;
  uint32_t last_key;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct button_map_s button_map[] =
{
  {.key = LV_KEY_UP,        .bit = LV_KEY_UP_MAP_BIT},
  {.key = LV_KEY_DOWN,      .bit = LV_KEY_DOWN_MAP_BIT},
  {.key = LV_KEY_RIGHT,     .bit = LV_KEY_RIGHT_MAP_BIT},
  {.key = LV_KEY_LEFT,      .bit = LV_KEY_LEFT_MAP_BIT},
  {.key = LV_KEY_ESC,       .bit = LV_KEY_ESC_MAP_BIT},
  {.key = LV_KEY_DEL,       .bit = LV_KEY_DEL_MAP_BIT},
  {.key = LV_KEY_BACKSPACE, .bit = LV_KEY_BACKSPACE_MAP_BIT},
  {.key = LV_KEY_ENTER,     .bit = LV_KEY_ENTER_MAP_BIT},
  {.key = LV_KEY_NEXT,      .bit = LV_KEY_NEXT_MAP_BIT},
  {.key = LV_KEY_PREV,      .bit = LV_KEY_PREV_MAP_BIT},
  {.key = LV_KEY_HOME,      .bit = LV_KEY_HOME_MAP_BIT},
  {.key = LV_KEY_END,       .bit = LV_KEY_END_MAP_BIT}
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: keypad_get_key
 ****************************************************************************/

static uint32_t keypad_get_key(int fd)
{
  uint32_t act_key = 0;
  btn_buttonset_t buttonset;
  const int buttonset_bits = sizeof(btn_buttonset_t) * 8;

  int ret = read(fd, &buttonset, sizeof(btn_buttonset_t));
  if (ret < 0)
    {
      return 0;
    }

  for (int i = 0; i < sizeof(button_map) / sizeof(struct button_map_s); i++)
    {
      int bit = button_map[i].bit;

      if (bit >= 0 && bit < buttonset_bits)
        {
          btn_buttonset_t mask = 1 << bit;
          if (buttonset & mask)
            {
              act_key = button_map[i].key;
              break;
            }
        }
    }

  return act_key;
}

/****************************************************************************
 * Name: keypad_read
 ****************************************************************************/

static bool keypad_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
  struct button_drv_s *button_drv = (struct button_drv_s *)drv->user_data;

  /* Get whether the a key is pressed and save the pressed key */

  uint32_t act_key = keypad_get_key(button_drv->fd);
  if (act_key != 0)
    {
      data->state = LV_INDEV_STATE_PR;
      button_drv->last_key = act_key;
    }
  else
    {
      data->state = LV_INDEV_STATE_REL;
    }

  data->key = button_drv->last_key;

  /* Return `false` because we are not buffering and no more data to read */

  return false;
}

static lv_indev_t *keypad_init(int fd)
{
  struct button_drv_s *button_drv =
    (struct button_drv_s *)lv_mem_alloc(sizeof(struct button_drv_s));

  if (button_drv == NULL)
    {
      LV_LOG_ERROR("button_drv malloc failed");
      return NULL;
    }

  button_drv->fd = fd;
  button_drv->last_key = 0;

  /* Register a keypad input device */

  lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_KEYPAD;
  indev_drv.read_cb = keypad_read;
#if ( LV_USE_USER_DATA != 0 )
  indev_drv.user_data = button_drv;
#else
#error LV_USE_USER_DATA must be enabled
#endif
  return lv_indev_drv_register(&indev_drv);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lv_keypad_interface_init
 ****************************************************************************/

lv_indev_t *lv_keypad_interface_init(void)
{
  int ret;
  int fd;

  LV_LOG_INFO("button opening %s", BUTTON_DEVICEPATH);
  fd = open(BUTTON_DEVICEPATH, O_RDONLY | O_NONBLOCK);
  if (fd < 0)
    {
      int errcode = errno;
      LV_LOG_ERROR("button failed to open %s ! errcode: %d",
                   BUTTON_DEVICEPATH, errcode);
      return NULL;
    }

  /* Get the set of BUTTONs supported */

  btn_buttonset_t supported;

  ret = ioctl(fd, BTNIOC_SUPPORTED,
              (unsigned long)((uintptr_t)&supported));
  if (ret < 0)
    {
      int errcode = errno;
      LV_LOG_ERROR("button ioctl(BTNIOC_SUPPORTED) failed! errcode: %d",
                   errcode);
      return NULL;
    }

  LV_LOG_INFO("button supported BUTTONs 0x%08x", (unsigned int)supported);

  return keypad_init(fd);
}
