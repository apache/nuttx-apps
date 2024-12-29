/****************************************************************************
 * apps/examples/fb/fb_main.c
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

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>

#include <nuttx/video/fb.h>
#include <nuttx/video/rgbcolors.h>

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

#define NCOLORS 6

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct fb_state_s
{
  int fd;
  struct fb_videoinfo_s vinfo;
  struct fb_planeinfo_s pinfo;
#ifdef CONFIG_FB_OVERLAY
  struct fb_overlayinfo_s oinfo;
#endif
  FAR void *fbmem;
  FAR void *fbmem2;
  FAR void *act_fbmem;
  uint32_t mem2_yoffset;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char g_default_fbdev[] = CONFIG_EXAMPLES_FB_DEFAULTFB;

/* Violet-Blue-Green-Yellow-Orange-Red */

static const uint32_t g_rgb24[NCOLORS] =
{
  RGB24_VIOLET, RGB24_BLUE, RGB24_GREEN,
  RGB24_YELLOW, RGB24_ORANGE, RGB24_RED
};

static const uint16_t g_rgb16[NCOLORS] =
{
  RGB16_VIOLET, RGB16_BLUE, RGB16_GREEN,
  RGB16_YELLOW, RGB16_ORANGE, RGB16_RED
};

static const uint8_t g_rgb8[NCOLORS] =
{
  RGB8_VIOLET, RGB8_BLUE, RGB8_GREEN,
  RGB8_YELLOW, RGB8_ORANGE, RGB8_RED
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * sync_area
 ****************************************************************************/

static void sync_area(FAR struct fb_state_s *state)
{
  if (state->fbmem2 == NULL)
    {
      return;
    }

  if (state->act_fbmem == state->fbmem)
    {
      memcpy(state->fbmem, state->fbmem2,
             state->vinfo.yres * state->pinfo.stride);
    }
  else
    {
      memcpy(state->fbmem2, state->fbmem,
             state->vinfo.yres * state->pinfo.stride);
    }
}

/****************************************************************************
 * pan_display
 ****************************************************************************/

static void pan_display(FAR struct fb_state_s *state)
{
  struct pollfd pfd;
  int ret;
  pfd.fd = state->fd;
  pfd.events = POLLOUT;

  ret = poll(&pfd, 1, 0);

  if (ret > 0)
    {
      if (state->fbmem2 == NULL)
        {
          return;
        }

      if (state->act_fbmem == state->fbmem)
        {
          state->pinfo.yoffset = 0;
        }
      else
        {
          state->pinfo.yoffset = state->mem2_yoffset;
        }

      ioctl(state->fd, FBIOPAN_DISPLAY,
            (unsigned long)(uintptr_t)&state->pinfo);

      state->act_fbmem = state->act_fbmem == state->fbmem ?
                         state->fbmem2 : state->fbmem;
    }
}

/****************************************************************************
 * fbdev_get_pinfo
 ****************************************************************************/

static int fbdev_get_pinfo(int fd, FAR struct fb_planeinfo_s *pinfo)
{
  if (ioctl(fd, FBIOGET_PLANEINFO, (unsigned long)((uintptr_t)pinfo)) < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: ioctl(FBIOGET_PLANEINFO) failed: %d\n",
              errcode);
      return EXIT_FAILURE;
    }

  printf("PlaneInfo (plane %d):\n", pinfo->display);
  printf("    fbmem: %p\n", pinfo->fbmem);
  printf("    fblen: %zu\n", pinfo->fblen);
  printf("   stride: %u\n", pinfo->stride);
  printf("  display: %u\n", pinfo->display);
  printf("      bpp: %u\n", pinfo->bpp);

  /* Only these pixel depths are supported.  viinfo.fmt is ignored, only
   * certain color formats are supported.
   */

  if (pinfo->bpp != 32 && pinfo->bpp != 24 &&
      pinfo->bpp != 16 && pinfo->bpp != 8 &&
      pinfo->bpp != 1)
    {
      fprintf(stderr, "ERROR: bpp=%u not supported\n", pinfo->bpp);
      return EXIT_FAILURE;
    }

  return 0;
}

