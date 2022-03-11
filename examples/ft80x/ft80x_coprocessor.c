/****************************************************************************
 * apps/examples/ft80x/ft80x_coprocessor.c
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
 * Derives from FTDI sample code which appears to have an unrestricted
 * license.  Re-released here under the BSD 3-clause license:
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
 * Pre-processor Definitions
 ****************************************************************************/

#define INTERACTIVE_TEXTSIZE (512)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ft80x_coproc_button
 *
 * Description:
 *   Demonstrate the button functionality
 *
 ****************************************************************************/

int ft80x_coproc_button(int fd, FAR struct ft80x_dlbuffer_s *buffer)
{
  int16_t xoffset;
  int16_t yoffset;
  int16_t width;
  int16_t height;
  int16_t xdist;
  int16_t ydist;
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
      struct ft80x_cmd_button_s    button;
    } b;
    struct
    {
      struct ft80x_cmd_fgcolor_s   fgcolor;
      struct ft80x_cmd_gradcolor_s gradcolor;
      struct ft80x_cmd_button_s    button;
    } c;
    struct
    {
      struct ft80x_cmd_gradcolor_s gradcolor;
      struct ft80x_cmd_button_s    button;
    } d;
    struct ft80x_cmd_button_s     button;
    struct ft80x_cmd_text_s       text;
  } cmds;

  /* Create the hardware display list */

  ret = ft80x_dl_start(fd, buffer, true);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_start failed: %d\n", ret);
      return ret;
    }

  cmds.a.clearrgb.cmd      = FT80X_CLEAR_COLOR_RGB(64, 64, 64);
  cmds.a.clear.cmd         = FT80X_CLEAR(1, 1, 1);
  cmds.a.colorrgb.cmd      = FT80X_COLOR_RGB(0xff, 0xff, 0xff);

  /* Copy the commands into the display list */

  ret = ft80x_dl_data(fd, buffer, &cmds.a, sizeof(cmds.a));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  width                    = 60;
  height                   = 30;
  xdist                    = width + (FT80X_DISPLAY_WIDTH - 4 * width) / 5;
  ydist                    = height + 5;
  xoffset                  = 10;
  yoffset                  = FT80X_DISPLAY_HEIGHT / 2 - 2 * ydist;

  /* Construct a buttons with "ONE/TWO/THREE" text and default background */

  /* Draw buttons 60x30 resolution at 10x40,10x75,10x110 */

  /* Flat effect and default color background */

  cmds.b.fgcolor.cmd       = FT80X_CMD_FGCOLOR;
  cmds.b.fgcolor.c         = 0x0000ff;

  cmds.b.button.cmd        = FT80X_CMD_BUTTON;
  cmds.b.button.x          = xoffset;
  cmds.b.button.y          = yoffset;
  cmds.b.button.w          = width;
  cmds.b.button.h          = height;
  cmds.b.button.font       = 28;
  cmds.b.button.options    = FT80X_OPT_FLAT;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "ABC");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  yoffset                 += ydist;

  cmds.button.cmd          = FT80X_CMD_BUTTON;
  cmds.button.x            = xoffset;
  cmds.button.y            = yoffset;
  cmds.button.w            = width;
  cmds.button.h            = height;
  cmds.button.font         = 28;
  cmds.button.options      = FT80X_OPT_FLAT;

  ret = ft80x_dl_data(fd, buffer, &cmds.button, sizeof(cmds.button));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "ABC");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  yoffset                 += ydist;
  cmds.button.y            = yoffset;

  ret = ft80x_dl_data(fd, buffer, &cmds.button, sizeof(cmds.button));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "ABC");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  cmds.text.cmd            = FT80X_CMD_TEXT;
  cmds.text.x              = 20;
  cmds.text.y              = 40;
  cmds.text.font           = 26;
  cmds.text.options        = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.text, sizeof(cmds.text));
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

  /* 3D effect */

  xoffset                 += xdist;
  yoffset                  = FT80X_DISPLAY_HEIGHT / 2 - 2 * ydist;

  cmds.button.cmd          = FT80X_CMD_BUTTON;
  cmds.button.x            = xoffset;
  cmds.button.y            = yoffset;
  cmds.button.w            = width;
  cmds.button.h            = height;
  cmds.button.font         = 28;
  cmds.button.options      = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.button, sizeof(cmds.button));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "ABC");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  yoffset                 += ydist;
  cmds.button.y            = yoffset;

  ret = ft80x_dl_data(fd, buffer, &cmds.button, sizeof(cmds.button));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "ABC");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  yoffset                 += ydist;
  cmds.button.y            = yoffset;

  ret = ft80x_dl_data(fd, buffer, &cmds.button, sizeof(cmds.button));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "ABC");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  yoffset                 += ydist;
  cmds.button.y            = yoffset;

  ret = ft80x_dl_data(fd, buffer, &cmds.button, sizeof(cmds.button));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "ABC");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  cmds.text.cmd            = FT80X_CMD_TEXT;
  cmds.text.x              = xoffset;
  cmds.text.y              = yoffset + 40;
  cmds.text.font           = 26;
  cmds.text.options        = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.text, sizeof(cmds.text));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "3D Effect");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  /* 3d effect with background color */

  xoffset                 += xdist;
  yoffset                  = FT80X_DISPLAY_HEIGHT / 2 - 2 * ydist;

  cmds.b.fgcolor.cmd       = FT80X_CMD_FGCOLOR;
  cmds.b.fgcolor.c         = 0xffff00;

  cmds.b.button.cmd        = FT80X_CMD_BUTTON;
  cmds.b.button.x          = xoffset;
  cmds.b.button.y          = yoffset;
  cmds.b.button.w          = width;
  cmds.b.button.h          = height;
  cmds.b.button.font       = 28;
  cmds.b.button.options    = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "ABC");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  yoffset                 += ydist;
  cmds.b.fgcolor.c         = 0xffff00;
  cmds.b.button.y          = yoffset;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "ABC");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  yoffset                 += ydist;
  cmds.b.fgcolor.c         = 0xff00ff;
  cmds.b.button.y          = yoffset;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "ABC");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  cmds.text.cmd            = FT80X_CMD_TEXT;
  cmds.text.x              = xoffset;
  cmds.text.y              = yoffset + 40;
  cmds.text.font           = 26;
  cmds.text.options        = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.text, sizeof(cmds.text));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "3D Color");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  /* 3d effect with gradient color */

  xoffset                 += xdist;
  yoffset                  = FT80X_DISPLAY_HEIGHT / 2 - 2 * ydist;

  cmds.c.fgcolor.cmd       = FT80X_CMD_FGCOLOR;
  cmds.c.fgcolor.c         = 0x101010;

  cmds.c.gradcolor.cmd     = FT80X_CMD_GRADCOLOR;
  cmds.c.gradcolor.c       = 0xff0000;

  cmds.c.button.cmd        = FT80X_CMD_BUTTON;
  cmds.c.button.x          = xoffset;
  cmds.c.button.y          = yoffset;
  cmds.c.button.w          = width;
  cmds.c.button.h          = height;
  cmds.c.button.font       = 28;
  cmds.c.button.options    = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.c, sizeof(cmds.c));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "ABC");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  yoffset                 += ydist;

  cmds.d.gradcolor.cmd     = FT80X_CMD_GRADCOLOR;
  cmds.d.gradcolor.c       = 0x00ff00;

  cmds.d.button.cmd        = FT80X_CMD_BUTTON;
  cmds.d.button.x          = xoffset;
  cmds.d.button.y          = yoffset;
  cmds.d.button.w          = width;
  cmds.d.button.h          = height;
  cmds.d.button.font       = 28;
  cmds.d.button.options    = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.d, sizeof(cmds.d));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "ABC");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  yoffset                 += ydist;
  cmds.d.gradcolor.c       = 0x0000ff;
  cmds.d.button.y          = yoffset;

  ret = ft80x_dl_data(fd, buffer, &cmds.d, sizeof(cmds.d));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "ABC");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  cmds.text.cmd            = FT80X_CMD_TEXT;
  cmds.text.x              = xoffset;
  cmds.text.y              = yoffset + 40;
  cmds.text.font           = 26;
  cmds.text.options        = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.text, sizeof(cmds.text));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "3D Gradient");
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
 * Name: ft80x_coproc_clock
 *
 * Description:
 *   Demonstrate the clock widget
 *
 ****************************************************************************/

int ft80x_coproc_clock(int fd, FAR struct ft80x_dlbuffer_s *buffer)
{
#ifndef CONFIG_EXAMPLES_FT80X_EXCLUDE_BITMAPS
  FAR const struct ft80x_bitmaphdr_s *bmhdr = &g_lenaface_bmhdr;
#endif
  int16_t xoffset;
  int16_t yoffset;
  int16_t radius;
  int16_t xdist;

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
      struct ft80x_cmd_bgcolor_s   bgcolor;
      struct ft80x_cmd32_s         colorrgb;
      struct ft80x_cmd_clock_s     clock;
      struct ft80x_cmd_text_s      text;
    } b;
    struct
    {
      struct ft80x_cmd_bgcolor_s   bgcolor;
      struct ft80x_cmd32_s         colorrgb;
      struct ft80x_cmd_fgcolor_s   fgcolor;
      struct ft80x_cmd_clock_s     clock;
      struct ft80x_cmd32_s         colora;
      struct ft80x_cmd_text_s      text;
    } c;
    struct
    {
      struct ft80x_cmd32_s         colorrgb;
      struct ft80x_cmd_clock_s     clock;
    } d;
#ifndef CONFIG_EXAMPLES_FT80X_EXCLUDE_BITMAPS
    struct
    {
      struct ft80x_cmd32_s         linewidth;
      struct ft80x_cmd32_s         colorrgb1;
      struct ft80x_cmd32_s         begin1;
      struct ft80x_cmd32_s         vertex2f1;
      struct ft80x_cmd32_s         vertex2f2;
      struct ft80x_cmd32_s         end1;
      struct ft80x_cmd32_s         colora;
      struct ft80x_cmd32_s         colorrgb2;
      struct ft80x_cmd32_s         colormask1;
      struct ft80x_cmd32_s         clear;
      struct ft80x_cmd32_s         begin2;
      struct ft80x_cmd32_s         vertex2f3;
      struct ft80x_cmd32_s         vertex2f4;
      struct ft80x_cmd32_s         end2;
      struct ft80x_cmd32_s         colormask2;
      struct ft80x_cmd32_s         blend;
    } e;
    struct
    {
      struct ft80x_cmd_loadidentity_s loadidentity1;
      struct ft80x_cmd_scale_s     scale;
      struct ft80x_cmd_setmatrix_s setmatrix1;
      struct ft80x_cmd32_s         begin;
      struct ft80x_cmd32_s         bitmapsource;
      struct ft80x_cmd32_s         bitmaplayout;
      struct ft80x_cmd32_s         bitmapsize;
      struct ft80x_cmd32_s         vertex2f;
      struct ft80x_cmd32_s         end;
      struct ft80x_cmd32_s         blend;
      struct ft80x_cmd_loadidentity_s loadidentity2;
      struct ft80x_cmd_setmatrix_s setmatrix2;
      struct ft80x_cmd32_s         colorrgb;
      struct ft80x_cmd_clock_s     clock;
    } f;
