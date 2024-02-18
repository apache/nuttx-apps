/****************************************************************************
 * apps/graphics/lvgl/port/lv_port_lcddev.c
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
#include <pthread.h>
#include <semaphore.h>
#include "lv_port_lcddev.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

struct lcddev_obj_s
{
  int fd;
  lv_disp_draw_buf_t disp_draw_buf;
  lv_disp_drv_t disp_drv;
  struct lcddev_area_s area;

  pthread_t write_thread;
  sem_t flush_sem;
  sem_t wait_sem;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lcddev_wait
 ****************************************************************************/

static void lcddev_wait(FAR lv_disp_drv_t *disp_drv)
{
  FAR struct lcddev_obj_s *lcddev_obj = disp_drv->user_data;

  sem_wait(&(lcddev_obj->wait_sem));

  /* Tell the flushing is ready */

  lv_disp_flush_ready(disp_drv);
}

/****************************************************************************
 * Name: lcddev_update_thread
 ****************************************************************************/

static FAR void *lcddev_update_thread(FAR void *arg)
{
  int ret = OK;
  int errcode = 0;
  FAR struct lcddev_obj_s *lcddev_obj = arg;

  while (ret == OK)
    {
      sem_wait(&lcddev_obj->flush_sem);

      ret = ioctl(lcddev_obj->fd, LCDDEVIO_PUTAREA,
                  (unsigned long)&(lcddev_obj->area));
      if (ret < 0)
        {
          errcode = errno;
        }

      sem_post(&lcddev_obj->wait_sem);
    }

  LV_LOG_ERROR("ioctl(LCDDEVIO_PUTAREA) failed: %d", errcode);
  close(lcddev_obj->fd);
  lcddev_obj->fd = -1;

  return NULL;
}

/****************************************************************************
 * Name: lcddev_flush
 ****************************************************************************/

static void lcddev_flush(FAR lv_disp_drv_t *disp_drv,
                         FAR const lv_area_t *area_p,
                         FAR lv_color_t *color_p)
{
  FAR struct lcddev_obj_s *lcddev_obj = disp_drv->user_data;

  lcddev_obj->area.row_start = area_p->y1;
  lcddev_obj->area.row_end = area_p->y2;
  lcddev_obj->area.col_start = area_p->x1;
  lcddev_obj->area.col_end = area_p->x2;
  lcddev_obj->area.data = (FAR uint8_t *)color_p;

  sem_post(&lcddev_obj->flush_sem);
}

/****************************************************************************
 * Name: lcddev_init
 ****************************************************************************/

