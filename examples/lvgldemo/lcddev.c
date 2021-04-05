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

#include <nuttx/compiler.h>
#include <nuttx/config.h>
#include <nuttx/lcd/lcd_dev.h>

#ifdef CONFIG_EXAMPLES_LVGLDEMO_ASYNC_FLUSH
#include <pthread.h>
#include <semaphore.h>
#endif

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

#ifdef CONFIG_EXAMPLES_LVGLDEMO_ASYNC_FLUSH
static pthread_t lcd_write_thread;
static sem_t flush_sem;
static sem_t wait_sem;
static struct lcddev_area_s lcd_area;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lcddev_wait
 *
 * Description:
 *   Wait for the flushing operation conclusion to notify LVGL.
 *
 * Input Parameters:
 *   disp_drv - LVGL driver interface
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_LVGLDEMO_ASYNC_FLUSH

static void lcddev_wait(lv_disp_drv_t *disp_drv)
{
  sem_wait(&wait_sem);

  /* Tell the flushing is ready */

  lv_disp_flush_ready(disp_drv);
}

#endif

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

#ifdef CONFIG_EXAMPLES_LVGLDEMO_ASYNC_FLUSH

static void lcddev_async_flush(lv_disp_drv_t *disp_drv,
                               const lv_area_t *area,
                               lv_color_t *color_p)
{
  UNUSED(disp_drv);

  lcd_area.row_start = area->y1;
  lcd_area.row_end = area->y2;
  lcd_area.col_start = area->x1;
  lcd_area.col_end = area->x2;

  lcd_area.data = (uint8_t *)color_p;

  sem_post(&flush_sem);
}

#else

static void lcddev_sync_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area,
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

      gerr("ioctl(LCDDEVIO_PUTAREA) failed: %d\n", errcode);
      close(state.fd);
      return;
    }

  /* Tell the flushing is ready */

  lv_disp_flush_ready(disp_drv);
}

#endif

/****************************************************************************
 * Name: lcddev_write
 *
 * Description:
 *   Write the buffer to LCD interface.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_LVGLDEMO_ASYNC_FLUSH

static pthread_addr_t lcddev_write(pthread_addr_t addr)
{
  int ret = OK;
  int errcode;

  while (ret == OK)
    {
      sem_wait(&flush_sem);
      ret = ioctl(state.fd, LCDDEVIO_PUTAREA, (unsigned long)&lcd_area);
      if (ret < 0)
        {
          errcode = errno;
        }

      sem_post(&wait_sem);
    }

  if (ret != OK)
    {
      gerr("ioctl(LCDDEVIO_PUTAREA) failed: %d\n", errcode);
      close(state.fd);
      state.fd = -1;
    }

  return NULL;
}

#endif

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
      gerr("Failed to open %s: %d\n", state.fd, errcode);
      return EXIT_FAILURE;
    }

  /* Get the characteristics of the framebuffer */

  ret = ioctl(state.fd, LCDDEVIO_GETVIDEOINFO,
              (unsigned long)((uintptr_t)&state.vinfo));
  if (ret < 0)
    {
      int errcode = errno;

      gerr("ioctl(LCDDEVIO_GETVIDEOINFO) failed: %d\n", errcode);
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
      gerr("ioctl(LCDDEVIO_GETPLANEINFO) failed: %d\n", errcode);
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

      gerr("Display bpp (%u) did not match CONFIG_LV_COLOR_DEPTH (%u)\n",
           state.pinfo.bpp, CONFIG_LV_COLOR_DEPTH);
    }

  lv_drvr->hor_res = state.vinfo.xres;
  lv_drvr->ver_res = state.vinfo.yres;
#ifndef CONFIG_EXAMPLES_LVGLDEMO_ASYNC_FLUSH
  lv_drvr->flush_cb = lcddev_sync_flush;
#else
  lv_drvr->flush_cb = lcddev_async_flush;
  lv_drvr->wait_cb = lcddev_wait;

  /* Initialize the mutexes for buffer flushing synchronization */

  sem_init(&flush_sem, 0, 0);
  sem_init(&wait_sem, 0, 0);

  /* Initialize the buffer flushing thread */

  pthread_create(&lcd_write_thread, NULL, lcddev_write, NULL);
#endif

  return EXIT_SUCCESS;
}
