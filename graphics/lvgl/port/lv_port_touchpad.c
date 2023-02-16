/****************************************************************************
 * apps/graphics/lvgl/port/lv_port_touchpad.c
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
#include <nuttx/input/touchscreen.h>
#include "lv_port_touchpad.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

struct touchpad_obj_s
{
  int fd;
  lv_coord_t last_x;
  lv_coord_t last_y;
  lv_indev_state_t last_state;
  lv_indev_drv_t indev_drv;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: touchpad_read
 ****************************************************************************/

static void touchpad_read(FAR lv_indev_drv_t *drv, FAR lv_indev_data_t *data)
{
  FAR struct touchpad_obj_s *touchpad_obj = drv->user_data;
  struct touch_sample_s sample;

  /* Read one sample */

  int nbytes = read(touchpad_obj->fd, &sample,
                    sizeof(struct touch_sample_s));

  /* Handle unexpected return values */

  if (nbytes == sizeof(struct touch_sample_s))
    {
      uint8_t touch_flags = sample.point[0].flags;

      if (touch_flags & TOUCH_DOWN || touch_flags & TOUCH_MOVE)
        {
          const FAR lv_disp_drv_t *disp_drv = drv->disp->driver;
          lv_coord_t ver_max = disp_drv->ver_res - 1;
          lv_coord_t hor_max = disp_drv->hor_res - 1;

          touchpad_obj->last_x = LV_CLAMP(0, sample.point[0].x, hor_max);
          touchpad_obj->last_y = LV_CLAMP(0, sample.point[0].y, ver_max);
          touchpad_obj->last_state = LV_INDEV_STATE_PR;
        }
      else if (touch_flags & TOUCH_UP)
        {
          touchpad_obj->last_state = LV_INDEV_STATE_REL;
        }
    }

  /* Update touchpad data */

  data->point.x = touchpad_obj->last_x;
  data->point.y = touchpad_obj->last_y;
  data->state = touchpad_obj->last_state;
}

/****************************************************************************
 * Name: touchpad_init
 ****************************************************************************/

static FAR lv_indev_t *touchpad_init(int fd)
{
  FAR struct touchpad_obj_s *touchpad_obj;
  touchpad_obj = malloc(sizeof(struct touchpad_obj_s));

  if (touchpad_obj == NULL)
    {
      LV_LOG_ERROR("touchpad_obj_s malloc failed");
      return NULL;
    }

  touchpad_obj->fd = fd;
  touchpad_obj->last_x = 0;
  touchpad_obj->last_y = 0;
  touchpad_obj->last_state = LV_INDEV_STATE_REL;

  lv_indev_drv_init(&(touchpad_obj->indev_drv));
  touchpad_obj->indev_drv.type = LV_INDEV_TYPE_POINTER;
  touchpad_obj->indev_drv.read_cb = touchpad_read;
#if ( LV_USE_USER_DATA != 0 )
  touchpad_obj->indev_drv.user_data = touchpad_obj;
#else
#error LV_USE_USER_DATA must be enabled
#endif
  return lv_indev_drv_register(&(touchpad_obj->indev_drv));
}

/****************************************************************************
 * Name: touchpad_cursor_init
 ****************************************************************************/

static void touchpad_cursor_init(FAR lv_indev_t *indev, lv_coord_t size)
{
  FAR lv_obj_t *cursor;

  if (size <= 0)
    {
      return;
    }

  cursor = lv_obj_create(lv_layer_sys());
  lv_obj_remove_style_all(cursor);

  lv_obj_set_size(cursor, size, size);
  lv_obj_set_style_translate_x(cursor, -size / 2, 0);
  lv_obj_set_style_translate_y(cursor, -size / 2, 0);
  lv_obj_set_style_radius(cursor, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(cursor, LV_OPA_50, 0);
  lv_obj_set_style_bg_color(cursor, lv_color_black(), 0);
  lv_obj_set_style_border_width(cursor, 2, 0);
  lv_obj_set_style_border_color(cursor, lv_palette_main(LV_PALETTE_GREY), 0);
  lv_indev_set_cursor(indev, cursor);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lv_port_touchpad_init
 *
 * Description:
 *   Touchpad interface initialization.
 *
 * Input Parameters:
 *   dev_path - input device path, set to NULL to use the default path
 *
 * Returned Value:
 *   lv_indev object address on success; NULL on failure.
 *
 ****************************************************************************/

FAR lv_indev_t *lv_port_touchpad_init(FAR const char *dev_path)
{
  FAR const char *device_path = dev_path;
  FAR lv_indev_t *indev;
  int fd;

  if (device_path == NULL)
    {
      device_path = CONFIG_LV_PORT_TOUCHPAD_DEFAULT_DEVICEPATH;
    }

  LV_LOG_INFO("touchpad %s opening", device_path);
  fd = open(device_path, O_RDONLY | O_NONBLOCK);
  if (fd < 0)
    {
      LV_LOG_ERROR("touchpad %s open failed: %d", device_path, errno);
      return NULL;
    }

  LV_LOG_INFO("touchpad %s open success", device_path);

  indev = touchpad_init(fd);

  if (indev == NULL)
    {
      close(fd);
    }

  touchpad_cursor_init(indev, CONFIG_LV_PORT_TOUCHPAD_CURSOR_SIZE);

  return indev;
}