static FAR lv_disp_t *lcddev_init(int fd,
                                  int hor_res,
                                  int ver_res,
                                  int line_buf)
{
  FAR lv_color_t *buf1 = NULL;
  FAR lv_color_t *buf2 = NULL;
  FAR struct lcddev_obj_s *lcddev_obj;
  const size_t buf_size = hor_res * line_buf * sizeof(lv_color_t);
  lcddev_obj = malloc(sizeof(struct lcddev_obj_s));

  if (lcddev_obj == NULL)
    {
      LV_LOG_ERROR("lcddev_obj_s malloc failed");
      return NULL;
    }

  memset(lcddev_obj, 0, sizeof(struct lcddev_obj_s));

  buf1 = malloc(buf_size);
  if (buf1 == NULL)
    {
      LV_LOG_ERROR("display buf1 malloc failed");
      free(lcddev_obj);
      return NULL;
    }

#ifdef CONFIG_LV_PORT_LCDDEV_DOUBLE_BUFFER
  buf2 = malloc(buf_size);
  if (buf2 == NULL)
    {
      LV_LOG_ERROR("display buf2 malloc failed");
      free(lcddev_obj);
      free(buf1);
      return NULL;
    }
#endif

  LV_LOG_INFO("display buffer malloc success, buf size = %lu", buf_size);

  lcddev_obj->fd = fd;

  lv_disp_draw_buf_init(&(lcddev_obj->disp_draw_buf), buf1, buf2,
                        hor_res * line_buf);

  lv_disp_drv_init(&(lcddev_obj->disp_drv));
  lcddev_obj->disp_drv.flush_cb = lcddev_flush;
  lcddev_obj->disp_drv.draw_buf = &(lcddev_obj->disp_draw_buf);
  lcddev_obj->disp_drv.hor_res = hor_res;
  lcddev_obj->disp_drv.ver_res = ver_res;
  lcddev_obj->disp_drv.screen_transp = false;
#if ( LV_USE_USER_DATA != 0 )
  lcddev_obj->disp_drv.user_data = lcddev_obj;
#else
#error LV_USE_USER_DATA must be enabled
#endif
  lcddev_obj->disp_drv.wait_cb = lcddev_wait;

  /* Initialize the mutexes for buffer flushing synchronization */

  sem_init(&lcddev_obj->flush_sem, 0, 0);
  sem_init(&lcddev_obj->wait_sem, 0, 0);

  /* Initialize the buffer flushing thread */

  pthread_create(&(lcddev_obj->write_thread), NULL,
                 lcddev_update_thread, lcddev_obj);

  return lv_disp_drv_register(&(lcddev_obj->disp_drv));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lv_port_lcddev_init
 *
 * Description:
 *   Lcd interface initialization.
 *
 * Input Parameters:
 *   dev_path - lcd device path, set to NULL to use the default path
 *   line_buf - Number of line buffers,
 *              set to 0 to use the default line buffer
 *
 * Returned Value:
 *   lv_disp object address on success; NULL on failure.
 *
 ****************************************************************************/

FAR lv_disp_t *lv_port_lcddev_init(FAR const char *dev_path, int line_buf)
{
  FAR const char *device_path = dev_path;
  int line_buffer = line_buf;
  int fd;
  int ret;
  FAR lv_disp_t *disp;

  struct fb_videoinfo_s vinfo;
  struct lcd_planeinfo_s pinfo;

  if (device_path == NULL)
    {
      device_path = CONFIG_LV_PORT_LCDDEV_DEFAULT_DEVICEPATH;
    }

  LV_LOG_INFO("lcddev %s opening", device_path);
  fd = open(device_path, 0);
  if (fd < 0)
    {
      LV_LOG_ERROR("lcddev open failed: %d", errno);
      return NULL;
    }

  LV_LOG_INFO("lcddev %s open success", device_path);

  ret = ioctl(fd, LCDDEVIO_GETVIDEOINFO,
              (unsigned long)((uintptr_t)&vinfo));
  if (ret < 0)
    {
      LV_LOG_ERROR("ioctl(LCDDEVIO_GETVIDEOINFO) failed: %d", errno);
      close(fd);
      return NULL;
    }

  LV_LOG_INFO("VideoInfo:");
  LV_LOG_INFO("      fmt: %u", vinfo.fmt);
  LV_LOG_INFO("     xres: %u", vinfo.xres);
  LV_LOG_INFO("     yres: %u", vinfo.yres);
  LV_LOG_INFO("  nplanes: %u", vinfo.nplanes);

  ret = ioctl(fd, LCDDEVIO_GETPLANEINFO,
              (unsigned long)((uintptr_t)&pinfo));
  if (ret < 0)
    {
      LV_LOG_ERROR("ERROR: ioctl(LCDDEVIO_GETPLANEINFO) failed: %d",
                   errno);
      close(fd);
      return NULL;
    }

  LV_LOG_INFO("PlaneInfo:");
  LV_LOG_INFO("      bpp: %u", pinfo.bpp);

  if (pinfo.bpp != LV_COLOR_DEPTH)
    {
      LV_LOG_ERROR("lcddev bpp = %d, LV_COLOR_DEPTH = %d,"
          " color depth does not match", pinfo.bpp, LV_COLOR_DEPTH);
      close(fd);
      return NULL;
    }

#ifdef CONFIG_LV_PORT_LCDDEV_FULL_SCREEN_BUFFER
  line_buffer = vinfo.yres;
#else
  if (line_buffer <= 0)
    {
      line_buffer = CONFIG_LV_PORT_LCDDEV_LINE_BUFFER_DEFAULT;
    }
#endif

  disp = lcddev_init(fd, vinfo.xres, vinfo.yres,
                     line_buffer);
  if (disp == NULL)
    {
      close(fd);
    }

  return disp;
}