#endif
  } cmds;

  xdist  = FT80X_DISPLAY_WIDTH / 5;
  radius = xdist / 2 - FT80X_DISPLAY_WIDTH / 64;

#ifndef CONFIG_EXAMPLES_FT80X_EXCLUDE_BITMAPS
  /* Copy the image into graphics ram for the Lena faced clock */

  ret = ft80x_ramg_write(fd, 0, bmhdr->data, bmhdr->stride * bmhdr->height);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_ramg_write() failed: %d\n", ret);
      return ret;
    }
#endif

  /* Create the hardware display list */

  ret = ft80x_dl_start(fd, buffer, true);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_start failed: %d\n", ret);
      return ret;
    }

  cmds.a.clearrgb.cmd     = FT80X_CLEAR_COLOR_RGB(64, 64, 64);
  cmds.a.clear.cmd        = FT80X_CLEAR(1, 1, 1);
  cmds.a.colorrgb.cmd     = FT80X_COLOR_RGB(0xff, 0xff, 0xff);

  /* Copy the commands into the display list */

  ret = ft80x_dl_data(fd, buffer, &cmds.a, sizeof(cmds.a));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  /* Draw clock with blue as background and read as needle color */

  /* Flat effect and default color background */

  xoffset                 = xdist / 2;
  yoffset                 = radius + 5;

  cmds.b.bgcolor.cmd      = FT80X_CMD_BGCOLOR;                /* Background color */
  cmds.b.bgcolor.c        = 0x0000ff;

  cmds.b.colorrgb.cmd     = FT80X_COLOR_RGB(0xff, 0x00, 0x00);

  cmds.b.clock.cmd        = FT80X_CMD_CLOCK;                  /* Clock */
  cmds.b.clock.x          = xoffset;
  cmds.b.clock.y          = yoffset;
  cmds.b.clock.r          = radius;
  cmds.b.clock.options    = FT80X_OPT_FLAT;
  cmds.b.clock.h          = 30;
  cmds.b.clock.m          = 100;
  cmds.b.clock.s          = 5;
  cmds.b.clock.ms         = 10;

  cmds.b.text.cmd         = FT80X_CMD_TEXT;                   /* Text */
  cmds.b.text.x           = xoffset;
  cmds.b.text.y           = yoffset + radius + 6;
  cmds.b.text.font        = 26;
  cmds.b.text.options     = FT80X_OPT_CENTER;

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

  /* No seconds needle */

  xoffset                += xdist;

  cmds.c.bgcolor.cmd      = FT80X_CMD_BGCOLOR;                /* Background color */
  cmds.c.bgcolor.c        = 0x00ff00;

  cmds.c.colorrgb.cmd     = FT80X_COLOR_RGB(0xff, 0x00, 0x00);

  cmds.c.fgcolor.cmd      = FT80X_CMD_FGCOLOR;
  cmds.c.fgcolor.c        = 0xff0000;

  cmds.c.clock.cmd        = FT80X_CMD_CLOCK;                  /* Clock */
  cmds.c.clock.x          = xoffset;
  cmds.c.clock.y          = yoffset;
  cmds.c.clock.r          = radius;
  cmds.c.clock.options    = FT80X_OPT_NOSECS;
  cmds.c.clock.h          = 10;
  cmds.c.clock.m          = 10;
  cmds.c.clock.s          = 5;
  cmds.c.clock.ms         = 10;

  cmds.c.colora.cmd       = FT80X_COLOR_A(255);               /* Color A */

  cmds.c.text.cmd         = FT80X_CMD_TEXT;                   /* Text */
  cmds.c.text.x           = xoffset;
  cmds.c.text.y           = yoffset + radius + 6;
  cmds.c.text.font        = 26;
  cmds.c.text.options     = FT80X_OPT_CENTER;

  ret = ft80x_dl_data(fd, buffer, &cmds.c, sizeof(cmds.c));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "No Secs");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  /* No background color */

  xoffset                += xdist;

  cmds.b.bgcolor.cmd      = FT80X_CMD_BGCOLOR;                /* Background color */
  cmds.b.bgcolor.c        = 0x00ffff;

  cmds.b.colorrgb.cmd     = FT80X_COLOR_RGB(0xff, 0xff, 0x00);

  cmds.b.clock.cmd        = FT80X_CMD_CLOCK;                  /* Clock */
  cmds.b.clock.x          = xoffset;
  cmds.b.clock.y          = yoffset;
  cmds.b.clock.r          = radius;
  cmds.b.clock.options    = FT80X_OPT_NOBACK;
  cmds.b.clock.h          = 10;
  cmds.b.clock.m          = 10;
  cmds.b.clock.s          = 5;
  cmds.b.clock.ms         = 10;

  cmds.b.text.cmd         = FT80X_CMD_TEXT;                   /* Text */
  cmds.b.text.x           = xoffset;
  cmds.b.text.y           = yoffset + radius + 6;
  cmds.b.text.font        = 26;
  cmds.b.text.options     = FT80X_OPT_CENTER;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "No BG");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  /* No ticks */

  xoffset                += xdist;

  cmds.b.bgcolor.c        = 0xff00ff;
  cmds.b.colorrgb.cmd     = FT80X_COLOR_RGB(0x00, 0xff, 0xff);
  cmds.b.clock.x          = xoffset;
  cmds.b.clock.options    = FT80X_OPT_NOTICKS;
  cmds.b.text.x           = xoffset;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "No Ticks");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  /* No hands */

  xoffset                += xdist;

  cmds.b.bgcolor.c        = 0x808080;
  cmds.b.colorrgb.cmd     = FT80X_COLOR_RGB(0x00, 0xff, 0x00);
  cmds.b.clock.x          = xoffset;
  cmds.b.clock.options    = FT80X_OPT_NOHANDS;
  cmds.b.text.x           = xoffset;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "No Hands");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  /* Bigger clock */

  yoffset                += radius + 10;
  radius                  = FT80X_DISPLAY_HEIGHT - (2 * radius + 5 + 10);
  radius                  = (radius - 5 - 20) / 2;
  xoffset                 = radius + 10;
  yoffset                += radius + 5;

  cmds.d.colorrgb.cmd     = FT80X_COLOR_RGB(0x00, 0x00, 0xff);

  cmds.d.clock.cmd        = FT80X_CMD_CLOCK;                  /* Clock */
  cmds.d.clock.x          = xoffset;
  cmds.d.clock.y          = yoffset;
  cmds.d.clock.r          = radius;
  cmds.d.clock.options    = 0;
  cmds.d.clock.h          = 10;
  cmds.d.clock.m          = 10;
  cmds.d.clock.s          = 5;
  cmds.d.clock.ms         = 10;

  ret = ft80x_dl_data(fd, buffer, &cmds.d, sizeof(cmds.d));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

#ifndef CONFIG_EXAMPLES_FT80X_EXCLUDE_BITMAPS
  /* Lena clock with no background and no ticks */

  xoffset                += 2 * radius + 10;

  cmds.e.linewidth.cmd    = FT80X_LINE_WIDTH(10 * 16);
  cmds.e.colorrgb1.cmd    = FT80X_COLOR_RGB(0xff, 0xff, 0xff);
  cmds.e.begin1.cmd       = FT80X_BEGIN(FT80X_PRIM_RECTS);
  cmds.e.vertex2f1.cmd    = FT80X_VERTEX2F((xoffset - radius + 10) * 16,
                                           (yoffset - radius + 10) * 16);
  cmds.e.vertex2f2.cmd    = FT80X_VERTEX2F((xoffset + radius - 10) * 16,
                                           (yoffset + radius - 10) * 16);
  cmds.e.end1.cmd         = FT80X_END();
  cmds.e.colora.cmd       = FT80X_COLOR_A(0xff);
  cmds.e.colorrgb2.cmd    = FT80X_COLOR_RGB(0xff, 0xff, 0xff);
  cmds.e.colormask1.cmd   = FT80X_COLOR_MASK(0, 0, 0, 1);
  cmds.e.clear.cmd        = FT80X_CLEAR(1, 1, 1);
  cmds.e.begin2.cmd       = FT80X_BEGIN(FT80X_PRIM_RECTS);
  cmds.e.vertex2f3.cmd    = FT80X_VERTEX2F((xoffset - radius + 12) * 16,
                                           (yoffset - radius + 12) * 16);
  cmds.e.vertex2f4.cmd   = FT80X_VERTEX2F((xoffset + radius - 12) * 16,
                                           (yoffset + radius - 12) * 16);
  cmds.e.end2.cmd        = FT80X_END();
  cmds.e.colormask2.cmd  = FT80X_COLOR_MASK(1, 1, 1, 1);
  cmds.e.blend.cmd       = FT80X_BLEND_FUNC(FT80X_BLEND_DST_ALPHA,
                                            FT80X_BLEND_ONE_MINUS_DST_ALPHA);

  ret = ft80x_dl_data(fd, buffer, &cmds.e, sizeof(cmds.e));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  /* Lena bitmap - scale proportionately wrt output resolution */

  cmds.f.loadidentity1.cmd = FT80X_CMD_LOADIDENTITY;

  cmds.f.scale.cmd         = FT80X_CMD_SCALE;
  cmds.f.scale.sx          = (65536 * 2 * radius) / bmhdr->width;
  cmds.f.scale.sy          = (65536 * 2 * radius) / bmhdr->height;

  cmds.f.setmatrix1.cmd    = FT80X_CMD_SETMATRIX;

  cmds.f.begin.cmd         = FT80X_BEGIN(FT80X_PRIM_BITMAPS);
  cmds.f.bitmapsource.cmd  = FT80X_BITMAP_SOURCE(0);
  cmds.f.bitmaplayout.cmd  = FT80X_BITMAP_LAYOUT(bmhdr->format,
                                                 bmhdr->stride,
                                                 bmhdr->height);
  cmds.f.bitmapsize.cmd    = FT80X_BITMAP_SIZE(FT80X_FILTER_BILINEAR,
                                               FT80X_WRAP_BORDER,
                                               FT80X_WRAP_BORDER,
                                               2 * radius, 2 * radius);
  cmds.f.vertex2f.cmd      = FT80X_VERTEX2F((xoffset - radius) * 16,
                                            (yoffset - radius) * 16);
  cmds.f.end.cmd           = FT80X_END();
  cmds.f.blend.cmd         = FT80X_BLEND_FUNC(FT80X_BLEND_SRC_ALPHA,
                                        FT80X_BLEND_ONE_MINUS_SRC_ALPHA);
  cmds.f.loadidentity2.cmd = FT80X_CMD_LOADIDENTITY;
  cmds.f.setmatrix2.cmd    = FT80X_CMD_SETMATRIX;
  cmds.f.colorrgb.cmd      = FT80X_COLOR_RGB(0xff, 0xff, 0xff);

  cmds.f.clock.cmd         = FT80X_CMD_CLOCK;                  /* Clock */
  cmds.f.clock.x           = xoffset;
  cmds.f.clock.y           = yoffset;
  cmds.f.clock.r           = radius;
  cmds.f.clock.options     = FT80X_OPT_NOTICKS | FT80X_OPT_NOBACK;
  cmds.f.clock.h           = 10;
  cmds.f.clock.m           = 10;
  cmds.f.clock.s           = 5;
  cmds.f.clock.ms          = 10;

  ret = ft80x_dl_data(fd, buffer, &cmds.f, sizeof(cmds.f));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
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
 * Name: ft80x_coproc_gauge
 *
 * Description:
 *   Demonstrate the gauge widget
 *
 ****************************************************************************/

