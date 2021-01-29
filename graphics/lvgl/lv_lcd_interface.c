/****************************************************************************
 * graphics/lvgl/lv_lcd_interface.c
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
#include <nuttx/lcd/lcd_dev.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include "lv_lcd_interface.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LCD_DEVICEPATH      CONFIG_LV_LCD_INTERFACE_DEVICEPATH
#define LCD_BUFFER_SIZE     (CONFIG_LV_HOR_RES * \
                            CONFIG_LV_LCD_INTERFACE_BUFF_SIZE)

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

struct lcd_drv_s
{
  int fd;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lcd_flush
 ****************************************************************************/

static void lcd_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area_p,
                      lv_color_t *color_p)
{
  struct lcd_drv_s *lcd_drv = (struct lcd_drv_s *)disp_drv->user_data;

  struct lcddev_area_s lcd_area;

  lcd_area.row_start = area_p->y1;
  lcd_area.row_end = area_p->y2;
  lcd_area.col_start = area_p->x1;
  lcd_area.col_end = area_p->x2;
  lcd_area.data = (uint8_t *)color_p;

  ioctl(lcd_drv->fd, LCDDEVIO_PUTAREA, &lcd_area);

  /* Tell the flushing is ready */

  lv_disp_flush_ready(disp_drv);
}

static lv_disp_t *lcd_init(int fd)
{
  struct lcd_drv_s *lcd_drv =
    (struct lcd_drv_s *)lv_mem_alloc(sizeof(struct lcd_drv_s));

  if (lcd_drv == NULL)
    {
      LV_LOG_ERROR("lcd_drv malloc failed!");
      return NULL;
    }

  lcd_drv->fd = fd;

  static lv_disp_buf_t disp_buf;
  static lv_color_t buf[LCD_BUFFER_SIZE];
  lv_disp_buf_init(&disp_buf, buf, NULL, LCD_BUFFER_SIZE);

  lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.flush_cb = lcd_flush;
  disp_drv.buffer = &disp_buf;
#if ( LV_USE_USER_DATA != 0 )
  disp_drv.user_data = lcd_drv;
#else
#error LV_USE_USER_DATA must be enabled
#endif
  return lv_disp_drv_register(&disp_drv);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lv_lcd_interface_init
 ****************************************************************************/

lv_disp_t *lv_lcd_interface_init(void)
{
  LV_LOG_INFO("lcddev opening %s", LCD_DEVICEPATH);
  int fd = open(LCD_DEVICEPATH, 0);
  if (fd < 0)
    {
      int errcode = errno;
      LV_LOG_ERROR("lcddev open failed! errcode: %d", errcode);
      return NULL;
    }

  LV_LOG_INFO("lcddev %s open success", LCD_DEVICEPATH);

  return lcd_init(fd);
}
