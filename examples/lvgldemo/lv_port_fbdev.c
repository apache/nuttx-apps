/****************************************************************************
 * apps/examples/lvgldemo/lv_port_fbdev.c
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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <nuttx/video/fb.h>
#include <lvgl/lvgl.h>

#include "lv_port_fbdev.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_LVGLDEMO_FBDEV_PATH
#  define CONFIG_EXAMPLES_LVGLDEMO_FBDEV_PATH "/dev/fb0"
#endif

#if (LV_COLOR_DEPTH != 32) && (LV_COLOR_DEPTH != 16)
#  error "Only LV_COLOR_DEPTH 32 or 16 is supported by lv_port_fbdev"
#endif

/* SAFEGUARD STEP 3: RAM Sizing
 * Partial buffer (40 rows) to prevent heap exhaustion.
 * 1024 width * 40 lines * 4 bytes/pixel = ~160 KB RAM
 */

#define BUFFER_ROWS 40
#define MAX_HOR_RES 1024
#define BUF_BYTES   (MAX_HOR_RES * BUFFER_ROWS * (LV_COLOR_DEPTH / 8))

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_fbfd = -1;
static uint8_t *g_fb_mem = NULL;
static struct fb_videoinfo_s g_vinfo;
static struct fb_planeinfo_s g_pinfo;
static lv_display_t *g_disp = NULL;

static uint8_t g_buf_1[BUF_BYTES] aligned_data(LV_DRAW_BUF_ALIGN);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fbdev_color_probe_test
 *
 * Description:
 *   SAFEGUARD STEP 1: The Color Probe Test
 *   Writes 3 test pixels directly into mapped framebuffer memory:
 *     32-bit: Red=0xffff0000, Green=0xff00ff00, Blue=0xff0000ff
 *     16-bit: Red=0xf800,     Green=0x07e0,     Blue=0x001f
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_LVGLDEMO_COLOR_PROBE
static void fbdev_color_probe_test(void)
{
  if (g_fb_mem == NULL)
    {
      return;
    }

  if (g_pinfo.bpp == 32)
    {
      uint32_t *probe = (uint32_t *)g_fb_mem;

      /* Probe 0: Red test pixel (ARGB32) */

      probe[0] = 0xffff0000;

      /* Probe 1: Green test pixel (ARGB32) */

      probe[1] = 0xff00ff00;

      /* Probe 2: Blue test pixel (ARGB32) */

      probe[2] = 0xff0000ff;
    }
  else if (g_pinfo.bpp == 16)
    {
      uint16_t *probe = (uint16_t *)g_fb_mem;

      /* Probe 0: Red test pixel (RGB565) */

      probe[0] = 0xf800;

      /* Probe 1: Green test pixel (RGB565) */

      probe[1] = 0x07e0;

      /* Probe 2: Blue test pixel (RGB565) */

      probe[2] = 0x001f;
    }

  LV_LOG_USER("[SAFEGUARD] Color probe test written: 3 pixels\n");
}
#endif

/****************************************************************************
 * Name: fbdev_flush
 *
 * Description:
 *   SAFEGUARD STEP 2: Smart Flush Callback with Byte-Order Adaptation
 *   - Copies line-by-line using hardware stride and visible offsets.
 *   - Matches writes to reported bpp (32-bit vs 16-bit).
 *   - Forces opaque alpha (0xff) on 32-bit to prevent black screen traps.
 ****************************************************************************/