int ft80x_coproc_gauge(int fd, FAR struct ft80x_dlbuffer_s *buffer)
{
#ifndef CONFIG_EXAMPLES_FT80X_EXCLUDE_BITMAPS
  FAR const struct ft80x_bitmaphdr_s *bmhdr = &g_lenaface_bmhdr;
#endif
  int16_t xoffset;
  int16_t yoffset;
  int16_t radius;
  int16_t xdist;
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
      struct ft80x_cmd_bgcolor_s   bgcolor;
      struct ft80x_cmd32_s         colorrgb;
      struct ft80x_cmd_gauge_s     gauge;
      struct ft80x_cmd_text_s      text;
    } b;
    struct
    {
      struct ft80x_cmd_bgcolor_s   bgcolor;
      struct ft80x_cmd32_s         colorrgb;
      struct ft80x_cmd_fgcolor_s   fgcolor;
      struct ft80x_cmd_gauge_s     gauge;
      struct ft80x_cmd_text_s      text;
    } c;
    struct
    {
      struct ft80x_cmd_bgcolor_s   bgcolor;
      struct ft80x_cmd32_s         colorrgb;
      struct ft80x_cmd_gauge_s     gauge;
      struct ft80x_cmd32_s         colora;
      struct ft80x_cmd_text_s      text;
    } d;
    struct
    {
      struct ft80x_cmd_bgcolor_s   bgcolor;
      struct ft80x_cmd32_s         colorrgb1;
      struct ft80x_cmd_gauge_s     gauge1;
      struct ft80x_cmd32_s         colorrgb2;
      struct ft80x_cmd_gauge_s     gauge2;
    } e;
#ifndef CONFIG_EXAMPLES_FT80X_EXCLUDE_BITMAPS
    struct
    {
      struct ft80x_cmd32_s         pointsize1;
      struct ft80x_cmd32_s         colorrgb1;
      struct ft80x_cmd32_s         begin1;
      struct ft80x_cmd32_s         vertex2f1;
      struct ft80x_cmd32_s         end1;
      struct ft80x_cmd32_s         colora;
      struct ft80x_cmd32_s         colorrgb2;
      struct ft80x_cmd32_s         colormask1;
      struct ft80x_cmd32_s         clear;
      struct ft80x_cmd32_s         begin2;
      struct ft80x_cmd32_s         pointsize2;
      struct ft80x_cmd32_s         vertex2f2;
      struct ft80x_cmd32_s         end2;
      struct ft80x_cmd32_s         colormask2;
      struct ft80x_cmd32_s         blend;
    } f;
    struct
    {
      struct ft80x_cmd_loadidentity_s loadidentity1;
      struct ft80x_cmd_scale_s     scale;
      struct ft80x_cmd_setmatrix_s setmatrix1;
      struct ft80x_cmd32_s         begin;
      struct ft80x_cmd32_s         bitmapsource;
      struct ft80x_cmd32_s         bitmaplayout;
      struct ft80x_cmd32_s         bitmapsize;
      struct ft80x_cmd32_s         vertex2f;
      struct ft80x_cmd32_s         end;
      struct ft80x_cmd32_s         blend;
      struct ft80x_cmd_loadidentity_s loadidentity2;
      struct ft80x_cmd_setmatrix_s setmatrix2;
      struct ft80x_cmd32_s         colorrgb;
      struct ft80x_cmd_gauge_s     gauge;
    } g;
#endif
  } cmds;

  xdist  = FT80X_DISPLAY_WIDTH / 5;
  radius = xdist / 2 - FT80X_DISPLAY_WIDTH / 64;

#ifndef CONFIG_EXAMPLES_FT80X_EXCLUDE_BITMAPS
  /* Copy the image into graphics ram for the Lena faced gauge */

  ret = ft80x_ramg_write(fd, 0, bmhdr->data, bmhdr->stride * bmhdr->height);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_ramg_write() failed: %d\n", ret);
      return ret;
    }
#endif

  /* Create the hardware display list */

  ret = ft80x_dl_start(fd, buffer, true);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_start failed: %d\n", ret);
      return ret;
    }

  cmds.a.clearrgb.cmd     = FT80X_CLEAR_COLOR_RGB(64, 64, 64);
  cmds.a.clear.cmd        = FT80X_CLEAR(1, 1, 1);
  cmds.a.colorrgb.cmd     = FT80X_COLOR_RGB(0xff, 0xff, 0xff);

  /* Copy the commands into the display list */

  ret = ft80x_dl_data(fd, buffer, &cmds.a, sizeof(cmds.a));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  /* Draw gauge with blue as background and read as needle color */

  /* Flat effect and default color background */

  xoffset                 = xdist / 2;
  yoffset                 = radius + 5;

  cmds.b.bgcolor.cmd      = FT80X_CMD_BGCOLOR;                /* Background color */
  cmds.b.bgcolor.c        = 0x0000ff;

  cmds.b.colorrgb.cmd     = FT80X_COLOR_RGB(0xff, 0x00, 0x00);

  cmds.b.gauge.cmd        = FT80X_CMD_GAUGE;                  /* Gauge */
  cmds.b.gauge.x          = xoffset;
  cmds.b.gauge.y          = yoffset;
  cmds.b.gauge.r          = radius;
  cmds.b.gauge.options    = FT80X_OPT_FLAT;
  cmds.b.gauge.major      = 5;
  cmds.b.gauge.minor      = 4;
  cmds.b.gauge.val        = 45;
  cmds.b.gauge.range      = 100;

  cmds.b.text.cmd         = FT80X_CMD_TEXT;                   /* Text */
  cmds.b.text.x           = xoffset;
  cmds.b.text.y           = yoffset + radius + 6;
  cmds.b.text.font        = 26;
  cmds.b.text.options     = FT80X_OPT_CENTER;

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

  /* 3d effect */

  xoffset                += xdist;

  cmds.c.bgcolor.cmd      = FT80X_CMD_BGCOLOR;                /* Background color */
  cmds.c.bgcolor.c        = 0x0000ff;

  cmds.c.colorrgb.cmd     = FT80X_COLOR_RGB(0xff, 0x00, 0x00);

  cmds.c.fgcolor.cmd      = FT80X_CMD_FGCOLOR;                /* Foreground color */
  cmds.c.fgcolor.c        = 0xff0000;

  cmds.c.gauge.cmd        = FT80X_CMD_GAUGE;                  /* Gauge */
  cmds.c.gauge.x          = xoffset;
  cmds.c.gauge.y          = yoffset;
  cmds.c.gauge.r          = radius;
  cmds.c.gauge.options    = 0;
  cmds.c.gauge.major      = 5;
  cmds.c.gauge.minor      = 1;
  cmds.c.gauge.val        = 60;
  cmds.c.gauge.range      = 100;

  cmds.c.text.cmd         = FT80X_CMD_TEXT;                   /* Text */
  cmds.c.text.x           = xoffset;
  cmds.c.text.y           = yoffset + radius + 6;
  cmds.c.text.font        = 26;
  cmds.c.text.options     = FT80X_OPT_CENTER;

  ret = ft80x_dl_data(fd, buffer, &cmds.c, sizeof(cmds.c));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "3d effect");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  /* No background color */

  xoffset                += xdist;

  cmds.d.bgcolor.cmd      = FT80X_CMD_BGCOLOR;                /* Background color */
  cmds.d.bgcolor.c        = 0x00ffff;

  cmds.d.colorrgb.cmd     = FT80X_COLOR_RGB(0xff, 0xff, 0x00);

  cmds.d.gauge.cmd        = FT80X_CMD_GAUGE;                  /* Gauge */
  cmds.d.gauge.x          = xoffset;
  cmds.d.gauge.y          = yoffset;
  cmds.d.gauge.r          = radius;
  cmds.d.gauge.options    = FT80X_OPT_NOBACK;
  cmds.d.gauge.major      = 1;
  cmds.d.gauge.minor      = 6;
  cmds.d.gauge.val        = 90;
  cmds.d.gauge.range      = 100;

  cmds.d.colora.cmd       = FT80X_COLOR_A(255);

  cmds.d.text.cmd         = FT80X_CMD_TEXT;                   /* Text */
  cmds.d.text.x           = xoffset;
  cmds.d.text.y           = yoffset + radius + 6;
  cmds.d.text.font        = 26;
  cmds.d.text.options     = FT80X_OPT_CENTER;

  ret = ft80x_dl_data(fd, buffer, &cmds.d, sizeof(cmds.d));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "No BG");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  /* No ticks */

  xoffset += xdist;

  cmds.b.bgcolor.cmd      = FT80X_CMD_BGCOLOR;                /* Background color */
  cmds.b.bgcolor.c        = 0xff00ff;

  cmds.b.colorrgb.cmd     = FT80X_COLOR_RGB(0x00, 0xff, 0xff);

  cmds.b.gauge.cmd        = FT80X_CMD_GAUGE;                  /* Gauge */
  cmds.b.gauge.x          = xoffset;
  cmds.b.gauge.y          = yoffset;
  cmds.b.gauge.r          = radius;
  cmds.b.gauge.options    = FT80X_OPT_NOTICKS;
  cmds.b.gauge.major      = 5;
  cmds.b.gauge.minor      = 4;
  cmds.b.gauge.val        = 20;
  cmds.b.gauge.range      = 100;

  cmds.b.text.cmd         = FT80X_CMD_TEXT;                   /* Text */
  cmds.b.text.x           = xoffset;
  cmds.b.text.y           = yoffset + radius + 6;
  cmds.b.text.font        = 26;
  cmds.b.text.options     = FT80X_OPT_CENTER;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer,  "No Ticks");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  /* No hands */

  xoffset                += xdist;

  cmds.d.bgcolor.cmd      = FT80X_CMD_BGCOLOR;                /* Background color */
  cmds.d.bgcolor.c        = 0x808080;

  cmds.d.colorrgb.cmd     = FT80X_COLOR_RGB(0x00, 0xff, 0x00);

  cmds.d.gauge.cmd        = FT80X_CMD_GAUGE;                  /* Gauge */
  cmds.d.gauge.x          = xoffset;
  cmds.d.gauge.y          = yoffset;
  cmds.d.gauge.r          = radius;
  cmds.d.gauge.options    = FT80X_OPT_NOHANDS;
  cmds.d.gauge.major      = 5;
  cmds.d.gauge.minor      = 4;
  cmds.d.gauge.val        = 55;
  cmds.d.gauge.range      = 100;

  cmds.d.colora.cmd       = FT80X_COLOR_A(255);

  cmds.d.text.cmd         = FT80X_CMD_TEXT;                   /* Text */
  cmds.d.text.x           = xoffset;
  cmds.d.text.y           = yoffset + radius + 6;
  cmds.d.text.font        = 26;
  cmds.d.text.options     = FT80X_OPT_CENTER;

  ret = ft80x_dl_data(fd, buffer, &cmds.d, sizeof(cmds.d));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "No Hands");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  /* Bigger gauge */

  yoffset                += radius + 10;
  radius                  = FT80X_DISPLAY_HEIGHT - (2 *radius + 5 + 10);
  radius                  = (radius - 5 - 20) / 2;
  xoffset                 = radius + 10;
  yoffset                += radius + 5;

  cmds.e.bgcolor.cmd      = FT80X_CMD_BGCOLOR;                /* Background color */
  cmds.e.bgcolor.c        = 0x808000;

  cmds.e.colorrgb1.cmd    = FT80X_COLOR_RGB(0xff, 0xff, 0xff);

  cmds.e.gauge1.cmd        = FT80X_CMD_GAUGE;                  /* Gauge */
  cmds.e.gauge1.x          = xoffset;
  cmds.e.gauge1.y          = yoffset;
  cmds.e.gauge1.r          = radius;
  cmds.e.gauge1.options    = FT80X_OPT_NOPOINTER;
  cmds.e.gauge1.major      = 5;
  cmds.e.gauge1.minor      = 4;
  cmds.e.gauge1.val        = 80;
  cmds.e.gauge1.range      = 100;

  cmds.e.colorrgb2.cmd    = FT80X_COLOR_RGB(0xff, 0x00, 0x00);

  cmds.e.gauge2.cmd        = FT80X_CMD_GAUGE;                  /* Gauge */
  cmds.e.gauge2.x          = xoffset;
  cmds.e.gauge2.y          = yoffset;
  cmds.e.gauge2.r          = radius;
  cmds.e.gauge2.options    = FT80X_OPT_NOTICKS;
  cmds.e.gauge2.major      = 5;
  cmds.e.gauge2.minor      = 4;
  cmds.e.gauge2.val        = 30;
  cmds.e.gauge2.range      = 100;

  ret = ft80x_dl_data(fd, buffer, &cmds.e, sizeof(cmds.e));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