/****************************************************************************
 * fb_init_mem2
 ****************************************************************************/

static int fb_init_mem2(FAR struct fb_state_s *state)
{
  int ret;
  uintptr_t buf_offset;
  struct fb_planeinfo_s pinfo;

  memset(&pinfo, 0, sizeof(pinfo));
  pinfo.display = state->pinfo.display + 1;

  if ((ret = fbdev_get_pinfo(state->fd, &pinfo)) < 0)
    {
      return EXIT_FAILURE;
    }

  /* Check bpp */

  if (pinfo.bpp != state->pinfo.bpp)
    {
      fprintf(stderr, "ERROR: fbmem2 is incorrect");
      return -EINVAL;
    }

  /* Check the buffer address offset,
   * It needs to be divisible by pinfo.stride
   */

  buf_offset = pinfo.fbmem - state->fbmem;

  if ((buf_offset % state->pinfo.stride) != 0)
    {
      fprintf(stderr, "ERROR: It is detected that buf_offset(%" PRIuPTR ") "
              "and stride(%d) are not divisible, please ensure "
              "that the driver handles the address offset by itself.\n",
              buf_offset, state->pinfo.stride);
    }

  /* Calculate the address and yoffset of fbmem2 */

  if (buf_offset == 0)
    {
      /* Use consecutive fbmem2. */

      state->mem2_yoffset = state->vinfo.yres;
      state->fbmem2 = pinfo.fbmem + state->mem2_yoffset * pinfo.stride;
      printf("Use consecutive fbmem2 = %p, yoffset = %" PRIu32"\n",
             state->fbmem2, state->mem2_yoffset);
    }
  else
    {
      /* Use non-consecutive fbmem2. */

      state->mem2_yoffset = buf_offset / state->pinfo.stride;
      state->fbmem2 = pinfo.fbmem;
      printf("Use non-consecutive fbmem2 = %p, yoffset = %" PRIu32"\n",
             state->fbmem2, state->mem2_yoffset);
    }

  return 0;
}

/****************************************************************************
 * draw_rect
 ****************************************************************************/

static void draw_rect32(FAR struct fb_state_s *state,
                        FAR struct fb_area_s *area, int color)
{
  FAR uint32_t *dest;
  FAR uint8_t *row;
  int x;
  int y;

  row = (FAR uint8_t *)state->act_fbmem + state->pinfo.stride * area->y;
  for (y = 0; y < area->h; y++)
    {
      dest = ((FAR uint32_t *)row) + area->x;
      for (x = 0; x < area->w; x++)
        {
          *dest++ = g_rgb24[color] | 0xff000000;
        }

      row += state->pinfo.stride;
    }
}

static void draw_rect24(FAR struct fb_state_s *state,
                        FAR struct fb_area_s *area, int color)
{
  FAR uint8_t *dest;
  FAR uint8_t *row;
  int x;
  int y;

  row = (FAR uint8_t *)state->act_fbmem + state->pinfo.stride * area->y;
  for (y = 0; y < area->h; y++)
    {
      dest = ((FAR uint8_t *)row) + area->x * 3;
      for (x = 0; x < area->w; x++)
        {
          *dest++ = g_rgb24[color] & 0xff;
          *dest++ = (g_rgb24[color] >> 8) & 0xff;
          *dest++ = (g_rgb24[color] >> 16) & 0xff;
        }

      row += state->pinfo.stride;
    }
}

static void draw_rect16(FAR struct fb_state_s *state,
                        FAR struct fb_area_s *area, int color)
{
  FAR uint16_t *dest;
  FAR uint8_t *row;
  int x;
  int y;

  row = (FAR uint8_t *)state->act_fbmem + state->pinfo.stride * area->y;
  for (y = 0; y < area->h; y++)
    {
      dest = ((FAR uint16_t *)row) + area->x;
      for (x = 0; x < area->w; x++)
        {
          *dest++ = g_rgb16[color];
        }

      row += state->pinfo.stride;
    }
}

