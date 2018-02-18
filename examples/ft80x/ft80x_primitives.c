/****************************************************************************
 * examples/ft80x/ft80x_primitives.c
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
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

#include <errno.h>

#include <nuttx/lcd/ft80x.h>

#include "graphics/ft80x.h"
#include "ft80x.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ft80x_rectangles
 *
 * Description:
 *   Demonstate some rectanges
 *
 ****************************************************************************/

int ft80x_rectangles(int fd, FAR struct ft80x_dlbuffer_s *buffer)
{
  uint32_t cmds[15];
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

  cmds[14] = FT80X_DISPLAY();

  /* Create the hardware display list */

  ret = ft80x_dl_start(fd, buffer);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_start failed: %d\n", ret);
      return ret;
    }

  /* Copy the rectangle data into the display list */

  ret = ft80x_dl_data(fd, buffer, cmds, 15 * sizeof(uint32_t));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  /* And terminate the display list */

  ret = ft80x_dl_end(fd, buffer);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_end failed: %d\n", ret);
      return ret;
    }

  return OK;
}