#ifndef CONFIG_EXAMPLES_FT80X_EXCLUDE_BITMAPS
  /* Lena gauge with no background and no ticks */

  xoffset                 += 2 * radius + 10;

  cmds.f.pointsize1.cmd    = FT80X_POINT_SIZE(radius * 16);
  cmds.f.colorrgb1.cmd     = FT80X_COLOR_RGB(0xff, 0xff, 0xff);
  cmds.f.begin1.cmd        = FT80X_BEGIN(FT80X_PRIM_POINTS);
  cmds.f.vertex2f1.cmd     = FT80X_VERTEX2F(xoffset * 16, yoffset * 16);
  cmds.f.end1.cmd          = FT80X_END();
  cmds.f.colora.cmd        = FT80X_COLOR_A(0xff);
  cmds.f.colorrgb2.cmd     = FT80X_COLOR_RGB(0xff, 0xff, 0xff);
  cmds.f.colormask1.cmd    = FT80X_COLOR_MASK(0, 0, 0, 1);
  cmds.f.clear.cmd         = FT80X_CLEAR(1, 1, 1);
  cmds.f.begin2.cmd        = FT80X_BEGIN(FT80X_PRIM_POINTS);
  cmds.f.pointsize2.cmd    = FT80X_POINT_SIZE((radius - 2) * 16);
  cmds.f.vertex2f2.cmd     = FT80X_VERTEX2F(xoffset * 16, yoffset * 16);
  cmds.f.end2.cmd          = FT80X_END();
  cmds.f.colormask2.cmd    = FT80X_COLOR_MASK(1, 1, 1, 1);
  cmds.f.blend.cmd         = FT80X_BLEND_FUNC(FT80X_BLEND_DST_ALPHA,
                                        FT80X_BLEND_ONE_MINUS_DST_ALPHA);

  ret = ft80x_dl_data(fd, buffer, &cmds.f, sizeof(cmds.f));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  /* Lena bitmap - scale proportionately wrt output resolution */

  cmds.g.loadidentity1.cmd = FT80X_CMD_LOADIDENTITY;

  cmds.g.scale.cmd         = FT80X_CMD_SCALE;
  cmds.g.scale.sx          = (65536 * 2 * radius) / bmhdr->width;
  cmds.g.scale.sy          = (65536 * 2 * radius) / bmhdr->height;

  cmds.g.setmatrix1.cmd    = FT80X_CMD_SETMATRIX;

  cmds.g.begin.cmd         = FT80X_BEGIN(FT80X_PRIM_BITMAPS);
  cmds.g.bitmapsource.cmd  = FT80X_BITMAP_SOURCE(0);
  cmds.g.bitmaplayout.cmd  = FT80X_BITMAP_LAYOUT(bmhdr->format,
                                                 bmhdr->stride,
                                                 bmhdr->height);
  cmds.g.bitmapsize.cmd    = FT80X_BITMAP_SIZE(FT80X_FILTER_BILINEAR,
                                               FT80X_WRAP_BORDER,
                                               FT80X_WRAP_BORDER,
                                               2 * radius, 2 * radius);
  cmds.g.vertex2f.cmd      = FT80X_VERTEX2F((xoffset - radius) * 16,
                                            (yoffset - radius) * 16);
  cmds.g.end.cmd           = FT80X_END();
  cmds.g.blend.cmd         = FT80X_BLEND_FUNC(FT80X_BLEND_SRC_ALPHA,
                                        FT80X_BLEND_ONE_MINUS_SRC_ALPHA);
  cmds.g.loadidentity2.cmd = FT80X_CMD_LOADIDENTITY;
  cmds.g.setmatrix2.cmd    = FT80X_CMD_SETMATRIX;
  cmds.g.colorrgb.cmd      = FT80X_COLOR_RGB(0xff, 0xff, 0xff);

  cmds.g.gauge.cmd         = FT80X_CMD_GAUGE;                  /* Gauge */
  cmds.g.gauge.x           = xoffset;
  cmds.g.gauge.y           = yoffset;
  cmds.g.gauge.r           = radius;
  cmds.g.gauge.options     = FT80X_OPT_NOTICKS | FT80X_OPT_NOBACK;
  cmds.g.gauge.major       = 5;
  cmds.g.gauge.minor       = 4;
  cmds.g.gauge.val         = 30;
  cmds.g.gauge.range       = 100;

  ret = ft80x_dl_data(fd, buffer, &cmds.g, sizeof(cmds.g));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
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
 * Name: ft80x_coproc_keys
 *
 * Description:
 *   Demonstrate the keys widget
 *
 ****************************************************************************/

int ft80x_coproc_keys(int fd, FAR struct ft80x_dlbuffer_s *buffer)
{
  int16_t font;
  int16_t width;
  int16_t height;
  int16_t ydist;
  int16_t yoffset;
  int16_t xoffset;
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
      struct ft80x_cmd_keys_s      keys;
    } b;
    struct
    {
      struct ft80x_cmd_fgcolor_s   fgcolor;
      struct ft80x_cmd_gradcolor_s gradcolor;
      struct ft80x_cmd_keys_s      keys;
    } c;
    struct
    {
      struct ft80x_cmd_gradcolor_s gradcolor;
      struct ft80x_cmd_keys_s      keys;
    } d;
    struct
    {
      struct ft80x_cmd32_s         colora;
      struct ft80x_cmd_keys_s      keys;
    } e;
    struct ft80x_cmd_keys_s        keys;
    struct ft80x_cmd_button_s      button;
    struct ft80x_cmd_text_s        text;
  } cmds;

#ifdef CONFIG_LCD_FT80X_QVGA
  font                   = 27;
  width                  = 22;
  height                 = 22;
  ydist                  = 3;
#else
  font                   = 29;
  width                  = 30;
  height                 = 30;
  ydist                  = 5;
