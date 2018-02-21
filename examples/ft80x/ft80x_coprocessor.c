/****************************************************************************
 * examples/ft80x/ft80x_coprocessor.c
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
 * Name: ft80x_coproc_progressbar
 *
 * Description:
 *   Demonstrate the progress bar command
 *
 ****************************************************************************/

int ft80x_coproc_progressbar(int fd, FAR struct ft80x_dlbuffer_s *buffer)
{
  int16_t xoffset;
  int16_t yoffset;
  int16_t ydist = FT80X_DISPLAY_WIDTH / 12;
  int16_t ysz = FT80X_DISPLAY_WIDTH / 24;
  int ret;

  /* Formatted output chunks */

  union
  {
    struct
    {
      struct ft80x_cmd32_s        clearrgb;
      struct ft80x_cmd32_s        clear;
      struct ft80x_cmd32_s        colorrgb;
    } a;
    struct
    {
      struct ft80x_cmd32_s        colorrgb;
      struct ft80x_cmd_bgcolor_s  bgcolor;
      struct ft80x_cmd_progress_s progress;
      struct ft80x_cmd32_s        colora;
      struct ft80x_cmd_text_s     text;
    } b;
    struct
    {
      struct ft80x_cmd32_s        colorrgb;
      struct ft80x_cmd_bgcolor_s  bgcolor;
    } c;
    struct
    {
      struct ft80x_cmd_progress_s progress;
      struct ft80x_cmd_bgcolor_s  bgcolor;
    } d;
    struct
    {
      struct ft80x_cmd_progress_s progress;
      struct ft80x_cmd_text_s     text;
    } e;
    struct ft80x_cmd_progress_s   progress;
  } cmds;

  /* Create the hardware display list */

  ret = ft80x_dl_start(fd, buffer, true);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_start failed: %d\n", ret);
      return ret;
    }

  cmds.a.clearrgb.cmd     = FT80X_CLEAR_COLOR_RGB(64, 64, 64);
  cmds.a.clear.cmd        = FT80X_CLEAR(1 ,1, 1);
  cmds.a.colorrgb.cmd     = FT80X_COLOR_RGB(0xff, 0xff, 0xff);

  /* Copy the commands into the display list */

  ret = ft80x_dl_data(fd, buffer, &cmds.a, sizeof(cmds.a));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  /* Draw progress bar with flat effect */

  cmds.b.colorrgb.cmd     = FT80X_COLOR_RGB(0xff, 0xff, 0xff);

  cmds.b.bgcolor.cmd      = FT80X_CMD_BGCOLOR;                /* Background color */
  cmds.b.bgcolor.c        = 0x404080;

  cmds.b.progress.cmd     = FT80X_CMD_PROGRESS;               /* Progress bar */
  cmds.b.progress.x       = 20;
  cmds.b.progress.y       = 10;
  cmds.b.progress.w       = 120;
  cmds.b.progress.h       = 20;
  cmds.b.progress.options = FT80X_OPT_FLAT;
  cmds.b.progress.val     = 50;
  cmds.b.progress.range   = 100;

  cmds.b.colora.cmd       = FT80X_COLOR_A(255);               /* Color A */

  cmds.b.text.cmd         = FT80X_CMD_TEXT;                   /* Text */
  cmds.b.text.x           = 20;
  cmds.b.text.y           = 40;
  cmds.b.text.font        = 26;
  cmds.b.text.options     = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "Flat effect");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  /* Draw progress bar with 3d effect */

  cmds.b.colorrgb.cmd     = FT80X_COLOR_RGB(0x00, 0xff, 0x00);

  cmds.b.bgcolor.cmd      = FT80X_CMD_BGCOLOR;                /* Background color */
  cmds.b.bgcolor.c        = 0x800000;

  cmds.b.progress.cmd     = FT80X_CMD_PROGRESS;               /* Progress bar */
  cmds.b.progress.x       = 180;
  cmds.b.progress.y       = 10;
  cmds.b.progress.w       = 120;
  cmds.b.progress.h       = 20;
  cmds.b.progress.options = 0;
  cmds.b.progress.val     = 75;
  cmds.b.progress.range   = 100;

  cmds.b.colora.cmd       = FT80X_COLOR_A(255);               /* Color A */

  cmds.b.text.cmd         = FT80X_CMD_TEXT;                   /* Text */
  cmds.b.text.x           = 180;
  cmds.b.text.y           = 40;
  cmds.b.text.font        = 26;
  cmds.b.text.options     = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "3D effect");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  /* Draw progress bar with 3d effect and string on top */

  cmds.b.colorrgb.cmd     = FT80X_COLOR_RGB(0xff, 0x00, 0x00);

  cmds.b.bgcolor.cmd      = FT80X_CMD_BGCOLOR;                /* Background color */
  cmds.b.bgcolor.c        = 0x000080;

  cmds.b.progress.cmd     = FT80X_CMD_PROGRESS;               /* Progress bar */
  cmds.b.progress.x       = 30;
  cmds.b.progress.y       = 60;
  cmds.b.progress.w       = 120;
  cmds.b.progress.h       = 30;
  cmds.b.progress.options = 0;
  cmds.b.progress.val     = 19660;
  cmds.b.progress.range   = 65535;

  cmds.b.colora.cmd       = FT80X_COLOR_RGB(0xff, 0xff, 0xff);

  cmds.b.text.cmd         = FT80X_CMD_TEXT;                   /* Text */
  cmds.b.text.x           = 78;
  cmds.b.text.y           = 68;
  cmds.b.text.font        = 26;
  cmds.b.text.options     = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "3D effect");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  xoffset = 20;
  yoffset = 120;

  cmds.c.colorrgb.cmd     = FT80X_COLOR_RGB(0x00, 0xa0, 0x00);

  cmds.c.bgcolor.cmd      = FT80X_CMD_BGCOLOR;                /* Background color */
  cmds.c.bgcolor.c        = 0x800000;

  ret = ft80x_dl_data(fd, buffer, &cmds.c, sizeof(cmds.c));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  cmds.d.progress.cmd     = FT80X_CMD_PROGRESS;               /* Progress bar */
  cmds.d.progress.x       = xoffset;
  cmds.d.progress.y       = yoffset;
  cmds.d.progress.w       = 150;
  cmds.d.progress.h       = ysz;
  cmds.d.progress.options = 0;
  cmds.d.progress.val     = 10;
  cmds.d.progress.range   = 100;

  cmds.d.bgcolor.cmd      = FT80X_CMD_BGCOLOR;                /* Background color */
  cmds.d.bgcolor.c        = 0x000080;

  ret = ft80x_dl_data(fd, buffer, &cmds.d, sizeof(cmds.d));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  yoffset += ydist;

  cmds.d.progress.cmd     = FT80X_CMD_PROGRESS;               /* Progress bar */
  cmds.d.progress.x       = xoffset;
  cmds.d.progress.y       = yoffset;
  cmds.d.progress.w       = 150;
  cmds.d.progress.h       = ysz;
  cmds.d.progress.options = 0;
  cmds.d.progress.val     = 40;
  cmds.d.progress.range   = 100;

  cmds.d.bgcolor.cmd      = FT80X_CMD_BGCOLOR;                /* Background color */
  cmds.d.bgcolor.c        = 0xffff00;

  ret = ft80x_dl_data(fd, buffer, &cmds.d, sizeof(cmds.d));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  yoffset += ydist;

  cmds.d.progress.cmd     = FT80X_CMD_PROGRESS;               /* Progress bar */
  cmds.d.progress.x       = xoffset;
  cmds.d.progress.y       = yoffset;
  cmds.d.progress.w       = 150;
  cmds.d.progress.h       = ysz;
  cmds.d.progress.options = 0;
  cmds.d.progress.val     = 70;
  cmds.d.progress.range   = 100;

  cmds.d.bgcolor.cmd      = FT80X_CMD_BGCOLOR;                /* Background color */
  cmds.d.bgcolor.c        = 0x808080;

  ret = ft80x_dl_data(fd, buffer, &cmds.d, sizeof(cmds.d));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  yoffset += ydist;

  cmds.e.progress.cmd     = FT80X_CMD_PROGRESS;               /* Progress bar */
  cmds.e.progress.x       = xoffset;
  cmds.e.progress.y       = yoffset;
  cmds.e.progress.w       = 150;
  cmds.e.progress.h       = ysz;
  cmds.e.progress.options = 0;
  cmds.e.progress.val     = 90;
  cmds.e.progress.range   = 100;

  cmds.e.text.cmd         = FT80X_CMD_TEXT;                   /* Text */
  cmds.e.text.x           = xoffset + 180;
  cmds.e.text.y           = 80;
  cmds.e.text.font        = 26;
  cmds.e.text.options     = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.e, sizeof(cmds.e));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "40% TopBottom");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  cmds.progress.cmd     = FT80X_CMD_PROGRESS;               /* Progress bar */
  cmds.progress.x       = xoffset + 180;
  cmds.progress.y       = 100;
  cmds.progress.w       = ysz;
  cmds.progress.h       = 150;
  cmds.progress.options = 0;
  cmds.progress.val     = 40;
  cmds.progress.range   = 100;

  ret = ft80x_dl_data(fd, buffer, &cmds.progress, sizeof(cmds.progress));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  /* Finally, terminate the display list */

  ret = ft80x_dl_end(fd, buffer);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_end failed: %d\n", ret);
    }

  return ret;
}

