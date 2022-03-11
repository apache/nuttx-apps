/****************************************************************************
 * apps/examples/lvgldemo/fbdev.c
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

#include "fbdev.h"

#include <nuttx/compiler.h>
#include <nuttx/config.h>

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

#include <nuttx/video/fb.h>
#include <nuttx/video/rgbcolors.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef FBDEV_PATH
#  define FBDEV_PATH  "/dev/fb0"
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct fb_state_s
{
  int fd;
  struct fb_videoinfo_s vinfo;
  struct fb_planeinfo_s pinfo;
  FAR void *fbmem;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

struct fb_state_s state;

#ifdef CONFIG_EXAMPLES_LVGLDEMO_ASYNC_FLUSH
static pthread_t fb_write_thread;
static sem_t flush_sem;
static sem_t wait_sem;

static lv_area_t lv_area;
static lv_color_t *lv_color_p;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fbdev_wait
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

static void fbdev_wait(lv_disp_drv_t *disp_drv)
{
  sem_wait(&wait_sem);

  /* Tell the flushing is ready */

  lv_disp_flush_ready(disp_drv);
}

#endif

/****************************************************************************
 * Name: fbdev_flush_internal
 *
 * Description:
 *   Write the buffer to Framebuffer interface.
 *
 * Input Parameters:
 *   area      - Area of the screen to be flushed
 *   color_p   - A n array of colors
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static int fbdev_flush_internal(const lv_area_t *area, lv_color_t *color_p)
{
#ifdef CONFIG_FB_UPDATE
  struct fb_area_s fb_area;
#endif
  int32_t x1 = area->x1;
  int32_t y1 = area->y1;
  int32_t x2 = area->x2;
  int32_t y2 = area->y2;
  int32_t act_x1;
  int32_t act_y1;
  int32_t act_x2;
  int32_t act_y2;
  long int location = 0;

  if (state.fbmem == NULL)
    {
      return ERROR;
    }

  /* Return if the area is out the screen */

  if (x2 < 0)
    {
      return ERROR;
    }

  if (y2 < 0)
    {
      return ERROR;
    }

  if (x1 > state.vinfo.xres - 1)
    {
      return ERROR;
    }

  if (y1 > state.vinfo.yres - 1)
    {
      return ERROR;
    }

  /* Truncate the area to the screen */

  act_x1 = x1 < 0 ? 0 : x1;
  act_y1 = y1 < 0 ? 0 : y1;
  act_x2 = x2 > state.vinfo.xres - 1 ? state.vinfo.xres - 1 : x2;
  act_y2 = y2 > state.vinfo.yres - 1 ? state.vinfo.yres - 1 : y2;

  if (state.pinfo.bpp == 8)
    {
      uint8_t *fbp8 = (uint8_t *)state.fbmem;
      uint32_t x;
      uint32_t y;

      for (y = act_y1; y <= act_y2; y++)
        {
          for (x = act_x1; x <= act_x2; x++)
            {
              location = x + (y * state.vinfo.xres);
              fbp8[location] = color_p->full;
              color_p++;
            }

          color_p += x2 - act_x2;
        }
    }

  if (state.pinfo.bpp == 16)
    {
      uint16_t *fbp16 = (uint16_t *)state.fbmem;
      uint32_t x;
      uint32_t y;

      for (y = act_y1; y <= act_y2; y++)
        {
          for (x = act_x1; x <= act_x2; x++)
            {
              location = x + (y * state.vinfo.xres);
              fbp16[location] = color_p->full;
              color_p++;
            }

          color_p += x2 - act_x2;
        }
    }

  if (state.pinfo.bpp == 24 || state.pinfo.bpp == 32)
    {
      uint32_t *fbp32 = (uint32_t *)state.fbmem;
      uint32_t x;
      uint32_t y;

      for (y = act_y1; y <= act_y2; y++)
        {
          for (x = act_x1; x <= act_x2; x++)
            {
              location = x + (y * state.vinfo.xres);
              fbp32[location] = color_p->full;
              color_p++;
            }

          color_p += x2 - act_x2;
        }
    }

#ifdef CONFIG_FB_UPDATE
  fb_area.x = act_x1;
  fb_area.y = act_y1;
  fb_area.w = act_x2 - act_x1 + 1;
  fb_area.h = act_y2 - act_y1 + 1;
  ioctl(state.fd, FBIO_UPDATE, (unsigned long)((uintptr_t)&fb_area));
#endif

  return OK;
}

/****************************************************************************
 * Name: fbdev_async_flush
 *
 * Description:
 *   Flush a buffer to the marked area asynchronously.
 *
 * Input Parameters:
 *   disp_drv  - LVGL driver interface
 *   area      - Area of the screen to be flushed
 *   color_p   - A n array of colors
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_LVGLDEMO_ASYNC_FLUSH

static void fbdev_async_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area,
                              lv_color_t *color_p)
{
  UNUSED(disp_drv);

  lv_area.y1 = area->y1;
  lv_area.y2 = area->y2;
  lv_area.x1 = area->x1;
  lv_area.x2 = area->x2;

  lv_color_p = color_p;

  sem_post(&flush_sem);
}

#endif

/****************************************************************************
 * Name: fbdev_sync_flush
 *
 * Description:
 *   Flush a buffer to the marked area synchronously.
 *
 * Input Parameters:
 *   disp_drv  - LVGL driver interface
 *   area      - Area of the screen to be flushed
 *   color_p   - A n array of colors
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_LVGLDEMO_ASYNC_FLUSH

static void fbdev_sync_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area,
                             lv_color_t *color_p)
{
  fbdev_flush_internal(area, color_p);

  /* Tell the flushing is ready */

  lv_disp_flush_ready(disp_drv);
}