#endif

  /* Create the hardware display list */

  ret = ft80x_dl_start(fd, buffer, true);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_start failed: %d\n", ret);
      return ret;
    }

  cmds.a.clearrgb.cmd    = FT80X_CLEAR_COLOR_RGB(64, 64, 64);
  cmds.a.clear.cmd       = FT80X_CLEAR(1, 1, 1);
  cmds.a.colorrgb.cmd    = FT80X_COLOR_RGB(0xff, 0xff, 0xff);

  /* Copy the commands into the display list */

  ret = ft80x_dl_data(fd, buffer, &cmds.a, sizeof(cmds.a));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  /* Draw keys with flat effect */

  cmds.b.fgcolor.cmd     = FT80X_CMD_FGCOLOR;               /* Foreground color */
  cmds.b.fgcolor.c       = 0x404080;

  cmds.b.keys.cmd        = FT80X_CMD_KEYS;                  /* Keys */
  cmds.b.keys.x          = 10;
  cmds.b.keys.y          = 10;
  cmds.b.keys.w          = 4 * width;
  cmds.b.keys.h          = 30;
  cmds.b.keys.font       = font;
  cmds.b.keys.options    = FT80X_OPT_FLAT;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "ABCD");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  cmds.text.cmd          = FT80X_CMD_TEXT;                  /* Text */
  cmds.text.x            = 20;
  cmds.text.y            = 40;
  cmds.text.font         = 26;
  cmds.text.options      = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.text, sizeof(cmds.text));
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

  /* Draw keys with 3d effect */

  xoffset                = 4 * width + 20;

  cmds.b.fgcolor.cmd     = FT80X_CMD_FGCOLOR;               /* Foreground color */
  cmds.b.fgcolor.c       = 0x800000;

  cmds.b.keys.cmd        = FT80X_CMD_KEYS;                  /* Keys */
  cmds.b.keys.x          = xoffset;
  cmds.b.keys.y          = 10;
  cmds.b.keys.w          = 4 * width;
  cmds.b.keys.h          = 30;
  cmds.b.keys.font       = font;
  cmds.b.keys.options    = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "ABCD");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  cmds.text.cmd          = FT80X_CMD_TEXT;                  /* Text */
  cmds.text.x            = xoffset;
  cmds.text.y            = 40;
  cmds.text.font         = 26;
  cmds.text.options      = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.text, sizeof(cmds.text));
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

  /* Draw keys with center option */

  xoffset               += 4 * width + 20;

  cmds.b.fgcolor.cmd     = FT80X_CMD_FGCOLOR;               /* Foreground color */
  cmds.b.fgcolor.c       = 0xffff000;

  cmds.b.keys.cmd        = FT80X_CMD_KEYS;                  /* Keys */
  cmds.b.keys.x          = xoffset;
  cmds.b.keys.y          = 10;
  cmds.b.keys.w          = FT80X_DISPLAY_WIDTH - 230;
  cmds.b.keys.h          = 30;
  cmds.b.keys.font       = font;
  cmds.b.keys.options    = FT80X_OPT_CENTER;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "ABCD");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  cmds.text.cmd           = FT80X_CMD_TEXT;                 /* Text */
  cmds.text.x             = xoffset;
  cmds.text.y             = 40;
  cmds.text.font          = 26;
  cmds.text.options       = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.text, sizeof(cmds.text));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "Option Center");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  /* Construct a simple keyboard - note that the tags associated with the
   * keys are the character values given in the arguments.
   */

  yoffset                = 80 + 10;

  cmds.c.fgcolor.cmd     = FT80X_CMD_FGCOLOR;               /* Foreground color */
  cmds.c.fgcolor.c       = 0x404080;

  cmds.c.gradcolor.cmd   = FT80X_CMD_GRADCOLOR;             /* Gradient color */
  cmds.c.gradcolor.c     = 0x00ff00;

  cmds.c.keys.cmd        = FT80X_CMD_KEYS;                  /* Keys */
  cmds.c.keys.x          = ydist;
  cmds.c.keys.y          = yoffset;
  cmds.c.keys.w          = 10 * width;
  cmds.c.keys.h          = height;
  cmds.c.keys.font       = font;
  cmds.c.keys.options    = FT80X_OPT_CENTER;

  ret = ft80x_dl_data(fd, buffer, &cmds.c, sizeof(cmds.c));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "qwertyuiop");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  yoffset += height + ydist;

  cmds.d.gradcolor.cmd   = FT80X_CMD_GRADCOLOR;             /* Gradient color */
  cmds.d.gradcolor.c     = 0x00ffff;

  cmds.d.keys.cmd        = FT80X_CMD_KEYS;                  /* Keys */
  cmds.d.keys.x          = ydist;
  cmds.d.keys.y          = yoffset;
  cmds.d.keys.w          = 10 * width;
  cmds.d.keys.h          = height;
  cmds.d.keys.font       = font;
  cmds.d.keys.options    = FT80X_OPT_CENTER;

  ret = ft80x_dl_data(fd, buffer, &cmds.d, sizeof(cmds.d));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "asdfghijkl");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  yoffset               += height + ydist;

  cmds.d.gradcolor.c     = 0xffff00;
  cmds.d.keys.y          = yoffset;
  cmds.d.keys.options    = FT80X_OPT_CENTER | 'z';

  ret = ft80x_dl_data(fd, buffer, &cmds.d, sizeof(cmds.d));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "zxcvbnm");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  cmds.button.cmd        = FT80X_CMD_BUTTON;
  cmds.button.x          = xoffset;
  cmds.button.y          = yoffset;
  cmds.button.w          = width;
  cmds.button.h          = height;
  cmds.button.font       = 28;
  cmds.button.options    = FT80X_OPT_FLAT;

  ret = ft80x_dl_data(fd, buffer, &cmds.button, sizeof(cmds.button));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, " ");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  yoffset                = 80 + 10;

  cmds.keys.cmd          = FT80X_CMD_KEYS;                  /* Keys */
  cmds.keys.x            = 11 * width;
  cmds.keys.y            = yoffset;
  cmds.keys.w            = 3 * width;
  cmds.keys.h            = height;
  cmds.keys.font         = font;
  cmds.keys.options      = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.keys, sizeof(cmds.keys));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "789");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  yoffset               += height + ydist;
  cmds.keys.y            = yoffset;

  ret = ft80x_dl_data(fd, buffer, &cmds.keys, sizeof(cmds.keys));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "456");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  yoffset               += height + ydist;
  cmds.keys.y            = yoffset;

  ret = ft80x_dl_data(fd, buffer, &cmds.keys, sizeof(cmds.keys));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "123");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  yoffset               += height + ydist;

  cmds.e.colora.cmd      = FT80X_COLOR_A(255);

  cmds.e.keys.cmd        = FT80X_CMD_KEYS;                  /* Keys */
  cmds.e.keys.x          = 11 * width;
  cmds.e.keys.y          = yoffset;
  cmds.e.keys.w          = 3 * width;
  cmds.e.keys.h          = height;
  cmds.e.keys.font       = font;
  cmds.e.keys.options    = 0 | '0';

  ret = ft80x_dl_data(fd, buffer, &cmds.e, sizeof(cmds.e));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "0.");
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
 * Name: ft80x_coproc_interactive
 *
 * Description:
 *   Demonstrate the keys HMI interactions
 *
 ****************************************************************************/

int ft80x_coproc_interactive(int fd, FAR struct ft80x_dlbuffer_s *buffer)
{
  int16_t fontid;
  int16_t width ;
  int16_t height;
  int16_t ydist;
  int16_t yoffset;
  char text[INTERACTIVE_TEXTSIZE];
  char ch = '|';
  uint8_t currtag = 0;
  uint8_t prevtag = 0;
  int32_t textndx = 0;
  int ret;
  int i;

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
      struct ft80x_cmd32_s         tagmask;
      struct ft80x_cmd_text_s      text;
    } b;
    struct
    {
      struct ft80x_cmd32_s         tagmask;
      struct ft80x_cmd_gradcolor_s gradcolor;
      struct ft80x_cmd_keys_s      keys;
    } c;
    struct
    {
      struct ft80x_cmd_gradcolor_s gradcolor;
      struct ft80x_cmd_keys_s      keys;
    } d;
    struct
    {
      struct ft80x_cmd32_s         tag;
      struct ft80x_cmd_button_s    button;
    } e;
    struct
    {
      struct ft80x_cmd32_s         colora;
      struct ft80x_cmd_keys_s      keys;
    } f;
    struct ft80x_cmd_keys_s        keys;
  } cmds;

#ifdef CONFIG_LCD_FT80X_QVGA
  fontid = 27;
  width  = 22;
  height = 22;
  ydist  = 3;
#else
  fontid = 29;
  width  = 30;
  height = 30;
  ydist  = 5;