static void draw_rect8(FAR struct fb_state_s *state,
                       FAR struct fb_area_s *area, int color)
{
  FAR uint8_t *dest;
  FAR uint8_t *row;
  int x;
  int y;

  row = (FAR uint8_t *)state->act_fbmem + state->pinfo.stride * area->y;
  for (y = 0; y < area->h; y++)
    {
      dest = row + area->x;
      for (x = 0; x < area->w; x++)
        {
          *dest++ = g_rgb8[color];
        }

      row += state->pinfo.stride;
    }
}

static void draw_rect1(FAR struct fb_state_s *state,
                       FAR struct fb_area_s *area, int color)
{
  FAR uint8_t *pixel;
  FAR uint8_t *row;
  uint8_t color8 = (color & 1) == 0 ? 0 : 0xff;

  int start_full_x;
  int end_full_x;
  int start_bit_shift;
  int last_bits;
  uint8_t lmask;
  uint8_t rmask;
  int y;

  /* Calculate the framebuffer address of the first row to draw on */

  row    = (FAR uint8_t *)state->act_fbmem + state->pinfo.stride * area->y;

  /* Calculate the position of the first complete (with all bits) byte.
   * Then calculate the last byte with all the bits.
   */

  start_full_x = ((area->x + 7) >> 3);
  end_full_x   = ((area->x + area->w) >> 3);

  /* Calculate the number of bits in byte before start that need to remain
   * unchanged. Later calculate the mask.
   */

  start_bit_shift = 8 + area->x - (start_full_x << 3);
  lmask = 0xff >> start_bit_shift;

  /* Calculate the number of bits that needs to be changed after last byte
   * with all the bits. Later calculate the mask.
   */

  last_bits = area->x + area->w - (end_full_x << 3);
  rmask = 0xff << (8 - last_bits);

  /* Calculate a mask on the first and last bytes of the sequence that may
   * not be completely filled with pixel.
   */

  /* Now draw each row, one-at-a-time */

  for (y = 0; y < area->h; y++)
    {
      /* 'pixel' points to the 1st pixel the next row */

      /* Special case: The row starts and ends within the same byte */

      if (start_full_x > end_full_x)
        {
          pixel = row + start_full_x - 1;
          *pixel = (*pixel & (~lmask | ~rmask)) | (lmask & rmask & color8);
          continue;
        }

      if (start_bit_shift != 0)
        {
          pixel = row + start_full_x - 1;
          *pixel = (*pixel & ~lmask) | (lmask & color8);
        }

      if (end_full_x > start_full_x)
        {
          pixel = row + start_full_x;
          memset(pixel, color8, end_full_x - start_full_x);
        }

      if (last_bits != 0)
        {
          pixel = row + end_full_x;
          *pixel = (*pixel & ~rmask) | (rmask & color8);
        }

      row += state->pinfo.stride;
    }
}

static void draw_rect(FAR struct fb_state_s *state,
                      FAR struct fb_area_s *area, int color)
{
#ifdef CONFIG_FB_UPDATE
  int ret;
#endif

  switch (state->pinfo.bpp)
    {
      case 32:
        draw_rect32(state, area, color);
        break;

      case 24:
        draw_rect24(state, area, color);
        break;

      case 16:
        draw_rect16(state, area, color);
        break;

      case 8:
      default:
        draw_rect8(state, area, color);
        break;

      case 1:
        draw_rect1(state, area, color);
        break;
    }

#ifdef CONFIG_FB_UPDATE
  int yoffset = state->act_fbmem == state->fbmem ?
                0 : state->mem2_yoffset;
  area->y += yoffset;

  ret = ioctl(state->fd, FBIO_UPDATE,
              (unsigned long)((uintptr_t)area));
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: ioctl(FBIO_UPDATE) failed: %d\n",
              errcode);
    }
