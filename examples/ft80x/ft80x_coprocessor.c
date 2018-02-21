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