static void fbdev_flush(lv_display_t *disp, const lv_area_t *area,
                        uint8_t *px_map)
{
  lv_coord_t w = lv_area_get_width(area);
  lv_coord_t y;
  uint8_t px_size;

  if (g_fb_mem == NULL)
    {
      lv_display_flush_ready(disp);
      return;
    }

  px_size = lv_color_format_get_size(lv_display_get_color_format(disp));

  if (g_pinfo.bpp == 32)
    {
      for (y = area->y1; y <= area->y2; y++)
        {
          uint32_t *dst = (uint32_t *)(g_fb_mem +
                                        ((y + g_pinfo.yoffset) *
                                         g_pinfo.stride)) +
                          (area->x1 + g_pinfo.xoffset);

          if (px_size == 4)
            {
              uint32_t *src = (uint32_t *)px_map;
              lv_coord_t x;

              for (x = 0; x < w; x++)
                {
                  uint32_t c = src[x];

                  /* LVGL 8.3.2 produces BGRA in memory on little-endian
                   * ARM64. Extract color components:
                   */

                  uint8_t b = (c >> 0)  & 0xff;
                  uint8_t g = (c >> 8)  & 0xff;
                  uint8_t r = (c >> 16) & 0xff;

                  /* Swap Red and Blue channels to match RGBA/ARGB hardware,
                   * and force Opaque Alpha (0xff) to prevent black screen.
                   */

                  dst[x] = 0xff000000u |
                           ((uint32_t)b << 16) |
                           ((uint32_t)g << 8) |
                           (uint32_t)r;
                }
            }
          else if (px_size == 2)
            {
              uint16_t *src = (uint16_t *)px_map;
              lv_coord_t x;

              for (x = 0; x < w; x++)
                {
                  uint16_t c = src[x];
                  uint8_t r = ((c >> 11) & 0x1f) * 255 / 31;
                  uint8_t g = ((c >> 5) & 0x3f) * 255 / 63;
                  uint8_t b = (c & 0x1f) * 255 / 31;

                  dst[x] = 0xff000000u | ((uint32_t)r << 16) |
                           ((uint32_t)g << 8) | (uint32_t)b;
                }
            }

          px_map += w * px_size;
        }
    }
  else if (g_pinfo.bpp == 16)
    {
      for (y = area->y1; y <= area->y2; y++)
        {
          uint16_t *dst = (uint16_t *)(g_fb_mem +
                                        ((y + g_pinfo.yoffset) *
                                         g_pinfo.stride)) +
                          (area->x1 + g_pinfo.xoffset);

          if (px_size == 2)
            {
              memcpy(dst, px_map, w * sizeof(uint16_t));
            }
          else if (px_size == 4)
            {
              uint32_t *src = (uint32_t *)px_map;
              lv_coord_t x;

              for (x = 0; x < w; x++)
                {
                  uint32_t c = src[x];
                  uint8_t r = (c >> 16) & 0xff;
                  uint8_t g = (c >> 8) & 0xff;
                  uint8_t b = c & 0xff;

                  dst[x] = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
                }
            }

          px_map += w * px_size;
        }
    }

#if defined(CONFIG_FB_UPDATE) && defined(FBIO_UPDATE)
  /* Trigger hardware display update if driver supports it */

  struct fb_area_s fb_area;

  fb_area.x = area->x1;
  fb_area.y = area->y1;
  fb_area.w = w;
  fb_area.h = lv_area_get_height(area);
  ioctl(g_fbfd, FBIO_UPDATE, (unsigned long)((uintptr_t)&fb_area));
#endif

  lv_display_flush_ready(disp);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lv_port_disp_init
 ****************************************************************************/

lv_display_t *lv_port_disp_init(void)
{
  int ret;

  /* 1. Open Framebuffer Device */

  g_fbfd = open(CONFIG_EXAMPLES_LVGLDEMO_FBDEV_PATH, O_RDWR);
  if (g_fbfd < 0)
    {
      LV_LOG_ERROR("Cannot open %s, errno=%d\n",
                   CONFIG_EXAMPLES_LVGLDEMO_FBDEV_PATH, errno);
      return NULL;
    }

  /* 2. Retrieve Video Info (Resolution & Format) */

  ret = ioctl(g_fbfd, FBIOGET_VIDEOINFO,
              (unsigned long)((uintptr_t)&g_vinfo));
  if (ret < 0)
    {
      LV_LOG_ERROR("ioctl(FBIOGET_VIDEOINFO) failed: %d\n", errno);
      close(g_fbfd);
      g_fbfd = -1;
      return NULL;
    }

  /* 3. Retrieve Plane Info (Stride, BPP, Framebuffer Length) */

  ret = ioctl(g_fbfd, FBIOGET_PLANEINFO,
              (unsigned long)((uintptr_t)&g_pinfo));
  if (ret < 0)
    {
      LV_LOG_ERROR("ioctl(FBIOGET_PLANEINFO) failed: %d\n", errno);
      close(g_fbfd);
      g_fbfd = -1;
      return NULL;
    }

  /* Validate reported bit depth and color formats */

  if (g_pinfo.bpp == 32)
    {
      if (g_vinfo.fmt != FB_FMT_RGB32 && g_vinfo.fmt != FB_FMT_RGBA32)
        {
          LV_LOG_ERROR("Unsupported 32-bpp format=%u\n", g_vinfo.fmt);
          close(g_fbfd);
          g_fbfd = -1;
          return NULL;
        }
    }
  else if (g_pinfo.bpp == 16)
    {
      if (g_vinfo.fmt != FB_FMT_RGB16_565)
        {
          LV_LOG_ERROR("Unsupported 16-bpp format=%u\n", g_vinfo.fmt);
          close(g_fbfd);
          g_fbfd = -1;
          return NULL;
        }
    }
  else
    {
      LV_LOG_ERROR("Unsupported bpp=%u, format=%u\n",
                   g_pinfo.bpp, g_vinfo.fmt);
      close(g_fbfd);
      g_fbfd = -1;
      return NULL;
    }

  /* Validate geometry against draw buffer size and fblen */

  if (g_vinfo.xres > MAX_HOR_RES)
    {
      LV_LOG_ERROR("xres %u exceeds MAX_HOR_RES %u\n",
                   g_vinfo.xres, MAX_HOR_RES);
      close(g_fbfd);
      g_fbfd = -1;
      return NULL;
    }

  if ((size_t)(g_pinfo.yoffset + g_vinfo.yres) * g_pinfo.stride >
      g_pinfo.fblen)
    {
      LV_LOG_ERROR("Framebuffer geometry exceeds fblen (%zu > %zu)\n",
                   (size_t)(g_pinfo.yoffset + g_vinfo.yres) * g_pinfo.stride,
                   g_pinfo.fblen);
      close(g_fbfd);
      g_fbfd = -1;
      return NULL;
    }

  LV_LOG_USER("[SAFEGUARD] FB: %u x %u, bpp=%u, stride=%u, fblen=%zu\n",
              g_vinfo.xres, g_vinfo.yres, g_pinfo.bpp,
              g_pinfo.stride, g_pinfo.fblen);

  /* 4. Memory-map Framebuffer */

  g_fb_mem = (uint8_t *)mmap(NULL, g_pinfo.fblen, PROT_READ | PROT_WRITE,
                             MAP_SHARED, g_fbfd, 0);
  if (g_fb_mem == MAP_FAILED)
    {
      LV_LOG_ERROR("mmap() failed on fbdev: %d\n", errno);
      close(g_fbfd);
      g_fbfd = -1;
      return NULL;
    }

  /* 5. Optional Color Probe Test */

#ifdef CONFIG_EXAMPLES_LVGLDEMO_COLOR_PROBE
  fbdev_color_probe_test();
#endif

  /* 6. Create LVGL v9 Display with Partial Draw Buffer (40 rows) */

  g_disp = lv_display_create(g_vinfo.xres, g_vinfo.yres);
  if (g_disp == NULL)
    {
      LV_LOG_ERROR("lv_display_create failed\n");
      lv_port_disp_deinit();
      return NULL;
    }

  lv_display_set_buffers(g_disp, g_buf_1, NULL, sizeof(g_buf_1),
                         LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(g_disp, fbdev_flush);

  LV_LOG_USER("[SAFEGUARD] LVGL Display registered successfully.\n");
  return g_disp;
}

/****************************************************************************
 * Name: lv_port_disp_deinit
 ****************************************************************************/

void lv_port_disp_deinit(void)
{
  if (g_disp != NULL)
    {
      lv_display_delete(g_disp);
      g_disp = NULL;
    }

  if (g_fb_mem != NULL && g_fb_mem != MAP_FAILED)
    {
      munmap(g_fb_mem, g_pinfo.fblen);
      g_fb_mem = NULL;
    }

  if (g_fbfd >= 0)
    {
      close(g_fbfd);
      g_fbfd = -1;
    }
}
