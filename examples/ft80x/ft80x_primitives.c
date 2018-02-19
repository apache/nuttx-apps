/****************************************************************************
 * examples/ft80x/ft80x_primitives.c
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Derives from FTDI sample code which appears to have an unrestricted
 * license.  Re-released here under the BSD 3-clause license:
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

#include <errno.h>

#include <nuttx/lcd/ft80x.h>

#include "graphics/ft80x.h"
#include "ft80x.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ft80x_bitmaps
 *
 * Description:
 *   Demonstrate the bitmaps primitive
 *
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_FT80X_EXCLUDE_BITMAPS
int ft80x_bitmaps(int fd, FAR struct ft80x_dlbuffer_s *buffer)
{
  FAR const struct ft80x_bitmaphdr_s *bmhdr = &g_lenaface_bmhdr;
  uint32_t cmds[18];
  int16_t offsetx;
  int16_t offsety;
  int ret;

  /* Copy the image into graphics ram */

  ret = ft80x_ramg_write(fd, 0, bmhdr->data, bmhdr->stride * bmhdr->height);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_ramg_write() failed: %d\n", ret);
      return ret;
    }

  /* Set up the display list */

  cmds[0]  = FT80X_CLEAR(1, 1, 1);               /* Clear screen */
  cmds[1]  = FT80X_COLOR_RGB(255,255,255);
  cmds[2]  = FT80X_BITMAP_SOURCE(FT80X_RAM_G);
  cmds[3]  = FT80X_BITMAP_LAYOUT(bmhdr->format, bmhdr->stride, bmhdr->height);
  cmds[4]  = FT80X_BITMAP_SIZE(FT80X_FILTER_NEAREST, FT80X_WRAP_BORDER,
                         FT80X_WRAP_BORDER, bmhdr->width, bmhdr->height);
  cmds[5]  = FT80X_BEGIN(FT80X_PRIM_BITMAPS);    /* Start drawing bitmaps */

  offsetx  = FT80X_DISPLAY_WIDTH / 4 - bmhdr->width / 2;
  offsety  = FT80X_DISPLAY_HEIGHT / 2 - bmhdr->height / 2;

  cmds[6]  = FT80X_VERTEX2II(offsetx, offsety, 0, 0);
  cmds[7]  = FT80X_COLOR_RGB(255, 64, 64);       /* Red at (200, 120) */

  offsetx  = (FT80X_DISPLAY_WIDTH * 2) / 4 - bmhdr->width / 2;
  offsety  = FT80X_DISPLAY_HEIGHT / 2 - bmhdr->height / 2;

  cmds[8]  = FT80X_VERTEX2II(offsetx, offsety, 0, 0);
  cmds[9]  = FT80X_COLOR_RGB(64, 180, 64);       /* Green at (216, 136) */

  offsetx += bmhdr->width / 2;
  offsety += bmhdr->height / 2;

  cmds[10] = FT80X_VERTEX2II(offsetx, offsety, 0, 0);
  cmds[11] = FT80X_COLOR_RGB(255, 255, 64);      /* Transparent yellow at (232, 152) */
  cmds[12] = FT80X_COLOR_A(150);

  offsetx += bmhdr->width / 2;
  offsety += bmhdr->height / 2;

  cmds[13] = FT80X_VERTEX2II(offsetx, offsety, 0, 0);
  cmds[14] = FT80X_COLOR_A(255);
  cmds[15] = FT80X_COLOR_RGB(255, 255, 255);
  cmds[16] = FT80X_VERTEX2F(-10 * 16, -10 * 16); /* For -ve coordinates use vertex2f instruction */
  cmds[17] = FT80X_END();

  /* Create the hardware display list */

  ret = ft80x_dl_create(fd, buffer, cmds, 18);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_create failed: %d\n", ret);
      return ret;
    }

  return OK;
}
#endif

/****************************************************************************
 * Name: ft80x_points
 *
 * Description:
 *   Demonstrate the points primitive
 *
 ****************************************************************************/

int ft80x_points(int fd, FAR struct ft80x_dlbuffer_s *buffer)
{
  uint32_t cmds[15];
  int ret;

  cmds[0]  = FT80X_CLEAR_COLOR_RGB(128,128,128);
  cmds[1]  = FT80X_CLEAR(1,1,1);
  cmds[2]  = FT80X_COLOR_RGB(128, 0, 0);
  cmds[3]  = FT80X_POINT_SIZE(5 * 16);

  cmds[4]  = FT80X_BEGIN(FT80X_PRIM_POINTS);
  cmds[5]  = FT80X_VERTEX2F((FT80X_DISPLAY_WIDTH / 5 ) * 16,
                            (FT80X_DISPLAY_HEIGHT / 2) * 16);

  cmds[6]  = FT80X_COLOR_RGB(0, 128, 0);
  cmds[7]  = FT80X_POINT_SIZE(15 * 16);
  cmds[8]  = FT80X_VERTEX2F(((FT80X_DISPLAY_WIDTH * 2) / 5) * 16,
                            (FT80X_DISPLAY_HEIGHT / 2) * 16);

  cmds[9]  = FT80X_COLOR_RGB(0, 0, 128);
  cmds[10] = FT80X_POINT_SIZE(25 * 16);
  cmds[11] = FT80X_VERTEX2F(((FT80X_DISPLAY_WIDTH * 3) / 5) * 16,
                            (FT80X_DISPLAY_HEIGHT / 2) * 16);

  cmds[12] = FT80X_COLOR_RGB(128, 128, 0);
  cmds[13] = FT80X_POINT_SIZE(35 * 16);
  cmds[14] = FT80X_VERTEX2F(((FT80X_DISPLAY_WIDTH * 4) / 5) * 16,
                             (FT80X_DISPLAY_HEIGHT / 2) * 16);

  /* Create the hardware display list */

  ret = ft80x_dl_create(fd, buffer, cmds, 15);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_create failed: %d\n", ret);
      return ret;
    }

  return OK;
}

