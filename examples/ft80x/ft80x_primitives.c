/****************************************************************************
 * apps/examples/ft80x/ft80x_primitives.c
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
 * Pre-processor Definitions
 ****************************************************************************/

#define STENCIL_NUM_LINES   8
#define STENCIL_NUM_POINTS  6

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static float lerp(float t, float a, float b)
{
  return (float)((1 - t) * a + t * b);
}

static float smoothlerp(float t, float a, float b)
{
  float lt = 3 * t * t - 2 * t * t * t;
  return lerp(lt, a, b);
}

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
int ft80x_prim_bitmaps(int fd, FAR struct ft80x_dlbuffer_s *buffer)
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

  ret = ft80x_dl_create(fd, buffer, cmds, 18, false);
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

int ft80x_prim_points(int fd, FAR struct ft80x_dlbuffer_s *buffer)
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

  ret = ft80x_dl_create(fd, buffer, cmds, 15, false);
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

int ft80x_prim_lines(int fd, FAR struct ft80x_dlbuffer_s *buffer)
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

  ret = ft80x_dl_create(fd, buffer, cmds, 14, false);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_create failed: %d\n", ret);
      return ret;
    }

  return OK;
}

/****************************************************************************
 * Name: ft80x_linestrip
 *
 * Description:
 *   Demonstrate the line strip primitive
 *
 ****************************************************************************/

int ft80x_prim_linestrip(int fd, FAR struct ft80x_dlbuffer_s *buffer)
{
  uint32_t cmds[7];
  int ret;

  /* Format the display list data */

  cmds[0] = FT80X_CLEAR_COLOR_RGB(5, 45, 10);
  cmds[1] = FT80X_COLOR_RGB(255, 168, 64);
  cmds[2] = FT80X_CLEAR(1 ,1 ,1);
  cmds[3] = FT80X_BEGIN(FT80X_PRIM_LINE_STRIP);
  cmds[4] = FT80X_VERTEX2F(16 * 16, 16 * 16);
  cmds[5] = FT80X_VERTEX2F(((FT80X_DISPLAY_WIDTH * 2) /3) * 16,
                           (FT80X_DISPLAY_HEIGHT * 2 / 3) * 16);
  cmds[6] = FT80X_VERTEX2F((FT80X_DISPLAY_WIDTH - 80) * 16,
                           (FT80X_DISPLAY_HEIGHT - 20) * 16);

  /* Create the hardware display list */

  ret = ft80x_dl_create(fd, buffer, cmds, 7, false);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_create failed: %d\n", ret);
      return ret;
    }

  return OK;
}

/****************************************************************************
 * Name: ft80x_edgestrip_r
 *
 * Description:
 *   Demonstrate the edge strip right primitive
 *
 ****************************************************************************/

int ft80x_prim_edgestrip_r(int fd, FAR struct ft80x_dlbuffer_s *buffer)
{
  uint32_t cmds[7];
  int ret;

  /* Format the display list data */

  cmds[0] = FT80X_CLEAR_COLOR_RGB(5, 45, 10);
  cmds[1] = FT80X_COLOR_RGB(255, 168, 64);
  cmds[2] = FT80X_CLEAR(1 ,1 ,1);
  cmds[3] = FT80X_BEGIN(FT80X_PRIM_EDGE_STRIP_R);
  cmds[4] = FT80X_VERTEX2F(16 * 16,16 * 16);
  cmds[5] = FT80X_VERTEX2F(((FT80X_DISPLAY_WIDTH * 2) / 3) * 16,
                           ((FT80X_DISPLAY_HEIGHT * 2) / 3) * 16);
  cmds[6] = FT80X_VERTEX2F((FT80X_DISPLAY_WIDTH - 80) * 16,
                           (FT80X_DISPLAY_HEIGHT - 20) * 16);

  /* Create the hardware display list */

  ret = ft80x_dl_create(fd, buffer, cmds, 7, false);
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

int ft80x_prim_rectangles(int fd, FAR struct ft80x_dlbuffer_s *buffer)
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

  ret = ft80x_dl_create(fd, buffer, cmds, 14, false);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_create failed: %d\n", ret);
      return ret;
    }

  return OK;
}

/****************************************************************************
 * Name: ft80x_prim_scissor
 *
 * Description:
 *   Demonstrate the scissor primitive
 *
 ****************************************************************************/