#endif

  for (i = 0; i < 600; i++)
    {
      /* Check the user input and then add the characters into array.
       * Hmmm... a better example might use the FT80X_IOC_EVENTNOTIFY ioctl
       * command to wait for a touch event.
       */

      currtag = ft80x_touch_tag(fd);

      ch = currtag;
      if (currtag == 0)
        {
          /* No touch */

          ch = '|';

          /* Check if we lost the touch */

          if (prevtag != 0)
            {
              textndx++;

              /* Clear all the characters after 100 are pressed */

              if  (textndx > 24)
                {
                  textndx = 0;
                }
            }
        }

      /* Create the hardware display list */

      ret = ft80x_dl_start(fd, buffer, true);
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_start failed: %d\n", ret);
          return ret;
        }

      cmds.a.clearrgb.cmd       = FT80X_CLEAR_COLOR_RGB(64, 64, 64);
      cmds.a.clear.cmd          = FT80X_CLEAR(1, 1, 1);
      cmds.a.colorrgb.cmd       = FT80X_COLOR_RGB(0xff, 0xff, 0xff);

      /* Copy the commands into the display list */

      ret = ft80x_dl_data(fd, buffer, &cmds.a, sizeof(cmds.a));
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
          return ret;
        }

      /* Draw text entered by user */

      /* Make sure the array is a NUL terminated string */

      text[textndx]             = ch;
      text[textndx + 1]         = '\0';

      cmds.b.tagmask.cmd        = FT80X_TAG_MASK(0);

      cmds.b.text.cmd           = FT80X_CMD_TEXT;                   /* Text */
      cmds.b.text.x             = FT80X_DISPLAY_WIDTH / 2;
      cmds.b.text.y             = 40;
      cmds.b.text.font          = fontid;
      cmds.b.text.options       = FT80X_OPT_CENTER;

      ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
          return ret;
        }

      ret = ft80x_dl_string(fd, buffer, text);
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
          return ret;
        }

      cmds.c.tagmask.cmd        = FT80X_TAG_MASK(1);

      /* Construct a simple keyboard - note that the tags associated with
       * the keys are the character values given in the arguments.
       */

      yoffset                   = 80 + 10;

      cmds.c.gradcolor.cmd      = FT80X_CMD_GRADCOLOR;             /* Gradient color */
      cmds.c.gradcolor.c        = 0x00ffff;

      cmds.c.keys.cmd           = FT80X_CMD_KEYS;                  /* Keys */
      cmds.c.keys.x             = ydist;
      cmds.c.keys.y             = yoffset;
      cmds.c.keys.w             = 10 * width;
      cmds.c.keys.h             = height;
      cmds.c.keys.font          = fontid;
      cmds.c.keys.options       = (FT80X_OPT_CENTER | currtag);

      ret = ft80x_dl_data(fd, buffer, &cmds.c, sizeof(cmds.c));
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
          return ret;
        }

      ret = ft80x_dl_string(fd, buffer, "qwertyuiop");
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
          return ret;
        }

      yoffset                  += height + ydist;

      cmds.d.gradcolor.cmd      = FT80X_CMD_GRADCOLOR;             /* Gradient color */
      cmds.d.gradcolor.c        = 0x00ffff;

      cmds.d.keys.cmd           = FT80X_CMD_KEYS;                  /* Keys */
      cmds.d.keys.x             = ydist;
      cmds.d.keys.y             = yoffset;
      cmds.d.keys.w             = 10 * width;
      cmds.d.keys.h             = height;
      cmds.d.keys.font          = fontid;
      cmds.d.keys.options       = (FT80X_OPT_CENTER | currtag);

      ret = ft80x_dl_data(fd, buffer, &cmds.d, sizeof(cmds.d));
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
          return ret;
        }

      ret = ft80x_dl_string(fd, buffer, "asdfghijkl");
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
          return ret;
        }

      yoffset                  += height + ydist;
      cmds.d.gradcolor.c        = 0xffff00;
      cmds.d.keys.y             = yoffset;

      ret = ft80x_dl_data(fd, buffer, &cmds.d, sizeof(cmds.d));
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
          return ret;
        }

      ret = ft80x_dl_string(fd, buffer, "zxcvbnm");
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
          return ret;
        }

      yoffset                  += height + ydist;

      cmds.e.tag.cmd            = FT80X_TAG(' ');

      cmds.e.button.cmd         = FT80X_CMD_BUTTON;
      cmds.e.button.x           = ydist;
      cmds.e.button.y           = yoffset;
      cmds.e.button.w           = 10  *width;
      cmds.e.button.h           = height;
      cmds.e.button.font        = fontid;

      if (currtag == ' ')
        {
          cmds.e.button.options = (FT80X_OPT_CENTER | FT80X_OPT_FLAT);
        }
      else
        {
          cmds.e.button.options = FT80X_OPT_CENTER;
        }

      ret = ft80x_dl_data(fd, buffer, &cmds.e, sizeof(cmds.e));
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
          return ret;
        }

      ret = ft80x_dl_string(fd, buffer, " ");
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
          return ret;
        }

      yoffset                   = 80 + 10;

      cmds.keys.cmd             = FT80X_CMD_KEYS;                  /* Keys */
      cmds.keys.x               = 11 * width;
      cmds.keys.y               = yoffset;
      cmds.keys.w               = 3 * width;
      cmds.keys.h               = height;
      cmds.keys.font            = fontid;
      cmds.keys.options         = (0 | currtag);

      ret = ft80x_dl_data(fd, buffer, &cmds.keys, sizeof(cmds.keys));
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
          return ret;
        }

      ret = ft80x_dl_string(fd, buffer, "789");
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
          return ret;
        }

      yoffset                  += height + ydist;
      cmds.keys.y               = yoffset;

      ret = ft80x_dl_data(fd, buffer, &cmds.keys, sizeof(cmds.keys));
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
          return ret;
        }

      ret = ft80x_dl_string(fd, buffer, "456");
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
          return ret;
        }

      yoffset                  += height + ydist;
      cmds.keys.y               = yoffset;

      ret = ft80x_dl_data(fd, buffer, &cmds.keys, sizeof(cmds.keys));
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
          return ret;
        }

      ret = ft80x_dl_string(fd, buffer, "123");
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
          return ret;
        }

      yoffset                  += height + ydist;

      cmds.f.colora.cmd         = FT80X_COLOR_A(255);

      cmds.f.keys.cmd           = FT80X_CMD_KEYS;                  /* Keys */
      cmds.f.keys.x             = 11 * width;
      cmds.f.keys.y             = yoffset;
      cmds.f.keys.w             = 3 * width;
      cmds.f.keys.h             = height;
      cmds.f.keys.font          = fontid;
      cmds.f.keys.options       = (9 | currtag);

      ret = ft80x_dl_data(fd, buffer, &cmds.f, sizeof(cmds.f));
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
          return ret;
        }

      ret = ft80x_dl_string(fd, buffer, "0.");
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
          return ret;
        }

      /* Terminate the display list */

      ret = ft80x_dl_end(fd, buffer);
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_end failed: %d\n", ret);
          return ret;
        }

      usleep(10 * 1000);
      prevtag = currtag;
    }

  return OK;
}

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
  cmds.a.clear.cmd        = FT80X_CLEAR(1, 1, 1);
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
  cmds.b.bgcolor.c        = 0x800000;
  cmds.b.progress.x       = 180;
  cmds.b.progress.options = 0;
  cmds.b.progress.val     = 75;
  cmds.b.text.x           = 180;

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
  cmds.b.bgcolor.c        = 0x000080;
  cmds.b.progress.x       = 30;
  cmds.b.progress.y       = 60;
  cmds.b.progress.h       = 30;
  cmds.b.progress.val     = 19660;
  cmds.b.progress.range   = 65535;
  cmds.b.text.x           = 78;
  cmds.b.text.y           = 68;

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

  xoffset                 = 20;
  yoffset                 = 120;

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

  yoffset                += ydist;
  cmds.d.progress.y       = yoffset;
  cmds.d.progress.val     = 40;
  cmds.d.bgcolor.c        = 0xffff00;

  ret = ft80x_dl_data(fd, buffer, &cmds.d, sizeof(cmds.d));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  yoffset                += ydist;
  cmds.d.progress.y       = yoffset;
  cmds.d.progress.val     = 70;
  cmds.d.bgcolor.c        = 0x808080;

  ret = ft80x_dl_data(fd, buffer, &cmds.d, sizeof(cmds.d));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  yoffset                += ydist;

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
  cmds.a.clear.cmd         = FT80X_CLEAR(1, 1, 1);
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

  cmds.b.fgcolor.c         = 0x00ff00;
  cmds.b.bgcolor.c         = 0x800000;
  cmds.b.scrollbar.x       = 180;
  cmds.b.scrollbar.options = 0;
  cmds.b.text.x            = 180;

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

  xoffset                  = 20;
  yoffset                  = 120;
  wsz                      = FT80X_DISPLAY_WIDTH / 2 - 40;

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

  yoffset                 += ydist;

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

  cmds.c.colora.cmd        = FT80X_COLOR_A(255);

  ret = ft80x_dl_data(fd, buffer, &cmds.c, sizeof(cmds.c));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  yoffset                 += ydist;

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

  yoffset                 += ydist;
  cmds.f.bgcolor.c         = 0x808080;
  cmds.f.scrollbar.y       = yoffset;
  cmds.f.scrollbar.val     = 70;

  ret = ft80x_dl_data(fd, buffer, &cmds.f, sizeof(cmds.f));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  /* Draw vertical scroll bars */

  xoffset                  = FT80X_DISPLAY_WIDTH / 2 + 40;
  yoffset                  = 80;
  wsz                      = FT80X_DISPLAY_HEIGHT - 100;

  cmds.f.bgcolor.c         = 0x800000;

  cmds.f.scrollbar.x       = xoffset;
  cmds.f.scrollbar.y       = yoffset;
  cmds.f.scrollbar.w       = 8;
  cmds.f.scrollbar.h       = wsz;
  cmds.f.scrollbar.val     = 10;

  ret = ft80x_dl_data(fd, buffer, &cmds.f, sizeof(cmds.f));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  xoffset                 += xdist;

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

  xoffset                 += xdist;

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

  xoffset                 += xdist;
  cmds.f.bgcolor.c         = 0x808080;
  cmds.f.scrollbar.x       = xoffset;
  cmds.f.scrollbar.val     = 70;

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
  cmds.a.clear.cmd         = FT80X_CLEAR(1, 1, 1);
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

  cmds.b.fgcolor.c         = 0x00ff00;
  cmds.b.bgcolor.c         = 0x800000;
  cmds.b.dial.x            = 140;
  cmds.b.dial.options      = 0;
  cmds.b.dial.val          = 0.45 * 65535;                    /* 45% */
  cmds.b.text.x            = 105;

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

  cmds.c.fgcolor.c         = 0x000080;
  cmds.c.colorrgb.cmd      = FT80X_COLOR_RGB(11, 7, 65);
  cmds.c.dial.x            = 100;
  cmds.c.dial.r            = 40;
  cmds.c.dial.val          = 0.45 * 65535;                    /* 45% */
  cmds.c.text.x            = 90;
  cmds.c.text.y            = 200;

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

  cmds.c.fgcolor.c         = 0xffff00;
  cmds.c.colorrgb.cmd      = FT80X_COLOR_RGB(87, 94, 9);
  cmds.c.dial.x            = 210;
  cmds.c.dial.r            = 60;
  cmds.c.dial.val          = 0.60 * 65535;                    /* 60% */
  cmds.c.text.x            = 200;
  cmds.c.text.y            = 220;

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
  cmds.c.fgcolor.c         = 0x808080;

  cmds.c.colorrgb.cmd      = FT80X_COLOR_RGB(51, 50, 52);
  cmds.c.dial.x            = 360;
  cmds.c.dial.r            = 80;
  cmds.c.dial.val          = 0.75 * 65535;                    /* 75% */
  cmds.c.text.x            = 350;
  cmds.c.text.y            = 240;

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
  cmds.a.clear.cmd         = FT80X_CLEAR(1, 1, 1);
  cmds.a.colorrgb.cmd      = FT80X_COLOR_RGB(0xff, 0xff, 0xff);

  ret = ft80x_dl_data(fd, buffer, &cmds.a, sizeof(cmds.a));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

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

  cmds.b.fgcolor.c         = 0x00ff00;
  cmds.b.bgcolor.c         = 0x800000;
  cmds.b.slider.x          = 180;
  cmds.b.slider.options    = 0;
  cmds.b.slider.val        = 50;
  cmds.b.colora.cmd        = FT80X_COLOR_A(255);
  cmds.b.text.x            = 180;

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

  xoffset                  = 20;
  yoffset                  = 120;
  wsz                      = FT80X_DISPLAY_WIDTH / 2 - 40;

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

  yoffset                 += ydist;

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

  yoffset                 += ydist;

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

  yoffset                 += ydist;
  cmds.e.bgcolor.c         = 0x808080;
  cmds.e.colorrgb.cmd      = FT80X_COLOR_RGB(51, 50, 52);
  cmds.e.slider.y          = yoffset;
  cmds.e.slider.val        = 70;

  ret = ft80x_dl_data(fd, buffer, &cmds.e, sizeof(cmds.e));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  xoffset                  = FT80X_DISPLAY_WIDTH / 2 + 40;
  yoffset                  = 80;
  wsz                      = FT80X_DISPLAY_HEIGHT - 100;

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

  xoffset                 += xdist;
  cmds.f.bgcolor.c         = 0x000080;
  cmds.f.slider.x          = xoffset;
  cmds.f.slider.val        = 30;

  ret = ft80x_dl_data(fd, buffer, &cmds.f, sizeof(cmds.f));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  xoffset                 += xdist;
  cmds.f.bgcolor.c         = 0xffff00;
  cmds.f.slider.x          = xoffset;
  cmds.f.slider.val        = 50;

  ret = ft80x_dl_data(fd, buffer, &cmds.f, sizeof(cmds.f));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  xoffset                 += xdist;
  cmds.f.bgcolor.c         = 0x808080;
  cmds.f.slider.x          = xoffset;
  cmds.f.slider.val        = 70;

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
  cmds.a.clear.cmd         = FT80X_CLEAR(1, 1, 1);
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
  cmds.c.toggle.x          = 140;
  cmds.c.toggle.y          = 10;
  cmds.c.toggle.w          = 30;
  cmds.c.toggle.font       = 27;
  cmds.c.toggle.options    = 0;
  cmds.c.toggle.state      = 1 * 65535;

  ret = ft80x_dl_data(fd, buffer, &cmds.c, sizeof(cmds.c));
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

  xoffset                  = 40;
  yoffset                  = 80;

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

  ret = ft80x_dl_data(fd, buffer, &cmds.c, sizeof(cmds.c));
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

  yoffset                 += ydist;
  cmds.c.fgcolor.c         = 0x0b0721;
  cmds.c.bgcolor.c         = 0x000080;
  cmds.c.toggle.y          = yoffset;
  cmds.c.toggle.state      = 0.25 * 65535;

  ret = ft80x_dl_data(fd, buffer, &cmds.c, sizeof(cmds.c));
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

  yoffset                 += ydist;

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

  yoffset                 += ydist;
  cmds.b.fgcolor.c         = 0x333234;
  cmds.b.bgcolor.c         = 0x808080;
  cmds.b.colorrgb.cmd      = FT80X_COLOR_RGB(0xff, 0xff, 0xff);
  cmds.b.toggle.y          = yoffset;
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
 * Name: ft80x_coproc_number
 *
 * Description:
 *   Demonstrate the number widget
 *
 ****************************************************************************/

