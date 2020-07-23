/****************************************************************************
 * apps/examples/lvgldemo/fbdev.c
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
 *   Author: Gábor Kiss-Vámosi <kisvegabor@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "fbdev.h"

#include <nuttx/config.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

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

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fbdev_init
 *
 * Description:
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

int fbdev_init(void)
{
  FAR const char *fbdev = "/dev/fb0";
  int ret;

  /* Open the framebuffer driver */

  state.fd = open(fbdev, O_RDWR);
  if (state.fd < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: Failed to open %s: %d\n", fbdev, errcode);
      return EXIT_FAILURE;
    }

  /* Get the characteristics of the framebuffer */

  ret = ioctl(state.fd, FBIOGET_VIDEOINFO,
              (unsigned long)((uintptr_t)&state.vinfo));
  if (ret < 0)
    {
      int errcode = errno;

      fprintf(stderr, "ERROR: ioctl(FBIOGET_VIDEOINFO) failed: %d\n",
              errcode);
      close(state.fd);
      return EXIT_FAILURE;
    }

  printf("VideoInfo:\n");
  printf("      fmt: %u\n", state.vinfo.fmt);
  printf("     xres: %u\n", state.vinfo.xres);
  printf("     yres: %u\n", state.vinfo.yres);
  printf("  nplanes: %u\n", state.vinfo.nplanes);

  ret = ioctl(state.fd, FBIOGET_PLANEINFO,
              (unsigned long)((uintptr_t)&state.pinfo));
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: ioctl(FBIOGET_PLANEINFO) failed: %d\n",
              errcode);
      close(state.fd);
      return EXIT_FAILURE;
    }

  printf("PlaneInfo (plane 0):\n");
  printf("    fbmem: %p\n", state.pinfo.fbmem);
  printf("    fblen: %lu\n", (unsigned long)state.pinfo.fblen);
  printf("   stride: %u\n", state.pinfo.stride);
  printf("  display: %u\n", state.pinfo.display);
  printf("      bpp: %u\n", state.pinfo.bpp);

  /* Only these pixel depths are supported.  viinfo.fmt is ignored, only
   * certain color formats are supported.
   */

  if (state.pinfo.bpp != 32 && state.pinfo.bpp != 16 &&
      state.pinfo.bpp != 8  && state.pinfo.bpp != 1)
    {
      fprintf(stderr, "ERROR: bpp=%u not supported\n", state.pinfo.bpp);
      close(state.fd);
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
      fprintf(stderr, "ERROR: ioctl(FBIOGET_PLANEINFO) failed: %d\n",
              errcode);
      close(state.fd);
      return EXIT_FAILURE;
    }

  printf("Mapped FB: %p\n", state.fbmem);

  return EXIT_SUCCESS;
}

/****************************************************************************
 * Name: fbdev_flush
 *
 * Description:
 *   Flush a buffer to the marked area
 *
 * Input Parameters:
 *   x1      - Left coordinate
 *   y1      - Top coordinate
 *   x2      - Right coordinate
 *   y2      - Bottom coordinate
 *   color_p - A n array of colors
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void fbdev_flush(struct _disp_drv_t *disp_drv, const lv_area_t *area,
                 lv_color_t *color_p)
{
#ifdef CONFIG_FB_UPDATE
  struct fb_area_s area;
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
      return;
    }

  /* Return if the area is out the screen */

  if (x2 < 0)
    {
      return;
    }

  if (y2 < 0)
    {
      return;
    }

  if (x1 > state.vinfo.xres - 1)
    {
      return;
    }

  if (y1 > state.vinfo.yres - 1)
    {
      return;
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
  area.x = act_x1;
  area.y = act_y1;
  area.w = act_x2 - act_x1 + 1;
  area.h = act_y2 - cat_y1 + 1;
  ioctl(state.fd, FBIO_UPDATE, (unsigned long)((uintptr_t)&area));
#endif

  /* Tell the flushing is ready */

  lv_disp_flush_ready(disp_drv);
}

/****************************************************************************
 * Name: fbdev_fill
 *
 * Description:
 *   Fill an area with a color
 *
 * Input Parameters:
 *   x1    - Left coordinate
 *   y1    - Top coordinate
 *   x2    - Right coordinate
 *   y2    - Bottom coordinate
 *   color - The fill color
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void fbdev_fill(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                lv_color_t color)
{
#ifdef CONFIG_FB_UPDATE
  struct fb_area_s area;
#endif
  int32_t act_x1;
  int32_t act_y1;
  int32_t act_x2;
  int32_t act_y2;
  long int location = 0;

  if (state.fbmem == NULL)
    {
      return;
    }

  /* Return if the area is out the screen */

  if (x2 < 0)
    {
      return;
    }

  if (y2 < 0)
    {
      return;
    }

  if (x1 > state.vinfo.xres - 1)
    {
      return;
    }

  if (y1 > state.vinfo.yres - 1)
    {
      return;
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
              fbp8[location] = color.full;
            }
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
              fbp16[location] = color.full;
            }
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
              fbp32[location] = color.full;
            }
        }
    }

#ifdef CONFIG_FB_UPDATE
  area.x = act_x1;
  area.y = act_y1;
  area.w = act_x2 - act_x1 + 1;
  area.h = act_y2 - act_y1 + 1;
  ioctl(state.fd, FBIO_UPDATE, (unsigned long)((uintptr_t)&area));
#endif
}

/****************************************************************************
 * Name: fbdev_map
 *
 * Description:
 *   Write an array of pixels (like an image) to the marked area
 *
 * Input Parameters:
 *   x1      - Left coordinate
 *   y1      - Top coordinate
 *   x2      - Right coordinate
 *   y2      - Bottom coordinate
 *   color_p - An array of colors
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void fbdev_map(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
               FAR const lv_color_t *color_p)
{
#ifdef CONFIG_FB_UPDATE
  struct fb_area_s area;
#endif
  int32_t act_x1;
  int32_t act_y1;
  int32_t act_x2;
  int32_t act_y2;
  long int location = 0;

  if (state.fbmem == NULL)
    {
      return;
    }

  /* Return if the area is out the screen */

  if (x2 < 0)
    {
      return;
    }

  if (y2 < 0)
    {
      return;
    }

  if (x1 > state.vinfo.xres - 1)
    {
      return;
    }

  if (y1 > state.vinfo.yres - 1)
    {
      return;
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
  area.x = act_x1;
  area.y = act_y1;
  area.w = act_x2 - act_x1 + 1;
  area.h = act_y2 - act_y1 + 1;
  ioctl(state.fd, FBIO_UPDATE, (unsigned long)((uintptr_t)&area));
#endif
}