#endif

/****************************************************************************
 * Name: fbdev_write
 *
 * Description:
 *   Thread for writing the buffer to Framebuffer interface.
 *
 * Input Parameters:
 *   arg       - Context data from the parent thread
 *
 * Returned Value:
 *   Context data to the parent thread
 *
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_LVGLDEMO_ASYNC_FLUSH

static pthread_addr_t fbdev_write(pthread_addr_t arg)
{
  int ret = OK;

  UNUSED(arg);

  while (ret == OK)
    {
      sem_wait(&flush_sem);

      ret = fbdev_flush_internal(&lv_area, lv_color_p);

      sem_post(&wait_sem);
    }

  gerr("Failed to write buffer contents to display device\n");

  return NULL;
}

#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fbdev_init
 *
 * Description:
 *
 * Input Parameters:
 *   lv_drvr -- LVGL driver interface
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

int fbdev_init(lv_disp_drv_t *lv_drvr)
{
  FAR const char *fbdev = "/dev/fb0";
  int ret;

  /* Open the framebuffer driver */

  state.fd = open(fbdev, O_RDWR);
  if (state.fd < 0)
    {
      int errcode = errno;
      gerr("Failed to open %s: %d\n", fbdev, errcode);
      return EXIT_FAILURE;
    }

  /* Get the characteristics of the framebuffer */

  ret = ioctl(state.fd, FBIOGET_VIDEOINFO,
              (unsigned long)((uintptr_t)&state.vinfo));
  if (ret < 0)
    {
      int errcode = errno;

      gerr("ioctl(FBIOGET_VIDEOINFO) failed: %d\n", errcode);
      close(state.fd);
      state.fd = -1;
      return EXIT_FAILURE;
    }

  ginfo("VideoInfo:\n\tfmt: %u\n\txres: %u\n\tyres: %u\n\tnplanes: %u\n",
    state.vinfo.fmt, state.vinfo.xres, state.vinfo.yres,
    state.vinfo.nplanes);

  ret = ioctl(state.fd, FBIOGET_PLANEINFO,
              (unsigned long)((uintptr_t)&state.pinfo));
  if (ret < 0)
    {
      int errcode = errno;
      gerr("ioctl(FBIOGET_PLANEINFO) failed: %d\n", errcode);
      close(state.fd);
      state.fd = -1;
      return EXIT_FAILURE;
    }

  ginfo("PlaneInfo (plane 0):\n"
        "\tfbmem: %p\n\tfblen: %l\n\tstride: %u\n"
        "\tdisplay: %u\n\tbpp: %u\n\t",
        state.pinfo.fbmem, (unsigned long)state.pinfo.fblen,
        state.pinfo.stride, state.pinfo.display, state.pinfo.bpp);

  lv_drvr->hor_res = state.vinfo.xres;
  lv_drvr->ver_res = state.vinfo.yres;
#ifndef CONFIG_EXAMPLES_LVGLDEMO_ASYNC_FLUSH
  lv_drvr->flush_cb = fbdev_sync_flush;
#else
  lv_drvr->flush_cb = fbdev_async_flush;
  lv_drvr->wait_cb = fbdev_wait;

  /* Initialize the mutexes for buffer flushing synchronization */

  sem_init(&flush_sem, 0, 0);
  sem_init(&wait_sem, 0, 0);

  /* Initialize the buffer flushing thread */

  pthread_create(&fb_write_thread, NULL, fbdev_write, NULL);
#endif

  /* Only these pixel depths are supported.  viinfo.fmt is ignored, only
   * certain color formats are supported.
   */

  if (state.pinfo.bpp != 32 && state.pinfo.bpp != 16 &&
      state.pinfo.bpp != 8  && state.pinfo.bpp != 1)
    {
      gerr("bpp=%u not supported\n", state.pinfo.bpp);
      close(state.fd);
      state.fd = -1;
      return EXIT_FAILURE;
    }

  /* mmap() the framebuffer.
   *
   * NOTE: In the FLAT build the frame buffer address returned by the
   * FBIOGET_PLANEINFO IOCTL command will be the same as the framebuffer
   * address.  mmap(), however, is the preferred way to get the framebuffer
   * address because in the KERNEL build, it will perform the necessary
   * address mapping to make the memory accessible to the application.
   */

  state.fbmem = mmap(NULL, state.pinfo.fblen, PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_FILE, state.fd, 0);
  if (state.fbmem == MAP_FAILED)
    {
      int errcode = errno;
      gerr("ioctl(FBIOGET_PLANEINFO) failed: %d\n", errcode);
      close(state.fd);
      state.fd = -1;
      return EXIT_FAILURE;
    }

  ginfo("Mapped FB: %p\n", state.fbmem);

  return EXIT_SUCCESS;
}