int ft80x_coproc_number(int fd, FAR struct ft80x_dlbuffer_s *buffer)
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
      struct ft80x_cmd32_s         numcolor;
      struct ft80x_cmd_number_s    number;
      struct ft80x_cmd32_s         txtcolor;
      struct ft80x_cmd_text_s      text;
    } b;
  } cmds;

  /* Create the hardware display list */

  ret = ft80x_dl_start(fd, buffer, true);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_start failed: %d\n", ret);
      return ret;
    }

  cmds.a.clearrgb.cmd      = FT80X_CLEAR_COLOR_RGB(64, 64, 64);
  cmds.a.clear.cmd         = FT80X_CLEAR(1, 1, 1);
  cmds.a.colorrgb.cmd      = FT80X_COLOR_RGB(0xff, 0xff, 0xff);

  ret = ft80x_dl_data(fd, buffer, &cmds.a, sizeof(cmds.a));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  /* Draw number at 0,0 location */

  cmds.b.numcolor.cmd      = FT80X_CLEAR_COLOR_RGB(0x00, 0x00, 0x80);

  cmds.b.number.cmd        = FT80X_CMD_NUMBER;
  cmds.b.number.x          = 0;
  cmds.b.number.y          = 0;
  cmds.b.number.font       = 29;
  cmds.b.number.options    = 0;
  cmds.b.number.n          = 1234;

  cmds.b.txtcolor.cmd      = FT80X_CLEAR_COLOR_RGB(0xff, 0xff, 0xff);

  cmds.b.text.cmd          = FT80X_CMD_TEXT;                   /* Text */
  cmds.b.text.x            = 0;
  cmds.b.text.y            = 40;
  cmds.b.text.font         = 26;
  cmds.b.text.options      = 0;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "Number at 0,0");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  /* Number with centerx */

  cmds.b.numcolor.cmd      = FT80X_CLEAR_COLOR_RGB(0x80, 0x00, 0x00);
  cmds.b.number.x          = FT80X_DISPLAY_WIDTH / 2;
  cmds.b.number.y          = 50;
  cmds.b.number.options    = FT80X_OPT_CENTERX | FT80X_OPT_SIGNED;
  cmds.b.number.n          = -1234;
  cmds.b.text.x            = FT80X_DISPLAY_WIDTH / 2 - 30;
  cmds.b.text.y            = 90;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "Number at CenterX");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  /* Number with centery */

  cmds.b.numcolor.cmd      = FT80X_CLEAR_COLOR_RGB(0x41, 0x01, 0x05);
  cmds.b.number.x          = FT80X_DISPLAY_WIDTH / 5;
  cmds.b.number.y          = 120;
  cmds.b.number.options    = FT80X_OPT_CENTERY;
  cmds.b.number.n          = 1234;
  cmds.b.text.x            = FT80X_DISPLAY_WIDTH / 5;
  cmds.b.text.y            = 140;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "Number at CenterY");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  /* Number with center */

  cmds.b.numcolor.cmd      = FT80X_CLEAR_COLOR_RGB(0x0b, 0x07, 0x21);
  cmds.b.number.x          = FT80X_DISPLAY_WIDTH / 2;
  cmds.b.number.y          = 180;
  cmds.b.number.options    = FT80X_OPT_CENTER | FT80X_OPT_SIGNED;
  cmds.b.number.n          = -1234;
  cmds.b.text.x            = FT80X_DISPLAY_WIDTH / 2 - 50;
  cmds.b.text.y            = 200;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "Number at Center");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  /* Number with rightx */

  cmds.b.numcolor.cmd      = FT80X_CLEAR_COLOR_RGB(0x57, 0x5e, 0x1b);
  cmds.b.number.x          = FT80X_DISPLAY_WIDTH;
  cmds.b.number.y          = 60;
  cmds.b.number.options    = FT80X_OPT_RIGHTX;
  cmds.b.number.n          = 1234;
  cmds.b.text.x            = FT80X_DISPLAY_WIDTH - 100;
  cmds.b.text.y            = 100;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "Number at RightX");
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
 *   REVISIT:  One soft resets, the touch positions come up with different
 *   colors.  Probably need to select some proper fgcolor and bgcolor?
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
  cmds.a.clear.cmd         = FT80X_CLEAR(1, 1, 1);
  cmds.a.colorrgb.cmd      = FT80X_COLOR_RGB(0xff, 0xff, 0xff);

  cmds.a.text.cmd          = FT80X_CMD_TEXT;                   /* Text */
  cmds.a.text.x            = FT80X_DISPLAY_WIDTH / 2;
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

  ret = ft80x_dl_flush(fd, buffer, true);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_flush failed: %d\n", ret);
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
             matrix[0], matrix[1], matrix[2],
             matrix[3], matrix[4], matrix[5]);
  return OK;
}

/****************************************************************************
 * Name: ft80x_coproc_spinner
 *
 * Description:
 *   Demonstrate the spinner widget
 *
 ****************************************************************************/