int ft80x_prim_scissor(int fd, FAR struct ft80x_dlbuffer_s *buffer)
{
  uint32_t cmds[5];
  int ret;

  cmds[0] = FT80X_CLEAR(1, 1, 1);               /* Clear to black */
  cmds[1] = FT80X_SCISSOR_XY(40, 20);           /* Scissor rectangle top left at (40, 20) */
  cmds[2] = FT80X_SCISSOR_SIZE(40, 40);         /* Scissor rectangle is 40 x 40 pixels */
  cmds[3] = FT80X_CLEAR_COLOR_RGB(255, 255, 0); /* Clear to yellow */
  cmds[4] = FT80X_CLEAR(1, 1, 1);

  /* Create the hardware display list */

  ret = ft80x_dl_create(fd, buffer, cmds, 5, false);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_create failed: %d\n", ret);
      return ret;
    }

  return OK;
}

/****************************************************************************
 * Name: ft80x_prim_stencil
 *
 * Description:
 *   Demonstrate additive blend
 *
 ****************************************************************************/

int ft80x_prim_stencil(int fd, FAR struct ft80x_dlbuffer_s *buffer)
{
  int16_t xball = FT80X_DISPLAY_WIDTH / 2;
  int16_t yball = 120;
  int16_t rball = FT80X_DISPLAY_WIDTH / 8;
  int16_t asize;
  int16_t aradius;
  int16_t gridsize = 20;
  int32_t asmooth;
  int32_t dispr = FT80X_DISPLAY_WIDTH - 10;
  int32_t displ = 10;
  int32_t dispa = 10;
  int32_t dispb = FT80X_DISPLAY_HEIGHT - 10;
  int8_t xflag = 1;
  int8_t yflag = 1;
  int ret;
  int i;
  int j;

  /* Display output chunks */

  union
  {
    struct
    {
      struct ft80x_cmd32_s         clearrgb;
      struct ft80x_cmd32_s         clear;
      struct ft80x_cmd32_s         stencil;
      struct ft80x_cmd32_s         colorrgb;
      struct ft80x_cmd32_s         linewidth;
      struct ft80x_cmd32_s         begin;
    } a;
    struct
    {
      struct ft80x_cmd32_s         vertex2f1;
      struct ft80x_cmd32_s         vertex2f2;
    } b;
    struct
    {
      struct ft80x_cmd32_s         end;
      struct ft80x_cmd32_s         colormask;
      struct ft80x_cmd32_s         pointsize;
      struct ft80x_cmd32_s         begin;
      struct ft80x_cmd32_s         vertex2f;
      struct ft80x_cmd32_s         stencilop;
      struct ft80x_cmd32_s         stencilfunc;
    } c;
    struct
    {
      struct ft80x_cmd32_s         pointsize;
      struct ft80x_cmd32_s         vertex2f;
      struct ft80x_cmd32_s         end;
      struct ft80x_cmd32_s         begin;
    } d;
    struct
    {
      struct ft80x_cmd32_s         linewidth;
      struct ft80x_cmd32_s         vertex2f1;
      struct ft80x_cmd32_s         vertex2f2;
    } e;
    struct
    {
      struct ft80x_cmd32_s         end1;
      struct ft80x_cmd32_s         colormask;
      struct ft80x_cmd32_s         stencilfunc1;
      struct ft80x_cmd32_s         stencilop1;
      struct ft80x_cmd32_s         colorrgb1;
      struct ft80x_cmd32_s         pointsize;
      struct ft80x_cmd32_s         begin;
      struct ft80x_cmd32_s         vertex2f1;
      struct ft80x_cmd32_s         colorrgb2;
      struct ft80x_cmd32_s         colora1;
      struct ft80x_cmd32_s         vertex2f2;
      struct ft80x_cmd32_s         colora2;
      struct ft80x_cmd32_s         colorrgb3;
      struct ft80x_cmd32_s         vertex2f3;
      struct ft80x_cmd32_s         colorrgb4;
      struct ft80x_cmd32_s         stencilfunc2;
      struct ft80x_cmd32_s         stencilop2;
      struct ft80x_cmd32_s         vertex2f4;
      struct ft80x_cmd32_s         end2;
    } f;
  } cmds;

  dispr -= (dispr - displ) % gridsize;
  dispb -= (dispb - dispa) % gridsize;

  ret = ft80x_audio_enable(fd, true);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_audio_enable(FT80X_IOC_AUDIO) failed: %d\n", ret);
    }

  for (i = 100; i > 0; i--)
    {
      if ((xball + rball + 2) >= dispr || (xball - rball - 2) <= displ)
        {
          xflag ^= 1;

          ret = ft80x_audio_playsound(fd, 0, FT08X_EFFECT_CLICK);
          if (ret < 0)
            {
              ft80x_err("ERROR: ft80x_audio_playsound failed: %d\n", ret);
              goto errout_with_sound;;
            }
        }

      if ((yball + rball + 8) >= dispb || (yball - rball - 8) <= dispa)
        {
          yflag ^= 1;

          ret = ft80x_audio_playsound(fd, 0, FT08X_EFFECT_CLICK);
          if (ret < 0)
            {
              ft80x_err("ERROR: ft80x_audio_playsound failed: %d\n", ret);
              goto errout_with_sound;;
            }
        }

      if (xflag != 0)
        {
          xball += 2;
        }
      else
        {
          xball -= 2;
        }

      if (yflag != 0)
        {
          yball += 8 ;
        }
      else
        {
          yball -= 8;
        }

      /* Create the hardware display list */

      ret = ft80x_dl_start(fd, buffer, false);
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_start failed: %d\n", ret);
          goto errout_with_sound;;
        }

      cmds.a.clearrgb.cmd    = FT80X_CLEAR_COLOR_RGB(128, 128, 0);
      cmds.a.clear.cmd       = FT80X_CLEAR(1, 1, 1);
      cmds.a.stencil.cmd     = FT80X_STENCIL_OP(STENCIL_OP_INCR, STENCIL_OP_INCR);
      cmds.a.colorrgb.cmd    = FT80X_COLOR_RGB(0, 0, 0);

      /* Draw grid */

      cmds.a.linewidth.cmd   = FT80X_LINE_WIDTH(16);
      cmds.a.begin.cmd       = FT80X_BEGIN(FT80X_PRIM_LINES);

      ret = ft80x_dl_data(fd, buffer, &cmds.a, sizeof(cmds.a));
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
          goto errout_with_sound;;
        }

      for (j = 0; j <= ((dispr - displ) / gridsize); j++)
        {
          cmds.b.vertex2f1.cmd =
            FT80X_VERTEX2F((displ + j * gridsize) * 16, dispa * 16);
          cmds.b.vertex2f2.cmd =
            FT80X_VERTEX2F((displ + j * gridsize) * 16, dispb * 16);

          ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
          if (ret < 0)
            {
              ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
              goto errout_with_sound;;
            }
        }

      for (j = 0; j <= ((dispb - dispa) / gridsize); j++)
        {
          cmds.b.vertex2f1.cmd =
            FT80X_VERTEX2F(displ * 16, (dispa + j * gridsize) * 16);
          cmds.b.vertex2f2.cmd =
            FT80X_VERTEX2F(dispr * 16, (dispa + j * gridsize) * 16);

          ret = ft80x_dl_data(fd, buffer, &cmds.b, sizeof(cmds.b));
          if (ret < 0)
            {
              ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
              goto errout_with_sound;;
            }
        }

      cmds.c.end.cmd         = FT80X_END();
      cmds.c.colormask.cmd   = FT80X_COLOR_MASK(0,0,0,0);
      cmds.c.pointsize.cmd   = FT80X_POINT_SIZE(rball*16);
      cmds.c.begin.cmd       = FT80X_BEGIN(FT80X_PRIM_POINTS);
      cmds.c.vertex2f.cmd    = FT80X_VERTEX2F(xball*16,yball*16);
      cmds.c.stencilop.cmd   = FT80X_STENCIL_OP(STENCIL_OP_INCR, STENCIL_OP_ZERO);
      cmds.c.stencilfunc.cmd = FT80X_STENCIL_FUNC(STENCIL_FUNC_GEQUAL,1,255);

      ret = ft80x_dl_data(fd, buffer, &cmds.c, sizeof(cmds.c));
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
          goto errout_with_sound;;
        }

      /* One side points */

      for (j = 1; j <= STENCIL_NUM_LINES; j++)
        {
          asize   = j * rball * 2 / (STENCIL_NUM_LINES + 1);
          asmooth = (int32_t)
            smoothlerp((float)((float)asize /
                       (2 * (float)rball)), 0, 2 * (float)rball);

          if (asmooth > rball)
            {
              /* Change the offset to -ve */

              int32_t tmp = asmooth - rball;
              aradius = (rball * rball + tmp * tmp) / (2 * tmp);

              cmds.d.pointsize.cmd = FT80X_POINT_SIZE(aradius*16);
              cmds.d.vertex2f.cmd  = FT80X_VERTEX2F((xball - aradius + tmp)*16,yball*16);
            }
          else
            {
              int32_t tmp = rball - asmooth;
              aradius = (rball * rball + tmp * tmp) / (2 * tmp);

              cmds.d.pointsize.cmd = FT80X_POINT_SIZE(aradius*16);
              cmds.d.vertex2f.cmd  = FT80X_VERTEX2F((xball+ aradius - tmp)*16,yball*16);
            }
        }

      cmds.d.end.cmd         = FT80X_END();
      cmds.d.begin.cmd       = FT80X_BEGIN(FT80X_PRIM_LINES);

      ret = ft80x_dl_data(fd, buffer, &cmds.d, sizeof(cmds.d));
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
          goto errout_with_sound;;
        }

      /* Draw lines - line should be at least radius diameter */

      for (j = 1; j <= STENCIL_NUM_LINES; j++)
        {
          asize   = (j * rball * 2 / STENCIL_NUM_LINES);
          asmooth = (int32_t)
            smoothlerp((float)((float)asize /
                       (2 * (float)rball)), 0, 2 * (float)rball);

          cmds.e.linewidth.cmd = FT80X_LINE_WIDTH(asmooth * 16);
          cmds.e.vertex2f1.cmd = FT80X_VERTEX2F((xball - rball)*16,(yball - rball )*16);
          cmds.e.vertex2f2.cmd = FT80X_VERTEX2F((xball + rball)*16,(yball - rball )*16);

          ret = ft80x_dl_data(fd, buffer, &cmds.e, sizeof(cmds.e));
          if (ret < 0)
            {
              ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
              goto errout_with_sound;;
            }
        }

      cmds.f.end1.cmd         = FT80X_END();

      cmds.f.colormask.cmd    = FT80X_COLOR_MASK(1, 1, 1, 1);
      cmds.f.stencilfunc1.cmd = FT80X_STENCIL_FUNC(STENCIL_FUNC_ALWAYS, 1, 255);
      cmds.f.stencilop1.cmd   = FT80X_STENCIL_OP(STENCIL_OP_KEEP, STENCIL_OP_KEEP);
      cmds.f.colorrgb1.cmd    = FT80X_COLOR_RGB(255, 255, 255);
      cmds.f.pointsize.cmd    = FT80X_POINT_SIZE(rball * 16);
      cmds.f.begin.cmd        = FT80X_BEGIN(FT80X_PRIM_POINTS);
      cmds.f.vertex2f1.cmd    = FT80X_VERTEX2F((xball - 1) * 16, (yball - 1) * 16);
      cmds.f.colorrgb2.cmd    = FT80X_COLOR_RGB(0, 0, 0);
      cmds.f.colora1.cmd      = FT80X_COLOR_A(160);
      cmds.f.vertex2f2.cmd    = FT80X_VERTEX2F((xball + 16) * 16, (yball + 8) * 16);
      cmds.f.colora2.cmd      = FT80X_COLOR_A(255);
      cmds.f.colorrgb3.cmd    = FT80X_COLOR_RGB(255, 255, 255);
      cmds.f.vertex2f3.cmd    = FT80X_VERTEX2F(xball * 16, yball * 16);
      cmds.f.colorrgb4.cmd    = FT80X_COLOR_RGB(255, 0, 0);
      cmds.f.stencilfunc2.cmd = FT80X_STENCIL_FUNC(STENCIL_FUNC_GEQUAL, 1, 1);
      cmds.f.stencilop2.cmd   = FT80X_STENCIL_OP(STENCIL_OP_KEEP, STENCIL_OP_KEEP);
      cmds.f.vertex2f4.cmd    = FT80X_VERTEX2F(xball * 16, yball * 16);
      cmds.f.end2.cmd         = FT80X_END();

      ret = ft80x_dl_data(fd, buffer, &cmds.f, sizeof(cmds.f));
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
          goto errout_with_sound;;
        }

      /* Finally, terminate the display list */

      ret = ft80x_dl_end(fd, buffer);
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_end failed: %d\n", ret);
        }
    }

errout_with_sound:
  ft80x_audio_enable(fd, false);
  return ret;
}

/****************************************************************************
 * Name: ft80x_prim_alphablend
 *
 * Description:
 *   Demonstrate additive blend
 *
 ****************************************************************************/

int ft80x_prim_alphablend(int fd, FAR struct ft80x_dlbuffer_s *buffer)
{
  uint32_t cmds[8];
  int ret;

  cmds[0] = FT80X_CLEAR(1, 1, 1);               /* Clear screen */
  cmds[1] = FT80X_BEGIN(FT80X_PRIM_BITMAPS);
  cmds[2] = FT80X_VERTEX2II(50, 30, 31, 0x47);
  cmds[3] = FT80X_COLOR_A(128);
  cmds[4] = FT80X_VERTEX2II(58, 38, 31, 0x47);
  cmds[5] = FT80X_COLOR_A(64);
  cmds[6] = FT80X_VERTEX2II(66, 46, 31, 0x47);
  cmds[7] = FT80X_END();

  /* Create the hardware display list */

  ret = ft80x_dl_create(fd, buffer, cmds, 8, false);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_create failed: %d\n", ret);
      return ret;
    }

  return OK;
}