/****************************************************************************
 * Name: ft80x_coproc_progressbar
 *
 * Description:
 *   Demonstrate the progress bar command
 *
 ****************************************************************************/

int ft80x_coproc_scrollbar(int fd, FAR struct ft80x_dlbuffer_s *buffer)
{
  int16_t xoffset;
  int16_t yoffset;
  int16_t xdist = FT80X_DISPLAY_WIDTH / 12;
  int16_t ydist = FT80X_DISPLAY_WIDTH / 12;
  int16_t wsz;
  int ret;

  /* Formatted output chunks */

  union
  {
    struct
    {
      struct ft80x_cmd32_s         clearrgb;
      struct ft80x_cmd32_s         clear;
      struct ft80x_cmd32_s         colorrgb;
    } a;
    struct
    {
      struct ft80x_cmd_fgcolor_s   fgcolor;
      struct ft80x_cmd_bgcolor_s   bgcolor;
      struct ft80x_cmd_scrollbar_s scrollbar;
      struct ft80x_cmd32_s         colora;
      struct ft80x_cmd_text_s      text;
    } b;
    struct
    {
      struct ft80x_cmd_fgcolor_s   fgcolor;
      struct ft80x_cmd_bgcolor_s   bgcolor;
      struct ft80x_cmd_scrollbar_s scrollbar;
      struct ft80x_cmd32_s         colora;
    } c;
    struct
    {
      struct ft80x_cmd_fgcolor_s   fgcolor;
      struct ft80x_cmd_bgcolor_s   bgcolor;
      struct ft80x_cmd_scrollbar_s scrollbar;
    } d;
    struct
    {
      struct ft80x_cmd_bgcolor_s   bgcolor;
      struct ft80x_cmd_scrollbar_s scrollbar;
      struct ft80x_cmd32_s         colora;
    } e;
    struct
    {
      struct ft80x_cmd_bgcolor_s   bgcolor;
      struct ft80x_cmd_scrollbar_s scrollbar;
    } f;
  } cmds;

  /* Create the hardware display list */

  ret = ft80x_dl_start(fd, buffer, true);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_start failed: %d\n", ret);
      return ret;
    }

  cmds.a.clearrgb.cmd      = FT80X_CLEAR_COLOR_RGB(64, 64, 64);
  cmds.a.clear.cmd         = FT80X_CLEAR(1 ,1, 1);
  cmds.a.colorrgb.cmd      = FT80X_COLOR_RGB(0xff, 0xff, 0xff);

  /* Copy the commands into the display list */

  ret = ft80x_dl_data(fd, buffer, &cmds.a, sizeof(cmds.a));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  /* Draw scroll bar with flat effect */

  cmds.b.fgcolor.cmd       = FT80X_CMD_FGCOLOR;
  cmds.b.fgcolor.c         = 0xffff00;

  cmds.b.bgcolor.cmd       = FT80X_CMD_BGCOLOR;
  cmds.b.bgcolor.c         = 0x404080;

  cmds.b.scrollbar.cmd     = FT80X_CMD_SCROLLBAR;
  cmds.b.scrollbar.x       = 20;
  cmds.b.scrollbar.y       = 10;
  cmds.b.scrollbar.w       = 120;
  cmds.b.scrollbar.h       = 8;
  cmds.b.scrollbar.options = FT80X_OPT_FLAT;
  cmds.b.scrollbar.val     = 20;
  cmds.b.scrollbar.size    = 30;
  cmds.b.scrollbar.range   = 100;

  cmds.b.colora.cmd       = FT80X_COLOR_A(255);

  cmds.b.text.cmd         = FT80X_CMD_TEXT;                   /* Text */
  cmds.b.text.x           = 20;
  cmds.b.text.y           = 40;
  cmds.b.text.font        = 26;
  cmds.b.text.options     = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "Flat effect");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  /* Draw scroll bar with 3d effect */

  cmds.b.fgcolor.cmd       = FT80X_CMD_FGCOLOR;
  cmds.b.fgcolor.c         = 0x00ff00;

  cmds.b.bgcolor.cmd       = FT80X_CMD_BGCOLOR;
  cmds.b.bgcolor.c         = 0x800000;

  cmds.b.scrollbar.cmd     = FT80X_CMD_SCROLLBAR;
  cmds.b.scrollbar.x       = 180;
  cmds.b.scrollbar.y       = 10;
  cmds.b.scrollbar.w       = 120;
  cmds.b.scrollbar.h       = 8;
  cmds.b.scrollbar.options = 0;
  cmds.b.scrollbar.val     = 20;
  cmds.b.scrollbar.size    = 30;
  cmds.b.scrollbar.range   = 100;

  cmds.b.colora.cmd       = FT80X_COLOR_A(255);

  cmds.b.text.cmd         = FT80X_CMD_TEXT;                   /* Text */
  cmds.b.text.x           = 180;
  cmds.b.text.y           = 40;
  cmds.b.text.font        = 26;
  cmds.b.text.options     = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "3D effect");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  /* Draw horizontal scroll bars */

  xoffset = 20;
  yoffset = 120;
  wsz     = FT80X_DISPLAY_WIDTH / 2 - 40;

  cmds.d.fgcolor.cmd       = FT80X_CMD_FGCOLOR;
  cmds.d.fgcolor.c         = 0x00a000;

  cmds.d.bgcolor.cmd       = FT80X_CMD_BGCOLOR;
  cmds.d.bgcolor.c         = 0x800000;

  cmds.d.scrollbar.cmd     = FT80X_CMD_SCROLLBAR;
  cmds.d.scrollbar.x       = xoffset;
  cmds.d.scrollbar.y       = yoffset;
  cmds.d.scrollbar.w       = wsz;
  cmds.d.scrollbar.h       = 8;
  cmds.d.scrollbar.options = 0;
  cmds.d.scrollbar.val     = 10;
  cmds.d.scrollbar.size    = 30;
  cmds.d.scrollbar.range   = 100;

  ret = ft80x_dl_data(fd, buffer, &cmds.d, sizeof(cmds.d));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  yoffset += ydist;

  cmds.c.bgcolor.cmd       = FT80X_CMD_BGCOLOR;
  cmds.c.bgcolor.c         = 0x000080;

  cmds.c.scrollbar.cmd     = FT80X_CMD_SCROLLBAR;
  cmds.c.scrollbar.x       = xoffset;
  cmds.c.scrollbar.y       = yoffset;
  cmds.c.scrollbar.w       = wsz;
  cmds.c.scrollbar.h       = 8;
  cmds.c.scrollbar.options = 0;
  cmds.c.scrollbar.val     = 30;
  cmds.c.scrollbar.size    = 30;
  cmds.c.scrollbar.range   = 100;

  cmds.c.colora.cmd       = FT80X_COLOR_A(255);

  ret = ft80x_dl_data(fd, buffer, &cmds.c, sizeof(cmds.c));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  yoffset += ydist;

  cmds.f.bgcolor.cmd       = FT80X_CMD_BGCOLOR;
  cmds.f.bgcolor.c         = 0xffff00;

  cmds.f.scrollbar.cmd     = FT80X_CMD_SCROLLBAR;
  cmds.f.scrollbar.x       = xoffset;
  cmds.f.scrollbar.y       = yoffset;
  cmds.f.scrollbar.w       = wsz;
  cmds.f.scrollbar.h       = 8;
  cmds.f.scrollbar.options = 0;
  cmds.f.scrollbar.val     = 50;
  cmds.f.scrollbar.size    = 30;
  cmds.f.scrollbar.range   = 100;

  ret = ft80x_dl_data(fd, buffer, &cmds.f, sizeof(cmds.f));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  yoffset += ydist;

  cmds.f.bgcolor.cmd       = FT80X_CMD_BGCOLOR;
  cmds.f.bgcolor.c         = 0x808080;

  cmds.f.scrollbar.cmd     = FT80X_CMD_SCROLLBAR;
  cmds.f.scrollbar.x       = xoffset;
  cmds.f.scrollbar.y       = yoffset;
  cmds.f.scrollbar.w       = wsz;
  cmds.f.scrollbar.h       = 8;
  cmds.f.scrollbar.options = 0;
  cmds.f.scrollbar.val     = 70;
  cmds.f.scrollbar.size    = 30;
  cmds.f.scrollbar.range   = 100;

  ret = ft80x_dl_data(fd, buffer, &cmds.f, sizeof(cmds.f));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  /* Draw vertical scroll bars */

  xoffset = FT80X_DISPLAY_WIDTH / 2 + 40;
  yoffset = 80;
  wsz     = FT80X_DISPLAY_HEIGHT - 100;

  cmds.f.bgcolor.cmd       = FT80X_CMD_BGCOLOR;
  cmds.f.bgcolor.c         = 0x800000;

  cmds.f.scrollbar.cmd     = FT80X_CMD_SCROLLBAR;
  cmds.f.scrollbar.x       = xoffset;
  cmds.f.scrollbar.y       = yoffset;
  cmds.f.scrollbar.w       = 8;
  cmds.f.scrollbar.h       = wsz;
  cmds.f.scrollbar.options = 0;
  cmds.f.scrollbar.val     = 10;
  cmds.f.scrollbar.size    = 30;
  cmds.f.scrollbar.range   = 100;

  ret = ft80x_dl_data(fd, buffer, &cmds.f, sizeof(cmds.f));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  xoffset += xdist;

  cmds.e.bgcolor.cmd       = FT80X_CMD_BGCOLOR;
  cmds.e.bgcolor.c         = 0x000080;

  cmds.e.scrollbar.cmd     = FT80X_CMD_SCROLLBAR;
  cmds.e.scrollbar.x       = xoffset;
  cmds.e.scrollbar.y       = yoffset;
  cmds.e.scrollbar.w       = 8;
  cmds.e.scrollbar.h       = wsz;
  cmds.e.scrollbar.options = 0;
  cmds.e.scrollbar.val     = 30;
  cmds.e.scrollbar.size    = 30;
  cmds.e.scrollbar.range   = 100;

  cmds.e.colora.cmd       = FT80X_COLOR_A(255);

  ret = ft80x_dl_data(fd, buffer, &cmds.e, sizeof(cmds.e));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  xoffset += xdist;

  cmds.f.bgcolor.cmd       = FT80X_CMD_BGCOLOR;
  cmds.f.bgcolor.c         = 0xffff00;

  cmds.f.scrollbar.cmd     = FT80X_CMD_SCROLLBAR;
  cmds.f.scrollbar.x       = xoffset;
  cmds.f.scrollbar.y       = yoffset;
  cmds.f.scrollbar.w       = 8;
  cmds.f.scrollbar.h       = wsz;
  cmds.f.scrollbar.options = 0;
  cmds.f.scrollbar.val     = 50;
  cmds.f.scrollbar.size    = 30;
  cmds.f.scrollbar.range   = 100;

  ret = ft80x_dl_data(fd, buffer, &cmds.f, sizeof(cmds.f));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  xoffset += xdist;

  cmds.f.bgcolor.cmd       = FT80X_CMD_BGCOLOR;
  cmds.f.bgcolor.c         = 0x808080;

  cmds.f.scrollbar.cmd     = FT80X_CMD_SCROLLBAR;
  cmds.f.scrollbar.x       = xoffset;
  cmds.f.scrollbar.y       = yoffset;
  cmds.f.scrollbar.w       = 8;
  cmds.f.scrollbar.h       = wsz;
  cmds.f.scrollbar.options = 0;
  cmds.f.scrollbar.val     = 70;
  cmds.f.scrollbar.size    = 30;
  cmds.f.scrollbar.range   = 100;

  ret = ft80x_dl_data(fd, buffer, &cmds.f, sizeof(cmds.f));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  /* Finally, terminate the display list */

  ret = ft80x_dl_end(fd, buffer);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_end failed: %d\n", ret);
    }

  return ret;
}