int ft80x_coproc_spinner(int fd, FAR struct ft80x_dlbuffer_s *buffer)
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
      struct ft80x_cmd32_s         colorrgb;
      struct ft80x_cmd_spinner_s   spinner;
    } b;
    struct ft80x_cmd_spinner_s     spinner;
    struct ft80x_cmd_text_s        text;
    struct ft80x_cmd32_s           stop;
  } cmds;

  /* Create the hardware display list */

  ret = ft80x_dl_start(fd, buffer, true);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_start failed: %d\n", ret);
      return ret;
    }

  cmds.a.clearrgb.cmd      = FT80X_CLEAR_COLOR_RGB(64, 64, 64);
  cmds.a.clear.cmd         = FT80X_CLEAR(1, 1, 1);
  cmds.a.colorrgb.cmd      = FT80X_COLOR_RGB(0xff, 0xff, 0xff);

  ret = ft80x_dl_data(fd, buffer, &cmds.a, sizeof(cmds.a));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  cmds.text.cmd            = FT80X_CMD_TEXT;                   /* Text */
  cmds.text.x              = FT80X_DISPLAY_WIDTH / 2;
  cmds.text.y              = 20;
  cmds.text.font           = 27;
  cmds.text.options        = FT80X_OPT_CENTER;

  ret = ft80x_dl_data(fd, buffer, &cmds.text, sizeof(cmds.text));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "Spinner circle");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  cmds.text.y              = 80;

  ret = ft80x_dl_data(fd, buffer, &cmds.text, sizeof(cmds.text));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "Please Wait ...");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  cmds.spinner.cmd        = FT80X_CMD_SPINNER;
  cmds.spinner.x          = FT80X_DISPLAY_WIDTH / 2;
  cmds.spinner.y          = FT80X_DISPLAY_HEIGHT / 2;
  cmds.spinner.style      = 0;
  cmds.spinner.scale      = 1;

  ret = ft80x_dl_data(fd, buffer, &cmds.spinner, sizeof(cmds.spinner));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  /* Flush the local display buffer to hardware and reset the display list. */

  ret = ft80x_dl_flush(fd, buffer, true);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_flush failed: %d\n", ret);
    }

  sleep(1);

  /* Spinner with style 1 and scale 1 */

  /* Start a new the hardware display list */

  ret = ft80x_dl_start(fd, buffer, true);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_start failed: %d\n", ret);
      return ret;
    }

  cmds.a.clearrgb.cmd      = FT80X_CLEAR_COLOR_RGB(64, 64, 64);
  cmds.a.clear.cmd         = FT80X_CLEAR(1, 1, 1);
  cmds.a.colorrgb.cmd      = FT80X_COLOR_RGB(0xff, 0xff, 0xff);

  ret = ft80x_dl_data(fd, buffer, &cmds.a, sizeof(cmds.a));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  cmds.text.cmd            = FT80X_CMD_TEXT;                   /* Text */
  cmds.text.x              = FT80X_DISPLAY_WIDTH / 2;
  cmds.text.y              = 20;
  cmds.text.font           = 27;
  cmds.text.options        = FT80X_OPT_CENTER;

  ret = ft80x_dl_data(fd, buffer, &cmds.text, sizeof(cmds.text));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "Spinner line");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  cmds.text.y              = 80;

  ret = ft80x_dl_data(fd, buffer, &cmds.text, sizeof(cmds.text));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "Please Wait ...");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  cmds.b.colorrgb.cmd      = FT80X_COLOR_RGB(0x00, 0x00, 0x80);

  cmds.b.spinner.cmd       = FT80X_CMD_SPINNER;
  cmds.b.spinner.x         = FT80X_DISPLAY_WIDTH / 2;
  cmds.b.spinner.y         = FT80X_DISPLAY_HEIGHT / 2;
  cmds.b.spinner.style     = 1;
  cmds.b.spinner.scale     = 1;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  /* Flush the local display buffer to hardware and reset the display list. */

  ret = ft80x_dl_flush(fd, buffer, true);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_flush failed: %d\n", ret);
    }

  sleep(1);

  /* Start a new the hardware display list */

  ret = ft80x_dl_start(fd, buffer, true);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_start failed: %d\n", ret);
      return ret;
    }

  cmds.a.clearrgb.cmd      = FT80X_CLEAR_COLOR_RGB(64, 64, 64);
  cmds.a.clear.cmd         = FT80X_CLEAR(1, 1, 1);
  cmds.a.colorrgb.cmd      = FT80X_COLOR_RGB(0xff, 0xff, 0xff);

  ret = ft80x_dl_data(fd, buffer, &cmds.a, sizeof(cmds.a));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  cmds.text.cmd            = FT80X_CMD_TEXT;                   /* Text */
  cmds.text.x              = FT80X_DISPLAY_WIDTH / 2;
  cmds.text.y              = 20;
  cmds.text.font           = 27;
  cmds.text.options        = FT80X_OPT_CENTER;

  ret = ft80x_dl_data(fd, buffer, &cmds.text, sizeof(cmds.text));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "Spinner clockhand");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  cmds.text.y              = 80;

  ret = ft80x_dl_data(fd, buffer, &cmds.text, sizeof(cmds.text));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "Please Wait ...");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  cmds.b.colorrgb.cmd      = FT80X_COLOR_RGB(0x80, 0x00, 0x00);

  cmds.b.spinner.cmd       = FT80X_CMD_SPINNER;
  cmds.b.spinner.x         = FT80X_DISPLAY_WIDTH / 2;
  cmds.b.spinner.y         = FT80X_DISPLAY_HEIGHT / 2 + 20;
  cmds.b.spinner.style     = 2;
  cmds.b.spinner.scale     = 1;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  /* Flush the local display buffer to hardware and reset the display list. */

  ret = ft80x_dl_flush(fd, buffer, true);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_flush failed: %d\n", ret);
    }

  sleep(1);

  /* Start a new the hardware display list */

  ret = ft80x_dl_start(fd, buffer, true);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_start failed: %d\n", ret);
      return ret;
    }

  cmds.a.clearrgb.cmd      = FT80X_CLEAR_COLOR_RGB(64, 64, 64);
  cmds.a.clear.cmd         = FT80X_CLEAR(1, 1, 1);
  cmds.a.colorrgb.cmd      = FT80X_COLOR_RGB(0xff, 0xff, 0xff);

  ret = ft80x_dl_data(fd, buffer, &cmds.a, sizeof(cmds.a));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  cmds.text.cmd            = FT80X_CMD_TEXT;                   /* Text */
  cmds.text.x              = FT80X_DISPLAY_WIDTH / 2;
  cmds.text.y              = 20;
  cmds.text.font           = 27;
  cmds.text.options        = FT80X_OPT_CENTER;

  ret = ft80x_dl_data(fd, buffer, &cmds.text, sizeof(cmds.text));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "Spinner two dots");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  cmds.text.y              = 80;

  ret = ft80x_dl_data(fd, buffer, &cmds.text, sizeof(cmds.text));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  cmds.b.colorrgb.cmd      = FT80X_COLOR_RGB(0x80, 0x00, 0x00);

  cmds.b.spinner.cmd       = FT80X_CMD_SPINNER;
  cmds.b.spinner.x         = FT80X_DISPLAY_WIDTH / 2;
  cmds.b.spinner.y         = FT80X_DISPLAY_HEIGHT / 2 + 20;
  cmds.b.spinner.style     = 3;
  cmds.b.spinner.scale     = 1;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  /* Flush the local display buffer to hardware and reset the display list. */

  ret = ft80x_dl_flush(fd, buffer, true);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_flush failed: %d\n", ret);
    }

  sleep(1);

  /* Send the stop command */

  cmds.stop.cmd = FT80X_CMD_STOP;
  ret = ft80x_coproc_send(fd, &cmds.stop.cmd, 1);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_coproc_send failed: %d\n", ret);
      return ret;
    }

  return ret;
}

/****************************************************************************
 * Name: ft80x_coproc_screensaver
 *
 * Description:
 *   Demonstrate the screensaver widget
 *
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_FT80X_EXCLUDE_BITMAPS
int ft80x_coproc_screensaver(int fd, FAR struct ft80x_dlbuffer_s *buffer)
{
  FAR const struct ft80x_bitmaphdr_s *bmhdr = &g_lenaface_bmhdr;
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
      struct ft80x_cmd_loadidentity_s loadidentity1;
      struct ft80x_cmd_scale_s     scale;
      struct ft80x_cmd_setmatrix_s setmatrix1;
      struct ft80x_cmd32_s         begin;
      struct ft80x_cmd32_s         bitmapsource;
      struct ft80x_cmd32_s         bitmaplayout;
      struct ft80x_cmd32_s         bitmapsize;
      struct ft80x_cmd32_s         macro;
      struct ft80x_cmd32_s         end;
      struct ft80x_cmd_loadidentity_s loadidentity2;
      struct ft80x_cmd_setmatrix_s setmatrix2;
      struct ft80x_cmd_text_s      text;
    } b;
    struct ft80x_cmd_screensaver_s screensaver;
    struct ft80x_cmd_memset_s      memset;
    struct ft80x_cmd32_s           stop;
  } cmds;

  /* Copy the image into graphics ram for the Lena faced clock */

  ret = ft80x_ramg_write(fd, 0, bmhdr->data, bmhdr->stride * bmhdr->height);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_ramg_write() failed: %d\n", ret);
      return ret;
    }

  /* Send the screen saver command */

  cmds.screensaver.cmd = FT80X_CMD_SCREENSAVER;
  ret = ft80x_coproc_send(fd, &cmds.screensaver.cmd, 1);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_coproc_send failed: %d\n", ret);
      return ret;
    }

  /* Create the hardware display list */

  ret = ft80x_dl_start(fd, buffer, true);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_start failed: %d\n", ret);
      return ret;
    }

  cmds.a.clearrgb.cmd      = FT80X_CLEAR_COLOR_RGB(0, 0, 0x80);
  cmds.a.clear.cmd         = FT80X_CLEAR(1, 1, 1);
  cmds.a.colorrgb.cmd      = FT80X_COLOR_RGB(0xff, 0xff, 0xff);

  ret = ft80x_dl_data(fd, buffer, &cmds.a, sizeof(cmds.a));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  /* Lena bitmap */

  cmds.b.loadidentity1.cmd = FT80X_CMD_LOADIDENTITY;

  cmds.b.scale.cmd         = FT80X_CMD_SCALE;
  cmds.b.scale.sx          = 3 * 65536;
  cmds.b.scale.sy          = 3 * 65536;

  cmds.b.setmatrix1.cmd    = FT80X_CMD_SETMATRIX;

  cmds.b.begin.cmd         = FT80X_BEGIN(FT80X_PRIM_BITMAPS);
  cmds.b.bitmapsource.cmd  = FT80X_BITMAP_SOURCE(0);
  cmds.b.bitmaplayout.cmd  = FT80X_BITMAP_LAYOUT(bmhdr->format,
                                                 bmhdr->stride,
                                                 bmhdr->height);
  cmds.b.bitmapsize.cmd    = FT80X_BITMAP_SIZE(FT80X_FILTER_BILINEAR,
                                               FT80X_WRAP_BORDER,
                                               FT80X_WRAP_BORDER,
                                               3 * bmhdr->width,
                                               3 * bmhdr->height);
  cmds.b.macro.cmd         = FT80X_MACRO(0);
  cmds.b.end.cmd           = FT80X_END();

  cmds.b.loadidentity2.cmd = FT80X_CMD_LOADIDENTITY;
  cmds.b.setmatrix2.cmd    = FT80X_CMD_SETMATRIX;

  cmds.b.text.cmd          = FT80X_CMD_TEXT;
  cmds.b.text.x            = FT80X_DISPLAY_WIDTH / 2;
  cmds.b.text.y            = FT80X_DISPLAY_HEIGHT / 2;
  cmds.b.text.font         = 27;
  cmds.b.text.options      = FT80X_OPT_CENTER;

  ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, "Screen Saver ...");
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  /* Display the text */

  cmds.memset.cmd          = FT80X_CMD_MEMSET;
  cmds.memset.ptr          = FT80X_RAM_G + 3200;
  cmds.memset.value        = 0xff;
  cmds.memset.num          = 160L *2 *120;

  ret = ft80x_dl_data(fd, buffer, &cmds.memset, sizeof(cmds.memset));
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
    }

  /* Wait 3 seconds and send the stop command */

  sleep(3);

  cmds.stop.cmd = FT80X_CMD_STOP;
  ret = ft80x_coproc_send(fd, &cmds.stop.cmd, 1);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_coproc_send failed: %d\n", ret);
      return ret;
    }

  return ret;
}
#endif

/****************************************************************************
 * Name: ft80x_coproc_logo
 *
 * Description:
 *   Demonstrate the logo command.  The logo command causes the co-processor
 *   engine to play back a short animation of the FTDI logo. During logo
 *   playback the MCU should not access any FT800 resources.
 *
 ****************************************************************************/

int ft80x_coproc_logo(int fd, FAR struct ft80x_dlbuffer_s *buffer)
{
  struct ft80x_cmd_logo_s logo;
  int ret;

  /* Send the logo command (no display list is used) */

  logo.cmd = FT80X_CMD_LOGO;

  ret = ft80x_coproc_send(fd, &logo.cmd, 1);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_coproc_send failed: %d\n", ret);
      return ret;
    }

  /* Wait for the logo animation to complete. */

  ret = ft80x_coproc_waitlogo(fd);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_coproc_waitlogo failed: %d\n", ret);
    }

  return ret;
}
