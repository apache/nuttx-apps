/****************************************************************************
 * graphics/lvgl/lv_button_interface.c
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
#include "lv_button_interface.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BUTTON_DEVICEPATH           CONFIG_LV_BUTTON_INTERFACE_BUTTON_DEVICEPATH
#define BUTTON_0_MAP_X              CONFIG_LV_BUTTON_INTERFACE_BUTTON_0_MAP_X
#define BUTTON_0_MAP_Y              CONFIG_LV_BUTTON_INTERFACE_BUTTON_0_MAP_Y
#define BUTTON_1_MAP_X              CONFIG_LV_BUTTON_INTERFACE_BUTTON_1_MAP_X
#define BUTTON_1_MAP_Y              CONFIG_LV_BUTTON_INTERFACE_BUTTON_1_MAP_Y
#define BUTTON_2_MAP_X              CONFIG_LV_BUTTON_INTERFACE_BUTTON_2_MAP_X
#define BUTTON_2_MAP_Y              CONFIG_LV_BUTTON_INTERFACE_BUTTON_2_MAP_Y
#define BUTTON_3_MAP_X              CONFIG_LV_BUTTON_INTERFACE_BUTTON_3_MAP_X
#define BUTTON_3_MAP_Y              CONFIG_LV_BUTTON_INTERFACE_BUTTON_3_MAP_Y
#define BUTTON_4_MAP_X              CONFIG_LV_BUTTON_INTERFACE_BUTTON_4_MAP_X
#define BUTTON_4_MAP_Y              CONFIG_LV_BUTTON_INTERFACE_BUTTON_4_MAP_Y
#define BUTTON_5_MAP_X              CONFIG_LV_BUTTON_INTERFACE_BUTTON_5_MAP_X
#define BUTTON_5_MAP_Y              CONFIG_LV_BUTTON_INTERFACE_BUTTON_5_MAP_Y

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

struct button_drv_s
{
  int fd;
  uint8_t last_btn;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Assign buttons to points on the screen */

static const lv_point_t button_points_map[6] =
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

  int ret = read(fd, &buttonset, sizeof(btn_buttonset_t));
  if (ret < 0)
    {
      return -1;
    }

  for (int bit = 0; bit < buttonset_bits; bit++)
    {
      btn_buttonset_t mask = 1 << bit;

      if (buttonset & mask)
        {
          btn_act = bit;
          break;
        }
    }

  return btn_act;
}

/****************************************************************************
 * Name: lv_button_interface_read
 ****************************************************************************/

static bool button_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
  struct button_drv_s *button_drv = (struct button_drv_s *)drv->user_data;

  /* Get the pressed button's ID */

  int btn_act = button_get_pressed_id(button_drv->fd);

  if (btn_act >= 0)
    {
      data->state = LV_INDEV_STATE_PR;
      button_drv->last_btn = btn_act;
    }
  else
    {
      data->state = LV_INDEV_STATE_REL;
    }

  /* Save the last pressed button's ID */

  data->btn_id = button_drv->last_btn;

  /* Return `false` because we are not buffering and no more data to read */

  return false;
}

static lv_indev_t *button_init(int fd)
{
  struct button_drv_s *button_drv =
    (struct button_drv_s *)lv_mem_alloc(sizeof(struct button_drv_s));

  if (button_drv == NULL)
    {
      LV_LOG_ERROR("button_drv malloc failed");
      return NULL;
    }

  button_drv->fd = fd;
  button_drv->last_btn = 0;

  lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_BUTTON;
  indev_drv.read_cb = button_read;
#if ( LV_USE_USER_DATA != 0 )
  indev_drv.user_data = button_drv;
#else
#error LV_USE_USER_DATA must be enabled
#endif
  lv_indev_t *indev_button = lv_indev_drv_register(&indev_drv);
  lv_indev_set_button_points(indev_button, button_points_map);

  return indev_button;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lv_button_interface_init
 ****************************************************************************/

lv_indev_t *lv_button_interface_init(void)
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

  return button_init(fd);
}
