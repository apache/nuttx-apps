/****************************************************************************
 * apps/graphics/lvgl/port/lv_port_keypad.c
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
#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>
#include <nuttx/input/buttons.h>
#include "lv_port_keypad.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LV_KEY_UP_MAP_BIT           CONFIG_LV_PORT_KEYPAD_KEY_UP_MAP_BIT
#define LV_KEY_DOWN_MAP_BIT         CONFIG_LV_PORT_KEYPAD_KEY_DOWN_MAP_BIT
#define LV_KEY_RIGHT_MAP_BIT        CONFIG_LV_PORT_KEYPAD_KEY_RIGHT_MAP_BIT
#define LV_KEY_LEFT_MAP_BIT         CONFIG_LV_PORT_KEYPAD_KEY_LEFT_MAP_BIT

#define LV_KEY_ESC_MAP_BIT          CONFIG_LV_PORT_KEYPAD_KEY_ESC_MAP_BIT
#define LV_KEY_DEL_MAP_BIT          CONFIG_LV_PORT_KEYPAD_KEY_DEL_MAP_BIT
#define LV_KEY_BACKSPACE_MAP_BIT    CONFIG_LV_PORT_KEYPAD_KEY_BACKSPACE_MAP_BIT
#define LV_KEY_ENTER_MAP_BIT        CONFIG_LV_PORT_KEYPAD_KEY_ENTER_MAP_BIT

#define LV_KEY_NEXT_MAP_BIT         CONFIG_LV_PORT_KEYPAD_KEY_NEXT_MAP_BIT
#define LV_KEY_PREV_MAP_BIT         CONFIG_LV_PORT_KEYPAD_KEY_PREV_MAP_BIT
#define LV_KEY_HOME_MAP_BIT         CONFIG_LV_PORT_KEYPAD_KEY_HOME_MAP_BIT
#define LV_KEY_END_MAP_BIT          CONFIG_LV_PORT_KEYPAD_KEY_END_MAP_BIT

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

struct keypad_map_s
{
  const lv_key_t key;
  int bit;
};

struct keypad_obj_s
{
  int fd;
  uint32_t last_key;
  lv_indev_drv_t indev_drv;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct keypad_map_s g_keypad_map[] =
{
  {LV_KEY_UP,        LV_KEY_UP_MAP_BIT},
  {LV_KEY_DOWN,      LV_KEY_DOWN_MAP_BIT},
  {LV_KEY_RIGHT,     LV_KEY_RIGHT_MAP_BIT},
  {LV_KEY_LEFT,      LV_KEY_LEFT_MAP_BIT},
  {LV_KEY_ESC,       LV_KEY_ESC_MAP_BIT},
  {LV_KEY_DEL,       LV_KEY_DEL_MAP_BIT},
  {LV_KEY_BACKSPACE, LV_KEY_BACKSPACE_MAP_BIT},
  {LV_KEY_ENTER,     LV_KEY_ENTER_MAP_BIT},
  {LV_KEY_NEXT,      LV_KEY_NEXT_MAP_BIT},
  {LV_KEY_PREV,      LV_KEY_PREV_MAP_BIT},
  {LV_KEY_HOME,      LV_KEY_HOME_MAP_BIT},
  {LV_KEY_END,       LV_KEY_END_MAP_BIT}
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
  int i;

  int ret = read(fd, &buttonset, sizeof(btn_buttonset_t));
  if (ret < 0)
    {
      return 0;
    }

  for (i = 0; i < nitems(g_keypad_map); i++)
    {
      int bit = g_keypad_map[i].bit;

      if (bit >= 0 && bit < buttonset_bits)
        {
          btn_buttonset_t mask = (btn_buttonset_t)(1 << bit);
          if ((buttonset & mask) != 0)
            {
              act_key = g_keypad_map[i].key;
              break;
            }
        }
    }

  return act_key;
}

/****************************************************************************
 * Name: keypad_read
 ****************************************************************************/

static void keypad_read(FAR lv_indev_drv_t *drv, FAR lv_indev_data_t *data)
{
  FAR struct keypad_obj_s *keypad_obj = drv->user_data;

  /* Get whether the a key is pressed and save the pressed key */

  uint32_t act_key = keypad_get_key(keypad_obj->fd);

  if (act_key != 0)
    {
      data->state = LV_INDEV_STATE_PR;
      keypad_obj->last_key = act_key;
    }
  else
    {
      data->state = LV_INDEV_STATE_REL;
    }

  data->key = keypad_obj->last_key;
}

/****************************************************************************
 * Name: keypad_init
 ****************************************************************************/

static FAR lv_indev_t *keypad_init(int fd)
{
  FAR struct keypad_obj_s *keypad_obj;
  keypad_obj = malloc(sizeof(struct keypad_obj_s));

  if (keypad_obj == NULL)
    {
      LV_LOG_ERROR("keypad_obj_s malloc failed");
      return NULL;
    }

  keypad_obj->fd = fd;
  keypad_obj->last_key = 0;

  /* Register a keypad input device */

  lv_indev_drv_init(&(keypad_obj->indev_drv));
  keypad_obj->indev_drv.type = LV_INDEV_TYPE_KEYPAD;
  keypad_obj->indev_drv.read_cb = keypad_read;
#if ( LV_USE_USER_DATA != 0 )
  keypad_obj->indev_drv.user_data = keypad_obj;
#else
#error LV_USE_USER_DATA must be enabled
#endif
  return lv_indev_drv_register(&(keypad_obj->indev_drv));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lv_port_keypad_init
 *
 * Description:
 *   Keypad interface initialization.
 *
 * Input Parameters:
 *   dev_path - input device path, set to NULL to use the default path
 *
 * Returned Value:
 *   lv_indev object address on success; NULL on failure.
 *
 ****************************************************************************/

FAR lv_indev_t *lv_port_keypad_init(FAR const char *dev_path)
{
  FAR const char *device_path = dev_path;
  FAR lv_indev_t *indev;
  int fd;
  int ret;
  btn_buttonset_t supported;

  if (device_path == NULL)
    {
      device_path = CONFIG_LV_PORT_KEYPAD_DEFAULT_DEVICEPATH;
    }

  LV_LOG_INFO("keypad %s opening", device_path);
  fd = open(device_path, O_RDONLY | O_NONBLOCK);
  if (fd < 0)
    {
      LV_LOG_ERROR("keypad %s open failed: %d", device_path, errno);
      return NULL;
    }

  /* Get the set of BUTTONs supported */

  ret = ioctl(fd, BTNIOC_SUPPORTED,
              (unsigned long)((uintptr_t)&supported));
  if (ret < 0)
    {
      LV_LOG_ERROR("button ioctl(BTNIOC_SUPPORTED) failed: %d",
                   errno);
      return NULL;
    }

  LV_LOG_INFO("button supported BUTTONs 0x%08x", (unsigned int)supported);

  indev = keypad_init(fd);

  if (indev == NULL)
    {
      close(fd);
    }

  return indev;
}
