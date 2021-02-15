/****************************************************************************
 * apps/examples/lvgldemo/lcddev.c
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

#include "lcddev.h"

#include <nuttx/config.h>
#include <nuttx/lcd/lcd_dev.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef LCDDEV_PATH
#  define LCDDEV_PATH  "/dev/lcd0"
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct lcd_state_s
{
  int fd;
  struct fb_videoinfo_s vinfo;
  struct lcd_planeinfo_s pinfo;
  bool rotated;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct lcd_state_s state;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lcddev_flush
 *
 * Description:
 *   Flush a buffer to the marked area.
 *
 * Input Parameters:
 *   disp_drv  - LVGL driver interface
 *   lv_area_t - Area of the screen to be flushed
 *   color_p   - A n array of colors
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void lcddev_flush(struct _disp_drv_t *disp_drv, const lv_area_t *area,
                         lv_color_t *color_p)
{
  int ret;
  struct lcddev_area_s lcd_area;

  lcd_area.row_start = area->y1;
  lcd_area.row_end = area->y2;
  lcd_area.col_start = area->x1;
  lcd_area.col_end = area->x2;

  lcd_area.data = (uint8_t *)color_p;

  ret = ioctl(state.fd, LCDDEVIO_PUTAREA,
              (unsigned long)((uintptr_t)&lcd_area));

  if (ret < 0)
    {
      int errcode = errno;

      gerr("ERROR: ioctl(LCDDEVIO_PUTAREA) failed: %d\n",
           errcode);
      close(state.fd);
      return;
    }

  /* Tell the flushing is ready */

  lv_disp_flush_ready(disp_drv);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lcddev_init
 *
 * Description:
 *   Initialize LCD device.
 *
 * Input Parameters:
 *   lv_drvr -- LVGL driver interface
 *
 * Returned Value:
 *   EXIT_SUCCESS on success; EXIT_FAILURE on failure.
 *
 ****************************************************************************/

int lcddev_init(lv_disp_drv_t *lv_drvr)
{
  FAR const char *lcddev = LCDDEV_PATH;
  int ret;

  /* Open the framebuffer driver */

  state.fd = open(lcddev, 0);
  if (state.fd < 0)
    {
      int errcode = errno;
      gerr("ERROR: Failed to open %s: %d\n", state.fd, errcode);
      return EXIT_FAILURE;
    }

  /* Get the characteristics of the framebuffer */

  ret = ioctl(state.fd, LCDDEVIO_GETVIDEOINFO,
              (unsigned long)((uintptr_t)&state.vinfo));
  if (ret < 0)
    {
      int errcode = errno;

      gerr("ERROR: ioctl(LCDDEVIO_GETVIDEOINFO) failed: %d\n", errcode);
      close(state.fd);
      state.fd = -1;
      return EXIT_FAILURE;
    }

  ginfo("VideoInfo:\n\tfmt: %u\n\txres: %u\n\tyres: %u\n\tnplanes: %u\n",
        state.vinfo.fmt, state.vinfo.xres, state.vinfo.yres,
        state.vinfo.nplanes);

  ret = ioctl(state.fd, LCDDEVIO_GETPLANEINFO,
              (unsigned long)((uintptr_t)&state.pinfo));
  if (ret < 0)
    {
      int errcode = errno;
      gerr("ERROR: ioctl(LCDDEVIO_GETPLANEINFO) failed: %d\n", errcode);
      close(state.fd);
      state.fd = -1;
      return EXIT_FAILURE;
    }

  ginfo("PlaneInfo (plane 0):\n\tbpp: %u\n", state.pinfo.bpp);

  if (state.pinfo.bpp != CONFIG_LV_COLOR_DEPTH)
    {
      /* For the LCD driver we do not have a great way to translate this
       * so fail to initialize.
       */

      gerr(
        "ERROR: Display bpp (%u) did not match CONFIG_LV_COLOR_DEPTH (%u)\n",
        state.pinfo.bpp,
        CONFIG_LV_COLOR_DEPTH);
    }

  lv_drvr->hor_res = state.vinfo.xres;
  lv_drvr->ver_res = state.vinfo.yres;
  lv_drvr->flush_cb = lcddev_flush;

  return EXIT_SUCCESS;
}