/****************************************************************************
 * Name: ft80x_coproc_dial
 *
 * Description:
 *   Demonstrate the dial widget
 *
 ****************************************************************************/

int ft80x_coproc_dial(int fd, FAR struct ft80x_dlbuffer_s *buffer)
{
  int ret;

  /* Formatted output chunks */

  union
  {
    struct
    {
      struct ft80x_cmd32_s         clearrgb;
      struct ft80x_cmd32_s         clear;
      struct ft80x_cmd32_s         colorrgb;
    } a;
    struct
    {
      struct ft80x_cmd_fgcolor_s   fgcolor;
      struct ft80x_cmd_bgcolor_s   bgcolor;
      struct ft80x_cmd_dial_s      dial;
      struct ft80x_cmd32_s         colora;
      struct ft80x_cmd_text_s      text;
    } b;
    struct
    {
      struct ft80x_cmd_fgcolor_s   fgcolor;
      struct ft80x_cmd32_s         colorrgb;
      struct ft80x_cmd_dial_s      dial;
      struct ft80x_cmd_text_s      text;
    } c;
  } cmds;

  /* Create the hardware display list */

  ret = ft80x_dl_start(fd, buffer, true);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_start failed: %d\n", ret);
      return ret;
    }

  cmds.a.clearrgb.cmd      = FT80X_CLEAR_COLOR_RGB(64, 64, 64);
  cmds.a.clear.cmd         = FT80X_CLEAR(1 ,1, 1);
  cmds.a.colorrgb.cmd      = FT80X_COLOR_RGB(0xff, 0xff, 0xff);

  /* Copy the commands into the display list */

  ret = ft80x_dl_data(fd, buffer, &cmds.a, sizeof(cmds.a));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  /* Draw dial with flat effect */

  cmds.b.fgcolor.cmd       = FT80X_CMD_FGCOLOR;
  cmds.b.fgcolor.c         = 0x00ff00;

  cmds.b.bgcolor.cmd       = FT80X_CMD_BGCOLOR;
  cmds.b.bgcolor.c         = 0x000080;

  cmds.b.dial.cmd          = FT80X_CMD_DIAL;                  /* Dial */
  cmds.b.dial.x            = 50;
  cmds.b.dial.y            = 50;
  cmds.b.dial.r            = 40;
  cmds.b.dial.options      = FT80X_OPT_FLAT;
  cmds.b.dial.val          = 0.20 * 65535;                    /* 20% */

  cmds.b.colora.cmd        = FT80X_COLOR_A(255);

  cmds.b.text.cmd          = FT80X_CMD_TEXT;                  /* Text */
  cmds.b.text.x            = 15;
  cmds.b.text.y            = 90;
  cmds.b.text.font         = 26;
  cmds.b.text.options      = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "Flat effect");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  /* Draw dial with 3d effect */

  cmds.b.fgcolor.cmd       = FT80X_CMD_FGCOLOR;
  cmds.b.fgcolor.c         = 0x00ff00;

  cmds.b.bgcolor.cmd       = FT80X_CMD_BGCOLOR;
  cmds.b.bgcolor.c         = 0x800000;

  cmds.b.dial.cmd          = FT80X_CMD_DIAL;                  /* Dial */
  cmds.b.dial.x            = 140;
  cmds.b.dial.y            = 50;
  cmds.b.dial.r            = 40;
  cmds.b.dial.options      = 0;
  cmds.b.dial.val          = 0.45 * 65535;                    /* 45% */

  cmds.b.colora.cmd        = FT80X_COLOR_A(255);

  cmds.b.text.cmd          = FT80X_CMD_TEXT;                  /* Text */
  cmds.b.text.x            = 105;
  cmds.b.text.y            = 90;
  cmds.b.text.font         = 26;
  cmds.b.text.options      = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer,  "3D effect");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  /* Draw increasing dials horizontally */

  cmds.c.fgcolor.cmd       = FT80X_CMD_FGCOLOR;
  cmds.c.fgcolor.c         = 0x800000;

  cmds.c.colorrgb.cmd      = FT80X_COLOR_RGB(41, 1, 5);

  cmds.c.dial.cmd          = FT80X_CMD_DIAL;                  /* Dial */
  cmds.c.dial.x            = 30;
  cmds.c.dial.y            = 160;
  cmds.c.dial.r            = 20;
  cmds.c.dial.options      = 0;
  cmds.c.dial.val          = 0.30 * 65535;                    /* 30% */

  cmds.c.text.cmd          = FT80X_CMD_TEXT;                  /* Text */
  cmds.c.text.x            = 20;
  cmds.c.text.y            = 180;
  cmds.c.text.font         = 26;
  cmds.c.text.options      = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.c, sizeof(cmds.c));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "30%");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  cmds.c.fgcolor.cmd       = FT80X_CMD_FGCOLOR;
  cmds.c.fgcolor.c         = 0x000080;

  cmds.c.colorrgb.cmd      = FT80X_COLOR_RGB(11, 7, 65);

  cmds.c.dial.cmd          = FT80X_CMD_DIAL;                  /* Dial */
  cmds.c.dial.x            = 100;
  cmds.c.dial.y            = 160;
  cmds.c.dial.r            = 40;
  cmds.c.dial.options      = 0;
  cmds.c.dial.val          = 0.45 * 65535;                    /* 45% */

  cmds.c.text.cmd          = FT80X_CMD_TEXT;                  /* Text */
  cmds.c.text.x            = 90;
  cmds.c.text.y            = 200;
  cmds.c.text.font         = 26;
  cmds.c.text.options      = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.c, sizeof(cmds.c));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "45%");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  cmds.c.fgcolor.cmd       = FT80X_CMD_FGCOLOR;
  cmds.c.fgcolor.c         = 0xffff00;

  cmds.c.colorrgb.cmd      = FT80X_COLOR_RGB(87, 94, 9);

  cmds.c.dial.cmd          = FT80X_CMD_DIAL;                  /* Dial */
  cmds.c.dial.x            = 210;
  cmds.c.dial.y            = 160;
  cmds.c.dial.r            = 60;
  cmds.c.dial.options      = 0;
  cmds.c.dial.val          = 0.60 * 65535;                    /* 60% */

  cmds.c.text.cmd          = FT80X_CMD_TEXT;                  /* Text */
  cmds.c.text.x            = 200;
  cmds.c.text.y            = 220;
  cmds.c.text.font         = 26;
  cmds.c.text.options      = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.c, sizeof(cmds.c));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "60%");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

