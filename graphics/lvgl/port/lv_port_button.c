/****************************************************************************
 * apps/graphics/lvgl/port/lv_port_button.c
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
#include <nuttx/input/buttons.h>
#include "lv_port_button.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BUTTON_0_MAP_X              CONFIG_LV_PORT_BUTTON_BUTTON_0_MAP_X
#define BUTTON_0_MAP_Y              CONFIG_LV_PORT_BUTTON_BUTTON_0_MAP_Y
#define BUTTON_1_MAP_X              CONFIG_LV_PORT_BUTTON_BUTTON_1_MAP_X
#define BUTTON_1_MAP_Y              CONFIG_LV_PORT_BUTTON_BUTTON_1_MAP_Y
#define BUTTON_2_MAP_X              CONFIG_LV_PORT_BUTTON_BUTTON_2_MAP_X
#define BUTTON_2_MAP_Y              CONFIG_LV_PORT_BUTTON_BUTTON_2_MAP_Y
#define BUTTON_3_MAP_X              CONFIG_LV_PORT_BUTTON_BUTTON_3_MAP_X
#define BUTTON_3_MAP_Y              CONFIG_LV_PORT_BUTTON_BUTTON_3_MAP_Y
#define BUTTON_4_MAP_X              CONFIG_LV_PORT_BUTTON_BUTTON_4_MAP_X
#define BUTTON_4_MAP_Y              CONFIG_LV_PORT_BUTTON_BUTTON_4_MAP_Y
#define BUTTON_5_MAP_X              CONFIG_LV_PORT_BUTTON_BUTTON_5_MAP_X
#define BUTTON_5_MAP_Y              CONFIG_LV_PORT_BUTTON_BUTTON_5_MAP_Y

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

struct button_obj_s
{
  int fd;
  uint8_t last_btn;
  lv_indev_drv_t indev_drv;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Assign buttons to points on the screen */

static const lv_point_t g_button_points_map[6] =
{
  {BUTTON_0_MAP_X, BUTTON_0_MAP_Y},
  {BUTTON_1_MAP_X, BUTTON_1_MAP_Y},
  {BUTTON_2_MAP_X, BUTTON_2_MAP_Y},
  {BUTTON_3_MAP_X, BUTTON_3_MAP_Y},
  {BUTTON_4_MAP_X, BUTTON_4_MAP_Y},
  {BUTTON_5_MAP_X, BUTTON_5_MAP_Y}
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: button_get_pressed_id
 ****************************************************************************/

static int button_get_pressed_id(int fd)
{
  int btn_act = -1;
  btn_buttonset_t buttonset;
  const int buttonset_bits = sizeof(btn_buttonset_t) * 8;
  int bit;

  int ret = read(fd, &buttonset, sizeof(btn_buttonset_t));
  if (ret < 0)
    {
      return -1;
    }

  for (bit = 0; bit < buttonset_bits; bit++)
    {
      btn_buttonset_t mask = (btn_buttonset_t)(1 << bit);

      if ((buttonset & mask) != 0)
        {
          btn_act = bit;
          break;
        }
    }

  return btn_act;
}

/****************************************************************************
 * Name: button_read
 ****************************************************************************/

static void button_read(FAR lv_indev_drv_t *drv, FAR lv_indev_data_t *data)
{
  FAR struct button_obj_s *button_obj = drv->user_data;

  /* Get the pressed button's ID */

  int btn_act = button_get_pressed_id(button_obj->fd);

  if (btn_act >= 0)
    {
      data->state = LV_INDEV_STATE_PR;
      button_obj->last_btn = btn_act;
    }
  else
    {
      data->state = LV_INDEV_STATE_REL;
    }

  /* Save the last pressed button's ID */

  data->btn_id = button_obj->last_btn;
}

/****************************************************************************
 * Name: button_init
 ****************************************************************************/

static FAR lv_indev_t *button_init(int fd)
{
  FAR struct button_obj_s *button_obj;
  FAR lv_indev_t *indev_button;

  button_obj = malloc(sizeof(struct button_obj_s));

  if (button_obj == NULL)
    {
      LV_LOG_ERROR("button_obj_s malloc failed");
      return NULL;
    }

  button_obj->fd = fd;
  button_obj->last_btn = 0;

  lv_indev_drv_init(&(button_obj->indev_drv));
  button_obj->indev_drv.type = LV_INDEV_TYPE_BUTTON;
  button_obj->indev_drv.read_cb = button_read;
#if ( LV_USE_USER_DATA != 0 )
  button_obj->indev_drv.user_data = button_obj;
#else
#error LV_USE_USER_DATA must be enabled
#endif
  indev_button = lv_indev_drv_register(&(button_obj->indev_drv));
  lv_indev_set_button_points(indev_button, g_button_points_map);

  return indev_button;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lv_port_button_init
 *
 * Description:
 *   Button interface initialization.
 *
 * Input Parameters:
 *   dev_path - input device path, set to NULL to use the default path
 *
 * Returned Value:
 *   lv_indev object address on success; NULL on failure.
 *
 ****************************************************************************/

FAR lv_indev_t *lv_port_button_init(FAR const char *dev_path)
{
  FAR const char *device_path = dev_path;
  FAR lv_indev_t *indev;
  int fd;
  btn_buttonset_t supported;
  int ret;

  if (device_path == NULL)
    {
      device_path = CONFIG_LV_PORT_BUTTON_DEFAULT_DEVICEPATH;
    }

  LV_LOG_INFO("button %s opening", device_path);
  fd = open(device_path, O_RDONLY | O_NONBLOCK);
  if (fd < 0)
    {
      LV_LOG_ERROR("button %s open failed: %d", device_path, errno);
      return NULL;
    }

  /* Get the set of BUTTONs supported */

  ret = ioctl(fd, BTNIOC_SUPPORTED,
              (unsigned long)((uintptr_t)&supported));
  if (ret < 0)
    {
      LV_LOG_ERROR("button ioctl(BTNIOC_SUPPORTED) failed: %d", errno);
      return NULL;
    }

  LV_LOG_INFO("button supported BUTTONs 0x%08x", (unsigned int)supported);

  indev = button_init(fd);

  if (indev == NULL)
    {
      close(fd);
    }

  return indev;
}
