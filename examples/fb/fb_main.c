/****************************************************************************
 * examples/fb/fb_main.c
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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

#include <nuttx/config.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxglib.h>
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
  FAR void *fbmem;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char g_default_fbdev[] = CONFIG_EXAMPLES_FB_DEFAULTFB;

/* Violet-Blue-Green-Yellow-Orange-Red */

static const uint32_t g_rgb24[NCOLORS] =
{
  RGB24_VIOLET, RGB24_BLUE, RGB24_GREEN, RGB24_YELLOW, RGB24_ORANGE, RGB24_RED
};

static const uint16_t g_rgb16[NCOLORS] =
{
  RGB16_VIOLET, RGB16_BLUE, RGB16_GREEN, RGB16_YELLOW, RGB16_ORANGE, RGB16_RED
};

static const uint8_t g_rgb8[NCOLORS] =
{
  RGB8_VIOLET,  RGB8_BLUE,  RGB8_GREEN,  RGB8_YELLOW,  RGB8_ORANGE,  RGB8_RED
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * draw_rect
 ****************************************************************************/

static void draw_rect32(FAR struct fb_state_s *state,
                        FAR struct nxgl_rect_s *rect, int color)
{
  FAR uint32_t *dest;
  FAR uint8_t *row;
  int x;
  int y;

  row = (FAR uint8_t *)state->fbmem + state->pinfo.stride * rect->pt1.y;
  for (y = rect->pt1.y; y <= rect->pt2.y; y++)
    {
      dest = ((FAR uint32_t *)row) + rect->pt1.x;
      for (x = rect->pt1.x; x <= rect->pt2.x; x++)
        {
          *dest++ = g_rgb24[color];
        }

      row += state->pinfo.stride;
    }
}

static void draw_rect16(FAR struct fb_state_s *state,
                        FAR struct nxgl_rect_s *rect, int color)
{
  FAR uint16_t *dest;
  FAR uint8_t *row;
  int x;
  int y;

  row = (FAR uint8_t *)state->fbmem + state->pinfo.stride * rect->pt1.y;
  for (y = rect->pt1.y; y <= rect->pt2.y; y++)
    {
      dest = ((FAR uint16_t *)row) + rect->pt1.x;
      for (x = rect->pt1.x; x <= rect->pt2.x; x++)
        {
          *dest++ = g_rgb16[color];
        }

      row += state->pinfo.stride;
    }
}

static void draw_rect8(FAR struct fb_state_s *state,
                       FAR struct nxgl_rect_s *rect, int color)
{
  FAR uint8_t *dest;
  FAR uint8_t *row;
  int x;
  int y;

  row = (FAR uint8_t *)state->fbmem + state->pinfo.stride * rect->pt1.y;
  for (y = rect->pt1.y; y <= rect->pt2.y; y++)
    {
      dest = row + rect->pt1.x;
      for (x = rect->pt1.x; x <= rect->pt2.x; x++)
        {
          *dest++ = g_rgb8[color];
        }

      row += state->pinfo.stride;
    }
}

static void draw_rect1(FAR struct fb_state_s *state,
                       FAR struct nxgl_rect_s *rect, int color)
{
  FAR uint8_t *pixel;
  FAR uint8_t *row;
  uint8_t color8 = (color & 1) == 0 ? 0 : 0xff;
  uint8_t lmask;
  uint8_t rmask;
  int startx;
  int endx;
  int x;
  int y;

  /* Calculate the framebuffer address of the first row to draw on */

  row    = (FAR uint8_t *)state->fbmem + state->pinfo.stride * rect->pt1.y;

  /* Calculate the start byte position rounding down so that we get the
   * first byte containing any part of the pixel sequence.  Then calculate
   * the last byte position with a ceil() operation so it includes any final
   * final pixels of the sequence.
   */

  startx = (rect->pt1.x >> 3);
  endx   = ((rect->pt2.x + 7) >> 3);

  /* Caculate a mask on the first and last bytes of the sequence that may
   * not be completely filled with pixel.
   */

  lmask  = 0xff << (8 - (rect->pt1.x & 7));
  rmask  = 0xff >> (rect->pt2.x & 7);

  /* Now draw each row, one-at-a-time */

  for (y = rect->pt1.y; y <= rect->pt2.y; y++)
    {
      /* 'pixel' points to the 1st pixel the next row */

      pixel = row + startx;

      /* Special case: The row is less no more than one byte wide */

      if (startx == endx)
        {
          uint8_t mask = lmask | rmask;

          *pixel = (*pixel & mask) | (color8 & ~mask);
        }
      else
        {

          /* Special case the first byte of the row */

          *pixel = (*pixel & lmask) | (color8 & ~lmask);
          pixel++;

          /* Handle all middle bytes in the row */

          for (x = startx + 1; x < endx; x++)
            {
              *pixel++ = color8;
            }

          /* Handle the final byte of the row */

          *pixel = (*pixel & rmask) | (color8 & ~rmask);
       }

      row += state->pinfo.stride;
    }
}

static void draw_rect(FAR struct fb_state_s *state,
                      FAR struct nxgl_rect_s *rect, int color)
{
#ifdef CONFIG_LCD_UPDATE
  int ret;
#endif

  switch (state->pinfo.bpp)
    {
      case 32:
        draw_rect32(state, rect, color);
        break;

      case 16:
        draw_rect16(state, rect, color);
        break;

      case 8:
      default:
        draw_rect8(state, rect, color);
        break;

      case 1:
        draw_rect1(state, rect, color);
        break;
    }

#ifdef CONFIG_LCD_UPDATE
  ret = ioctl(state->fd, FBIO_UPDATE,
              (unsigned long)((uintptr_t)rect));
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: ioctl(FBIO_UPDATE) failed: %d\n",
              errcode);
    }
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * fb_main
 ****************************************************************************/

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int fb_main(int argc, char *argv[])
#endif
{
  FAR const char *fbdev = g_default_fbdev;
  struct fb_state_s state;
  struct nxgl_rect_s rect;
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

  state.fbmem = mmap(NULL, state.pinfo.fblen, PROT_READ|PROT_WRITE,
                     MAP_SHARED|MAP_FILE, state.fd, 0);
  if (state.fbmem == MAP_FAILED)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: ioctl(FBIOGET_PLANEINFO) failed: %d\n",
              errcode);
      close(state.fd);
      return EXIT_FAILURE;
    }

  printf("Mapped FB: %p\n", state.fbmem);

  /* Draw some rectangles */

  nsteps = 2 * (NCOLORS - 1) + 1;
  xstep  = state.vinfo.xres / nsteps;
  ystep  = state.vinfo.yres / nsteps;
  width  = state.vinfo.xres;
  height = state.vinfo.yres;

  for (x = 0, y = 0, color = 0;
       color < NCOLORS;
       x += xstep, y += ystep, color++)
    {
      rect.pt1.x = x;
      rect.pt1.y = y;
      rect.pt2.x = x + width - 1;
      rect.pt2.y = y + height - 1;

      printf("%2d: (%3d,%3d) (%3d,%3d)\n",
             color, rect.pt1.x, rect.pt1.y, rect.pt2.x, rect.pt2.y);

      draw_rect(&state, &rect, color);
      usleep(500*1000);

      width  -= (2 * xstep);
      height -= (2 * ystep);
    }

  printf("Test finished\n");
  munmap(state.fd, state.fbmem);
  close(state.fd);
  return EXIT_SUCCESS;
}