#ifndef CONFIG_LCD_FT80X_QVGA
  cmds.c.fgcolor.cmd       = FT80X_CMD_FGCOLOR;
  cmds.c.fgcolor.c         = 0x808080;

  cmds.c.colorrgb.cmd      = FT80X_COLOR_RGB(51, 50, 52);

  cmds.c.dial.cmd          = FT80X_CMD_DIAL;                  /* Dial */
  cmds.c.dial.x            = 360;
  cmds.c.dial.y            = 160;
  cmds.c.dial.r            = 80;
  cmds.c.dial.options      = 0;
  cmds.c.dial.val          = 0.75 * 65535;                    /* 75% */

  cmds.c.text.cmd          = FT80X_CMD_TEXT;                  /* Text */
  cmds.c.text.x            = 350;
  cmds.c.text.y            = 240;
  cmds.c.text.font         = 26;
  cmds.c.text.options      = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.c, sizeof(cmds.c));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "75%");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }
#endif

  /* Finally, terminate the display list */

  ret = ft80x_dl_end(fd, buffer);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_end failed: %d\n", ret);
    }

  return ret;
}

/****************************************************************************
 * Name: ft80x_coproc_slider
 *
 * Description:
 *   Demonstrate the slider widget
 *
 ****************************************************************************/