/****************************************************************************
 * Name: ft80x_lines
 *
 * Description:
 *   Demonstrate the lines primitive
 *
 ****************************************************************************/

int ft80x_lines(int fd, FAR struct ft80x_dlbuffer_s *buffer)
{
  uint32_t cmds[14];
  uint32_t height;
  int ret;

  height   = 25;

  cmds[0]  = FT80X_CLEAR(1, 1, 1);  /* Clear screen */
  cmds[1]  = FT80X_COLOR_RGB(128, 0, 0);
  cmds[2]  = FT80X_LINE_WIDTH(5 * 16);
  cmds[3]  = FT80X_BEGIN(FT80X_PRIM_LINES);
  cmds[4]  = FT80X_VERTEX2F((FT80X_DISPLAY_WIDTH / 4) * 16,
                            ((FT80X_DISPLAY_HEIGHT - height) / 2) * 16);
  cmds[5]  = FT80X_VERTEX2F((FT80X_DISPLAY_WIDTH / 4) * 16,
                            ((FT80X_DISPLAY_HEIGHT + height) / 2) * 16);
  cmds[6]  = FT80X_COLOR_RGB(0, 128, 0);
  cmds[7]  = FT80X_LINE_WIDTH(10 * 16);

  height   = 40;

  cmds[8]  = FT80X_VERTEX2F(((FT80X_DISPLAY_WIDTH * 2) /4) * 16,
                            ((FT80X_DISPLAY_HEIGHT - height) / 2) * 16);
  cmds[9]  = FT80X_VERTEX2F(((FT80X_DISPLAY_WIDTH * 2) / 4) * 16,
                            ((FT80X_DISPLAY_HEIGHT + height) / 2) * 16);
  cmds[10] = FT80X_COLOR_RGB(128, 128, 0);
  cmds[11] = FT80X_LINE_WIDTH(20 * 16);

  height   = 55;

  cmds[12] = FT80X_VERTEX2F(((FT80X_DISPLAY_WIDTH * 3) / 4) * 16,
                            ((FT80X_DISPLAY_HEIGHT - height) / 2) * 16);
  cmds[13] = FT80X_VERTEX2F(((FT80X_DISPLAY_WIDTH * 3) / 4) * 16,
                            ((FT80X_DISPLAY_HEIGHT + height)/2) * 16);

  /* Create the hardware display list */

  ret = ft80x_dl_create(fd, buffer, cmds, 14);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_create failed: %d\n", ret);
      return ret;
    }

  return OK;
}

/****************************************************************************
 * Name: ft80x_rectangles
 *
 * Description:
 *   Demonstrate the rectangle primitive
 *
 ****************************************************************************/

int ft80x_rectangles(int fd, FAR struct ft80x_dlbuffer_s *buffer)
{
  uint32_t cmds[14];
  uint32_t width;
  uint32_t height;
  int ret;

  cmds[0]  = FT80X_CLEAR(1, 1, 1);          /* Clear screen */
  cmds[1]  = FT80X_COLOR_RGB(0, 0, 128);
  cmds[2]  = FT80X_LINE_WIDTH(1 * 16);      /* LINE_WIDTH is used for corner curvature */
  cmds[3]  = FT80X_BEGIN(FT80X_PRIM_RECTS); /* Start rectangle primitive */

  width    = 5;
  height   = 25;

  cmds[4]  = FT80X_VERTEX2F(((FT80X_DISPLAY_WIDTH / 4) - (width / 2)) * 16,
                            ((FT80X_DISPLAY_HEIGHT - height) / 2) * 16);
  cmds[5]  = FT80X_VERTEX2F(((FT80X_DISPLAY_WIDTH / 4) + (width / 2)) * 16,
                            ((FT80X_DISPLAY_HEIGHT + height) / 2) * 16);
  cmds[6]  = FT80X_COLOR_RGB(0, 128, 0);
  cmds[7]  = FT80X_LINE_WIDTH(5 * 16);

  width    = 10;
  height   = 40;

  cmds[8]  = FT80X_VERTEX2F((((FT80X_DISPLAY_WIDTH * 2) / 4) - (width / 2)) * 16,
                            ((FT80X_DISPLAY_HEIGHT - height) / 2) * 16);
  cmds[9]  = FT80X_VERTEX2F((((FT80X_DISPLAY_WIDTH * 2) /4) + (width / 2)) * 16,
                            ((FT80X_DISPLAY_HEIGHT + height) / 2) * 16);
  cmds[10] = FT80X_COLOR_RGB(128, 128, 0);
  cmds[11] = FT80X_LINE_WIDTH(10 * 16);

  width    = 20;
  height   = 55;

  cmds[12] = FT80X_VERTEX2F((((FT80X_DISPLAY_WIDTH * 3) / 4) - (width / 2)) * 16,
                            ((FT80X_DISPLAY_HEIGHT - height) / 2) * 16);
  cmds[13] = FT80X_VERTEX2F((((FT80X_DISPLAY_WIDTH * 3) / 4) + (width /2 )) * 16,
                            ((FT80X_DISPLAY_HEIGHT + height) / 2) * 16);

  /* Create the hardware display list */

  ret = ft80x_dl_create(fd, buffer, cmds, 14);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_create failed: %d\n", ret);
      return ret;
    }

  return OK;
}