#endif

  if (state->pinfo.yres_virtual == (state->vinfo.yres * 2))
    {
      pan_display(state);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * fb_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR const char *fbdev = g_default_fbdev;
  struct fb_state_s state;
  struct fb_area_s area;
  int nsteps;
  int xstep;
  int ystep;
  int width;
  int height;
  int color;
  int x;
  int y;
  int ret;

  /* There is a single required argument:  The path to the framebuffer
   * driver.
   */

  if (argc == 2)
    {
      fbdev = argv[1];
    }
  else if (argc != 1)
    {
      fprintf(stderr, "ERROR: Single argument required\n");
      fprintf(stderr, "USAGE: %s [<fb-driver-path>]\n", argv[0]);
      return EXIT_FAILURE;
    }

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

#ifdef CONFIG_FB_OVERLAY
  printf("noverlays: %u\n", state.vinfo.noverlays);

  /* Select the first overlay, which should be the composed framebuffer */

  ret = ioctl(state.fd, FBIO_SELECT_OVERLAY, 0);
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: ioctl(FBIO_SELECT_OVERLAY) failed: %d\n",
              errcode);
      close(state.fd);
      return EXIT_FAILURE;
    }

  /* Get the first overlay information */

  state.oinfo.overlay = 0;
  ret = ioctl(state.fd, FBIOGET_OVERLAYINFO,
                        (unsigned long)((uintptr_t)&state.oinfo));
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: ioctl(FBIOGET_OVERLAYINFO) failed: %d\n",
              errcode);
      close(state.fd);
      return EXIT_FAILURE;
    }

  printf("OverlayInfo (overlay 0):\n");
  printf("    fbmem: %p\n", state.oinfo.fbmem);
  printf("    fblen: %zu\n", state.oinfo.fblen);
  printf("   stride: %u\n", state.oinfo.stride);
  printf("  overlay: %u\n", state.oinfo.overlay);
  printf("      bpp: %u\n", state.oinfo.bpp);
  printf("    blank: %u\n", state.oinfo.blank);
  printf("chromakey: 0x%08" PRIx32 "\n", state.oinfo.chromakey);
  printf("    color: 0x%08" PRIx32 "\n", state.oinfo.color);
  printf("   transp: 0x%02x\n", state.oinfo.transp.transp);
  printf("     mode: %u\n", state.oinfo.transp.transp_mode);
  printf("     area: (%u,%u) => (%u,%u)\n",
                      state.oinfo.sarea.x, state.oinfo.sarea.y,
                      state.oinfo.sarea.w, state.oinfo.sarea.h);
  printf("     accl: %" PRIu32 "\n", state.oinfo.accl);

  /* select default framebuffer layer */

  ret = ioctl(state.fd, FBIO_SELECT_OVERLAY, FB_NO_OVERLAY);
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: ioctl(FBIO_SELECT_OVERLAY) failed: %d\n",
              errcode);
      close(state.fd);
      return EXIT_FAILURE;
    }

#endif

  if ((ret = fbdev_get_pinfo(state.fd, &state.pinfo)) < 0)
    {
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

  /* double buffer mode */

  if (state.pinfo.yres_virtual == (state.vinfo.yres * 2))
    {
      if ((ret = fb_init_mem2(&state)) < 0)
        {
          goto out;
        }
    }

  /* Draw some rectangles */

  state.act_fbmem = state.fbmem;
  nsteps = 2 * (NCOLORS - 1) + 1;
  xstep  = state.vinfo.xres / nsteps;
  ystep  = state.vinfo.yres / nsteps;
  width  = state.vinfo.xres;
  height = state.vinfo.yres;

  for (x = 0, y = 0, color = 0;
       color < NCOLORS;
       x += xstep, y += ystep, color++)
    {
      area.x = x;
      area.y = y;
      area.w = width;
      area.h = height;

      printf("%2d: (%3d,%3d) (%3d,%3d)\n",
             color, area.x, area.y, area.w, area.h);

      draw_rect(&state, &area, color);
      usleep(500 * 1000);

      width  -= (2 * xstep);
      height -= (2 * ystep);

      /* double buffer mode */

      if (state.pinfo.yres_virtual == (state.vinfo.yres * 2))
        {
          sync_area(&state);
        }
    }

  printf("Test finished\n");
  ret = EXIT_SUCCESS;
out:
  munmap(state.fbmem, state.pinfo.fblen);
  close(state.fd);
  return ret;
}