int ft80x_coproc_slider(int fd, FAR struct ft80x_dlbuffer_s *buffer)
{
  int16_t xoffset;
  int16_t yoffset;
  int16_t xdist = FT80X_DISPLAY_WIDTH / 12;
  int16_t ydist = FT80X_DISPLAY_WIDTH / 12;
  int16_t wsz;
  int ret;

  /* Formatted output chunks */

  union
  {
    struct
    {
      struct ft80x_cmd32_s         clearrgb;
      struct ft80x_cmd32_s         clear;
      struct ft80x_cmd32_s         colorrgb;
    } a;
    struct
    {
      struct ft80x_cmd_fgcolor_s   fgcolor;
      struct ft80x_cmd_bgcolor_s   bgcolor;
      struct ft80x_cmd_slider_s    slider;
      struct ft80x_cmd32_s         colora;
      struct ft80x_cmd_text_s      text;
    } b;
    struct
    {
      struct ft80x_cmd_fgcolor_s   fgcolor;
      struct ft80x_cmd_bgcolor_s   bgcolor;
      struct ft80x_cmd32_s         colorrgb;
      struct ft80x_cmd_slider_s    slider;
    } c;
    struct
    {
      struct ft80x_cmd_bgcolor_s   bgcolor;
      struct ft80x_cmd32_s         colorrgb;
      struct ft80x_cmd_slider_s    slider;
      struct ft80x_cmd32_s         colora;
    } d;
    struct
    {
      struct ft80x_cmd_bgcolor_s   bgcolor;
      struct ft80x_cmd32_s         colorrgb;
      struct ft80x_cmd_slider_s    slider;
    } e;
    struct
    {
      struct ft80x_cmd_bgcolor_s   bgcolor;
      struct ft80x_cmd_slider_s    slider;
    } f;
  } cmds;

  /* Create the hardware display list */

  ret = ft80x_dl_start(fd, buffer, true);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_start failed: %d\n", ret);
      return ret;
    }

  cmds.a.clearrgb.cmd      = FT80X_CLEAR_COLOR_RGB(64, 64, 64);
  cmds.a.clear.cmd         = FT80X_CLEAR(1 ,1, 1);
  cmds.a.colorrgb.cmd      = FT80X_COLOR_RGB(0xff, 0xff, 0xff);

  /* Draw slider with flat effect */

  cmds.b.fgcolor.cmd       = FT80X_CMD_FGCOLOR;
  cmds.b.fgcolor.c         = 0xffff00;

  cmds.b.bgcolor.cmd       = FT80X_CMD_BGCOLOR;
  cmds.b.bgcolor.c         = 0x000080;

  cmds.b.slider.cmd        = FT80X_CMD_SLIDER;                /* Slider */
  cmds.b.slider.x          = 20;
  cmds.b.slider.y          = 10;
  cmds.b.slider.w          = 120;
  cmds.b.slider.h          = 10;
  cmds.b.slider.options    = FT80X_OPT_FLAT;
  cmds.b.slider.val        = 30;
  cmds.b.slider.range      = 100;

  cmds.b.colora.cmd        = FT80X_COLOR_A(255);

  cmds.b.text.cmd          = FT80X_CMD_TEXT;                  /* Text */
  cmds.b.text.x            = 30;
  cmds.b.text.y            = 40;
  cmds.b.text.font         = 26;
  cmds.b.text.options      = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "Flat effect");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  /* Draw scroll bar with 3d effect */

  cmds.b.fgcolor.cmd       = FT80X_CMD_FGCOLOR;
  cmds.b.fgcolor.c         = 0x00ff00;

  cmds.b.bgcolor.cmd       = FT80X_CMD_BGCOLOR;
  cmds.b.bgcolor.c         = 0x800000;

  cmds.b.slider.cmd        = FT80X_CMD_SLIDER;                /* Slider */
  cmds.b.slider.x          = 180;
  cmds.b.slider.y          = 10;
  cmds.b.slider.w          = 120;
  cmds.b.slider.h          = 10;
  cmds.b.slider.options    = 0;
  cmds.b.slider.val        = 50;
  cmds.b.slider.range      = 100;

  cmds.b.colora.cmd        = FT80X_COLOR_A(255);

  cmds.b.text.cmd          = FT80X_CMD_TEXT;                  /* Text */
  cmds.b.text.x            = 180;
  cmds.b.text.y            = 40;
  cmds.b.text.font         = 26;
  cmds.b.text.options      = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "3D effect");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  /* Draw horizontal slider bars */

  xoffset = 20;
  yoffset = 120;
  wsz     = FT80X_DISPLAY_WIDTH / 2 - 40;

  cmds.c.fgcolor.cmd       = FT80X_CMD_FGCOLOR;
  cmds.c.fgcolor.c         = 0x00a000;

  cmds.c.bgcolor.cmd       = FT80X_CMD_BGCOLOR;
  cmds.c.bgcolor.c         = 0x800000;

  cmds.c.colorrgb.cmd      = FT80X_COLOR_RGB(41, 1, 5);

  cmds.c.slider.cmd        = FT80X_CMD_SLIDER;                /* Slider */
  cmds.c.slider.x          = xoffset;
  cmds.c.slider.y          = yoffset;
  cmds.c.slider.w          = wsz;
  cmds.c.slider.h          = 10;
  cmds.c.slider.options    = 0;
  cmds.c.slider.val        = 10;
  cmds.c.slider.range      = 100;

  ret = ft80x_dl_data(fd, buffer, &cmds.c, sizeof(cmds.c));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  yoffset += ydist;

  cmds.d.bgcolor.cmd       = FT80X_CMD_BGCOLOR;
  cmds.d.bgcolor.c         = 0x000080;

  cmds.d.colorrgb.cmd      = FT80X_COLOR_RGB(11, 7, 65);

  cmds.d.slider.cmd        = FT80X_CMD_SLIDER;                /* Slider */
  cmds.d.slider.x          = xoffset;
  cmds.d.slider.y          = yoffset;
  cmds.d.slider.w          = wsz;
  cmds.d.slider.h          = 10;
  cmds.d.slider.options    = 0;
  cmds.d.slider.val        = 30;
  cmds.d.slider.range      = 100;

  cmds.d.colora.cmd        = FT80X_COLOR_A(255);

  ret = ft80x_dl_data(fd, buffer, &cmds.d, sizeof(cmds.d));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  yoffset += ydist;

  cmds.e.bgcolor.cmd       = FT80X_CMD_BGCOLOR;
  cmds.e.bgcolor.c         = 0xffff00;

  cmds.e.colorrgb.cmd      = FT80X_COLOR_RGB(87, 94, 9);

  cmds.e.slider.cmd        = FT80X_CMD_SLIDER;                /* Slider */
  cmds.e.slider.x          = xoffset;
  cmds.e.slider.y          = yoffset;
  cmds.e.slider.w          = wsz;
  cmds.e.slider.h          = 10;
  cmds.e.slider.options    = 0;
  cmds.e.slider.val        = 50;
  cmds.e.slider.range      = 100;

  ret = ft80x_dl_data(fd, buffer, &cmds.e, sizeof(cmds.e));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  yoffset += ydist;

  cmds.e.bgcolor.cmd       = FT80X_CMD_BGCOLOR;
  cmds.e.bgcolor.c         = 0x808080;

  cmds.e.colorrgb.cmd      = FT80X_COLOR_RGB(51, 50, 52);

  cmds.e.slider.cmd        = FT80X_CMD_SLIDER;                /* Slider */
  cmds.e.slider.x          = xoffset;
  cmds.e.slider.y          = yoffset;
  cmds.e.slider.w          = wsz;
  cmds.e.slider.h          = 10;
  cmds.e.slider.options    = 0;
  cmds.e.slider.val        = 70;
  cmds.e.slider.range      = 100;

  ret = ft80x_dl_data(fd, buffer, &cmds.e, sizeof(cmds.e));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  xoffset = FT80X_DISPLAY_WIDTH / 2 + 40;
  yoffset = 80;
  wsz     = FT80X_DISPLAY_HEIGHT - 100;

  /* Draw vertical slider bars */

  cmds.f.bgcolor.cmd       = FT80X_CMD_BGCOLOR;
  cmds.f.bgcolor.c         = 0x800000;

  cmds.f.slider.cmd        = FT80X_CMD_SLIDER;                /* Slider */
  cmds.f.slider.x          = xoffset;
  cmds.f.slider.y          = yoffset;
  cmds.f.slider.w          = 10;
  cmds.f.slider.h          = wsz;
  cmds.f.slider.options    = 0;
  cmds.f.slider.val        = 10;
  cmds.f.slider.range      = 100;

  ret = ft80x_dl_data(fd, buffer, &cmds.f, sizeof(cmds.f));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  xoffset += xdist;

  cmds.f.bgcolor.cmd       = FT80X_CMD_BGCOLOR;
  cmds.f.bgcolor.c         = 0x000080;

  cmds.f.slider.cmd        = FT80X_CMD_SLIDER;                /* Slider */
  cmds.f.slider.x          = xoffset;
  cmds.f.slider.y          = yoffset;
  cmds.f.slider.w          = 10;
  cmds.f.slider.h          = wsz;
  cmds.f.slider.options    = 0;
  cmds.f.slider.val        = 30;
  cmds.f.slider.range      = 100;

  ret = ft80x_dl_data(fd, buffer, &cmds.f, sizeof(cmds.f));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  xoffset += xdist;

  cmds.f.bgcolor.cmd       = FT80X_CMD_BGCOLOR;
  cmds.f.bgcolor.c         = 0xffff00;

  cmds.f.slider.cmd        = FT80X_CMD_SLIDER;                /* Slider */
  cmds.f.slider.x          = xoffset;
  cmds.f.slider.y          = yoffset;
  cmds.f.slider.w          = 10;
  cmds.f.slider.h          = wsz;
  cmds.f.slider.options    = 0;
  cmds.f.slider.val        = 50;
  cmds.f.slider.range      = 100;

  ret = ft80x_dl_data(fd, buffer, &cmds.f, sizeof(cmds.f));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  xoffset += xdist;

  cmds.f.bgcolor.cmd       = FT80X_CMD_BGCOLOR;
  cmds.f.bgcolor.c         = 0x808080;

  cmds.f.slider.cmd        = FT80X_CMD_SLIDER;                /* Slider */
  cmds.f.slider.x          = xoffset;
  cmds.f.slider.y          = yoffset;
  cmds.f.slider.w          = 10;
  cmds.f.slider.h          = wsz;
  cmds.f.slider.options    = 0;
  cmds.f.slider.val        = 70;
  cmds.f.slider.range      = 100;

  ret = ft80x_dl_data(fd, buffer, &cmds.f, sizeof(cmds.f));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  /* Finally, terminate the display list */

  ret = ft80x_dl_end(fd, buffer);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_end failed: %d\n", ret);
    }

  return ret;
}

