/****************************************************************************
 * apps/graphics/lvgl/port/lv_port_encoder.c
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
#include <nuttx/input/mouse.h>
#include "lv_port_encoder.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

struct encoder_obj_s
{
  int fd;
  lv_indev_state_t last_state;
  lv_indev_drv_t indev_drv;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: encoder_read
 ****************************************************************************/

static void encoder_read(FAR lv_indev_drv_t *drv, FAR lv_indev_data_t *data)
{
  FAR struct encoder_obj_s *encoder_obj = drv->user_data;

  /* Read one sample */

  struct mouse_report_s sample;
  int16_t wheel = 0;

  int nbytes = read(encoder_obj->fd, &sample,
                    sizeof(struct mouse_report_s));

  /* Handle unexpected return values */

  if (nbytes == sizeof(struct mouse_report_s))
    {
      wheel = sample.wheel;
      encoder_obj->last_state = (sample.buttons & MOUSE_BUTTON_3) ?
        LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    }

  /* Update encoder data */

  data->enc_diff = wheel;
  data->state = encoder_obj->last_state;
}

/****************************************************************************
 * Name: encoder_init
 ****************************************************************************/

static FAR lv_indev_t *encoder_init(int fd)
{
  FAR struct encoder_obj_s *encoder_obj;
  encoder_obj = malloc(sizeof(struct encoder_obj_s));

  if (encoder_obj == NULL)
    {
      LV_LOG_ERROR("encoder_obj_s malloc failed");
      return NULL;
    }

  encoder_obj->fd = fd;
  encoder_obj->last_state = LV_INDEV_STATE_RELEASED;

  lv_indev_drv_init(&(encoder_obj->indev_drv));
  encoder_obj->indev_drv.type = LV_INDEV_TYPE_ENCODER;
  encoder_obj->indev_drv.read_cb = encoder_read;
#if ( LV_USE_USER_DATA != 0 )
  encoder_obj->indev_drv.user_data = encoder_obj;
#else
#error LV_USE_USER_DATA must be enabled
#endif
  return lv_indev_drv_register(&(encoder_obj->indev_drv));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lv_port_encoder_init
 *
 * Description:
 *   Encoder interface initialization.
 *
 * Input Parameters:
 *   dev_path - input device path, set to NULL to use the default path.
 *
 * Returned Value:
 *   lv_indev object address on success; NULL on failure.
 *
 ****************************************************************************/

FAR lv_indev_t *lv_port_encoder_init(FAR const char *dev_path)
{
  FAR const char *device_path = dev_path;
  FAR lv_indev_t *indev;
  int fd;

  if (device_path == NULL)
    {
      device_path = CONFIG_LV_PORT_ENCODER_DEFAULT_DEVICEPATH;
    }

  LV_LOG_INFO("encoder %s opening", device_path);
  fd = open(device_path, O_RDONLY | O_NONBLOCK);
  if (fd < 0)
    {
      LV_LOG_ERROR("encoder %s open failed: %d", device_path, errno);
      return NULL;
    }

  LV_LOG_INFO("encoder %s open success", device_path);

  indev = encoder_init(fd);

  if (indev == NULL)
    {
      close(fd);
    }

  return indev;
}
