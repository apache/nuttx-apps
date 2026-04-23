/****************************************************************************
 * apps/system/vncviewer/lcd_render.c
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <nuttx/video/fb.h>
#include <nuttx/lcd/lcd_dev.h>

#include "lcd_render.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lcd_init
 ****************************************************************************/

int lcd_init(struct lcd_ctx_s *ctx, int devno)
{
  char devpath[16];
  int ret;

  snprintf(devpath, sizeof(devpath), "/dev/lcd%d", devno);

  ctx->fd = open(devpath, O_RDWR);
  if (ctx->fd < 0)
    {
      printf("vncviewer: failed to open %s: %d\n", devpath, errno);
      return -errno;
    }

  /* Query LCD resolution */

  struct fb_videoinfo_s vinfo;
  ret = ioctl(ctx->fd, LCDDEVIO_GETVIDEOINFO,
             (unsigned long)(uintptr_t)&vinfo);
  if (ret < 0)
    {
      printf("vncviewer: GETVIDEOINFO failed: %d\n", errno);
      close(ctx->fd);
      ctx->fd = -1;
      return -errno;
    }
  else
    {
      ctx->xres = vinfo.xres;
      ctx->yres = vinfo.yres;
      ctx->fmt  = vinfo.fmt;
    }

  printf("vncviewer: LCD %s opened (%ux%u)\n",
         devpath, ctx->xres, ctx->yres);

  /* Set power on */

  ret = ioctl(ctx->fd, LCDDEVIO_SETPOWER, 1);
  if (ret < 0)
    {
      printf("vncviewer: WARNING: SETPOWER failed: %d\n", errno);
    }

  return OK;
}

/****************************************************************************
 * Name: lcd_put_row
 ****************************************************************************/

int lcd_put_row(struct lcd_ctx_s *ctx, uint16_t x, uint16_t y,
                uint16_t w, const uint16_t *pixels)
{
  struct lcddev_area_s area;
  int ret;

  /* Clip to LCD bounds */

  if (x >= ctx->xres || y >= ctx->yres)
    {
      return OK;
    }

  if (x + w > ctx->xres)
    {
      w = ctx->xres - x;
    }

  area.row_start = y;
  area.row_end   = y;
  area.col_start = x;
  area.col_end   = x + w - 1;
  area.data      = (uint8_t *)pixels;

  ret = ioctl(ctx->fd, LCDDEVIO_PUTAREA, (unsigned long)(uintptr_t)&area);
  if (ret < 0)
    {
      return -errno;
    }

  return OK;
}

/****************************************************************************
 * Name: lcd_fill
 ****************************************************************************/

int lcd_fill(struct lcd_ctx_s *ctx, uint16_t color)
{
  uint16_t y;
  uint16_t i;
  uint16_t w = ctx->xres;
  FAR uint16_t *rowbuf;

  rowbuf = (FAR uint16_t *)malloc(w * sizeof(uint16_t));
  if (rowbuf == NULL)
    {
      return -ENOMEM;
    }

  for (i = 0; i < w; i++)
    {
      rowbuf[i] = color;
    }

  for (y = 0; y < ctx->yres; y++)
    {
      lcd_put_row(ctx, 0, y, w, rowbuf);
    }

  free(rowbuf);

  return OK;
}

/****************************************************************************
 * Name: lcd_uninit
 ****************************************************************************/

void lcd_uninit(struct lcd_ctx_s *ctx)
{
  if (ctx->fd >= 0)
    {
      close(ctx->fd);
      ctx->fd = -1;
    }
}