/****************************************************************************
 * Name: ft80x_coproc_toggle
 *
 * Description:
 *   Demonstrate the toggle widget
 *
 ****************************************************************************/

int ft80x_coproc_toggle(int fd, FAR struct ft80x_dlbuffer_s *buffer)
{
  int16_t xoffset;
  int16_t yoffset;
  int16_t ydist = 40;
  int ret;

  /* Formatted output chunks */

  union
  {
    struct
    {
      struct ft80x_cmd32_s         clearrgb;
      struct ft80x_cmd32_s         clear;
      struct ft80x_cmd32_s         colorrgb;
    } a;
    struct
    {
      struct ft80x_cmd_fgcolor_s   fgcolor;
      struct ft80x_cmd_bgcolor_s   bgcolor;
      struct ft80x_cmd32_s         colorrgb;
      struct ft80x_cmd_toggle_s    toggle;
    } b;
    struct
    {
      struct ft80x_cmd_fgcolor_s   fgcolor;
      struct ft80x_cmd_bgcolor_s   bgcolor;
      struct ft80x_cmd_toggle_s    toggle;
    } c;
    struct
    {
      struct ft80x_cmd32_s         colora;
      struct ft80x_cmd_text_s      text;
    } d;
  } cmds;

  /* Create the hardware display list */

  ret = ft80x_dl_start(fd, buffer, true);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_start failed: %d\n", ret);
      return ret;
    }

  cmds.a.clearrgb.cmd      = FT80X_CLEAR_COLOR_RGB(64, 64, 64);
  cmds.a.clear.cmd         = FT80X_CLEAR(1 ,1, 1);
  cmds.a.colorrgb.cmd      = FT80X_COLOR_RGB(0xff, 0xff, 0xff);

  /* Copy the commands into the display list */

  ret = ft80x_dl_data(fd, buffer, &cmds.a, sizeof(cmds.a));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  /* Draw toggle with flat effect */

  cmds.b.fgcolor.cmd       = FT80X_CMD_FGCOLOR;
  cmds.b.fgcolor.c         = 0xffff00;

  cmds.b.bgcolor.cmd       = FT80X_CMD_BGCOLOR;
  cmds.b.bgcolor.c         = 0x000080;

  cmds.b.colorrgb.cmd      = FT80X_COLOR_RGB(0xff, 0xff, 0xff);

  cmds.b.toggle.cmd        = FT80X_CMD_TOGGLE;                 /* Toggle */
  cmds.b.toggle.x          = 40;
  cmds.b.toggle.y          = 10;
  cmds.b.toggle.w          = 30;
  cmds.b.toggle.font       = 27;
  cmds.b.toggle.options    = FT80X_OPT_FLAT;
  cmds.b.toggle.state      = 0.5 * 65535;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "no""\xff""yes");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  cmds.d.colora.cmd        = FT80X_COLOR_A(255);               /* Color A */

  cmds.d.text.cmd          = FT80X_CMD_TEXT;                   /* Text */
  cmds.d.text.x            = 40;
  cmds.d.text.y            = 40;
  cmds.d.text.font         = 26;
  cmds.d.text.options      = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.d, sizeof(cmds.d));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "Flat effect");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  /* Draw toggle bar with 3d effect */

  cmds.c.fgcolor.cmd       = FT80X_CMD_FGCOLOR;
  cmds.c.fgcolor.c         = 0x00ff00;

  cmds.c.bgcolor.cmd       = FT80X_CMD_BGCOLOR;
  cmds.c.bgcolor.c         = 0x800000;

  cmds.c.toggle.cmd        = FT80X_CMD_TOGGLE;                 /* Toggle */
  cmds.c.toggle.x          = 40;
  cmds.c.toggle.y          = 10;
  cmds.c.toggle.w          = 30;
  cmds.c.toggle.font       = 27;
  cmds.c.toggle.options    = 0;
  cmds.c.toggle.state      = 1 * 65535;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "stop""\xff""run");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  cmds.d.colora.cmd         = FT80X_COLOR_A(255);               /* Color A */

  cmds.d.text.cmd           = FT80X_CMD_TEXT;                   /* Text */
  cmds.d.text.x             = 140;
  cmds.d.text.y             = 40;
  cmds.d.text.font          = 26;
  cmds.d.text.options       = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.d, sizeof(cmds.d));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer,  "3D effect");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  /* Draw horizontal toggle bars */

  xoffset = 40;
  yoffset = 80;

  cmds.c.fgcolor.cmd       = FT80X_CMD_FGCOLOR;
  cmds.c.fgcolor.c         = 0x800000;

  cmds.c.bgcolor.cmd       = FT80X_CMD_BGCOLOR;
  cmds.c.bgcolor.c         = 0x410105;

  cmds.c.toggle.cmd        = FT80X_CMD_TOGGLE;                 /* Toggle */
  cmds.c.toggle.x          = xoffset;
  cmds.c.toggle.y          = yoffset;
  cmds.c.toggle.w          = 30;
  cmds.c.toggle.font       = 27;
  cmds.c.toggle.options    = 0;
  cmds.c.toggle.state      = 0 * 65535;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "-ve""\xff""+ve");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  yoffset += ydist;

  cmds.c.fgcolor.cmd       = FT80X_CMD_FGCOLOR;
  cmds.c.fgcolor.c         = 0x0b0721;

  cmds.c.bgcolor.cmd       = FT80X_CMD_BGCOLOR;
  cmds.c.bgcolor.c         = 0x000080;

  cmds.c.toggle.cmd        = FT80X_CMD_TOGGLE;                 /* Toggle */
  cmds.c.toggle.x          = xoffset;
  cmds.c.toggle.y          = yoffset;
  cmds.c.toggle.w          = 30;
  cmds.c.toggle.font       = 27;
  cmds.c.toggle.options    = 0;
  cmds.c.toggle.state      = 0.25 * 65535;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "zero""\xff""one");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  yoffset += ydist;

  cmds.b.fgcolor.cmd       = FT80X_CMD_FGCOLOR;
  cmds.b.fgcolor.c         = 0x575e1b;

  cmds.b.bgcolor.cmd       = FT80X_CMD_BGCOLOR;
  cmds.b.bgcolor.c         = 0xffff00;

  cmds.b.colorrgb.cmd      = FT80X_COLOR_RGB(0, 0, 0);

  cmds.b.toggle.cmd        = FT80X_CMD_TOGGLE;                 /* Toggle */
  cmds.b.toggle.x          = xoffset;
  cmds.b.toggle.y          = yoffset;
  cmds.b.toggle.w          = 30;
  cmds.b.toggle.font       = 27;
  cmds.b.toggle.options    = 0;
  cmds.b.toggle.state      = 0.50 * 65535;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "exit""\xff""init");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  yoffset += ydist;

  cmds.b.fgcolor.cmd       = FT80X_CMD_FGCOLOR;
  cmds.b.fgcolor.c         = 0x333234;

  cmds.b.bgcolor.cmd       = FT80X_CMD_BGCOLOR;
  cmds.b.bgcolor.c         = 0x808080;

  cmds.b.colorrgb.cmd      = FT80X_COLOR_RGB(0xff, 0xff, 0xff);

  cmds.b.toggle.cmd        = FT80X_CMD_TOGGLE;                 /* Toggle */
  cmds.b.toggle.x          = xoffset;
  cmds.b.toggle.y          = yoffset;
  cmds.b.toggle.w          = 30;
  cmds.b.toggle.font       = 27;
  cmds.b.toggle.options    = 0;
  cmds.b.toggle.state      = 0.75 * 65535;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "exit""\xff""init");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  /* Finally, terminate the display list */

  ret = ft80x_dl_end(fd, buffer);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_end failed: %d\n", ret);
    }

  return ret;
}

/****************************************************************************
 * Name: ft80x_coproc_calibrate
 *
 * Description:
 *   Demonstrate the calibrate widget
 *
 ****************************************************************************/

int ft80x_coproc_calibrate(int fd, FAR struct ft80x_dlbuffer_s *buffer)
{
  uint32_t matrix[6];
  int ret;

  /* Formatted output chunks */

  union
  {
    struct
    {
      struct ft80x_cmd32_s         clearrgb;
      struct ft80x_cmd32_s         clear;
      struct ft80x_cmd32_s         colorrgb;
      struct ft80x_cmd_text_s      text;
    } a;
    struct ft80x_cmd_calibrate_s   calib;
  } cmds;

  /* Create the hardware display list */

  ret = ft80x_dl_start(fd, buffer, true);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_start failed: %d\n", ret);
      return ret;
    }

  cmds.a.clearrgb.cmd      = FT80X_CLEAR_COLOR_RGB(64, 64, 64);
  cmds.a.clear.cmd         = FT80X_CLEAR(1 ,1, 1);
  cmds.a.colorrgb.cmd      = FT80X_COLOR_RGB(0xff, 0xff, 0xff);

  cmds.a.text.cmd          = FT80X_CMD_TEXT;                   /* Text */
  cmds.a.text.x            = FT80X_DISPLAY_WIDTH /2;
  cmds.a.text.y            = FT80X_DISPLAY_HEIGHT / 2;
  cmds.a.text.font         = 27;
  cmds.a.text.options      = FT80X_OPT_CENTER;

  ret = ft80x_dl_data(fd, buffer, &cmds.a, sizeof(cmds.a));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "Please Tap on the dot");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  cmds.calib.cmd           = FT80X_CMD_CALIBRATE;
  cmds.calib.result        = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.calib, sizeof(cmds.calib));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  /* Terminate the display list */

  ret = ft80x_dl_end(fd, buffer);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_end failed: %d\n", ret);
      return ret;
    }

  /* Get the calibration results */

  ret = ft80x_touch_gettransform(fd, matrix);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_touch_gettransform failed: %d\n", ret);
      return ret;
    }

  ft80x_info("Transform A-F: {%08lx, %08lx, %08lx, %08lx, %08lx, %08lx}\n",
             matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5]);
  return OK;
}
