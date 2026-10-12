/****************************************************************************
 * apps/industry/cnc/nvmpg-dro/nvmpg_dro_lcd.c
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
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <nuttx/lcd/lcd_dev.h>
#include <nuttx/video/fb.h>

#include "nvmpg_dro.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Scratch buffer for one character cell or a band of a filled rectangle */

#define NVDRO_BUFPIXELS  1024

#define NVDRO_FONT_FIRST 0x20
#define NVDRO_FONT_LAST  0x5a

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_lcdfd = -1;
static int g_xres;
static int g_yres;
static uint16_t g_buf[NVDRO_BUFPIXELS];

/* Classic 5x7 font, ' ' .. 'Z'.  One byte per column, bit 0 is the top
 * row.  Lowercase letters are drawn as uppercase except 'x', which is
 * handled separately so that the step reads "x100" like on the original
 * pendant.
 */

static const uint8_t g_font[][NVDRO_FONT_W] =
{
  { 0x00, 0x00, 0x00, 0x00, 0x00 }, /* ' ' */
  { 0x00, 0x00, 0x5f, 0x00, 0x00 }, /* '!' */
  { 0x00, 0x07, 0x00, 0x07, 0x00 }, /* '"' */
  { 0x14, 0x7f, 0x14, 0x7f, 0x14 }, /* '#' */
  { 0x24, 0x2a, 0x7f, 0x2a, 0x12 }, /* '$' */
  { 0x23, 0x13, 0x08, 0x64, 0x62 }, /* '%' */
  { 0x36, 0x49, 0x55, 0x22, 0x50 }, /* '&' */
  { 0x00, 0x05, 0x03, 0x00, 0x00 }, /* ''' */
  { 0x00, 0x1c, 0x22, 0x41, 0x00 }, /* '(' */
  { 0x00, 0x41, 0x22, 0x1c, 0x00 }, /* ')' */
  { 0x08, 0x2a, 0x1c, 0x2a, 0x08 }, /* '*' */
  { 0x08, 0x08, 0x3e, 0x08, 0x08 }, /* '+' */
  { 0x00, 0x50, 0x30, 0x00, 0x00 }, /* ',' */
  { 0x08, 0x08, 0x08, 0x08, 0x08 }, /* '-' */
  { 0x00, 0x60, 0x60, 0x00, 0x00 }, /* '.' */
  { 0x20, 0x10, 0x08, 0x04, 0x02 }, /* '/' */
  { 0x3e, 0x51, 0x49, 0x45, 0x3e }, /* '0' */
  { 0x00, 0x42, 0x7f, 0x40, 0x00 }, /* '1' */
  { 0x42, 0x61, 0x51, 0x49, 0x46 }, /* '2' */
  { 0x21, 0x41, 0x45, 0x4b, 0x31 }, /* '3' */
  { 0x18, 0x14, 0x12, 0x7f, 0x10 }, /* '4' */
  { 0x27, 0x45, 0x45, 0x45, 0x39 }, /* '5' */
  { 0x3c, 0x4a, 0x49, 0x49, 0x30 }, /* '6' */
  { 0x01, 0x71, 0x09, 0x05, 0x03 }, /* '7' */
  { 0x36, 0x49, 0x49, 0x49, 0x36 }, /* '8' */
  { 0x06, 0x49, 0x49, 0x29, 0x1e }, /* '9' */
  { 0x00, 0x36, 0x36, 0x00, 0x00 }, /* ':' */
  { 0x00, 0x56, 0x36, 0x00, 0x00 }, /* ';' */
  { 0x00, 0x08, 0x14, 0x22, 0x41 }, /* '<' */
  { 0x14, 0x14, 0x14, 0x14, 0x14 }, /* '=' */
  { 0x41, 0x22, 0x14, 0x08, 0x00 }, /* '>' */
  { 0x02, 0x01, 0x51, 0x09, 0x06 }, /* '?' */
  { 0x32, 0x49, 0x79, 0x41, 0x3e }, /* '@' */
  { 0x7e, 0x11, 0x11, 0x11, 0x7e }, /* 'A' */
  { 0x7f, 0x49, 0x49, 0x49, 0x36 }, /* 'B' */
  { 0x3e, 0x41, 0x41, 0x41, 0x22 }, /* 'C' */
  { 0x7f, 0x41, 0x41, 0x22, 0x1c }, /* 'D' */
  { 0x7f, 0x49, 0x49, 0x49, 0x41 }, /* 'E' */
  { 0x7f, 0x09, 0x09, 0x01, 0x01 }, /* 'F' */
  { 0x3e, 0x41, 0x41, 0x51, 0x32 }, /* 'G' */
  { 0x7f, 0x08, 0x08, 0x08, 0x7f }, /* 'H' */
  { 0x00, 0x41, 0x7f, 0x41, 0x00 }, /* 'I' */
  { 0x20, 0x40, 0x41, 0x3f, 0x01 }, /* 'J' */
  { 0x7f, 0x08, 0x14, 0x22, 0x41 }, /* 'K' */
  { 0x7f, 0x40, 0x40, 0x40, 0x40 }, /* 'L' */
  { 0x7f, 0x02, 0x04, 0x02, 0x7f }, /* 'M' */
  { 0x7f, 0x04, 0x08, 0x10, 0x7f }, /* 'N' */
  { 0x3e, 0x41, 0x41, 0x41, 0x3e }, /* 'O' */
  { 0x7f, 0x09, 0x09, 0x09, 0x06 }, /* 'P' */
  { 0x3e, 0x41, 0x51, 0x21, 0x5e }, /* 'Q' */
  { 0x7f, 0x09, 0x19, 0x29, 0x46 }, /* 'R' */
  { 0x46, 0x49, 0x49, 0x49, 0x31 }, /* 'S' */
  { 0x01, 0x01, 0x7f, 0x01, 0x01 }, /* 'T' */
  { 0x3f, 0x40, 0x40, 0x40, 0x3f }, /* 'U' */
  { 0x1f, 0x20, 0x40, 0x20, 0x1f }, /* 'V' */
  { 0x7f, 0x20, 0x18, 0x20, 0x7f }, /* 'W' */
  { 0x63, 0x14, 0x08, 0x14, 0x63 }, /* 'X' */
  { 0x03, 0x04, 0x78, 0x04, 0x03 }, /* 'Y' */
  { 0x61, 0x51, 0x49, 0x45, 0x43 }, /* 'Z' */
};

static const uint8_t g_glyph_x[NVDRO_FONT_W] =
{
  0x44, 0x28, 0x10, 0x28, 0x44      /* 'x' */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static FAR const uint8_t *nvdro_glyph(char ch)
{
  if (ch == 'x')
    {
      return g_glyph_x;
    }

  if (ch >= 'a' && ch <= 'z')
    {
      ch -= 'a' - 'A';
    }

  if (ch < NVDRO_FONT_FIRST || ch > NVDRO_FONT_LAST)
    {
      ch = '?';
    }

  return g_font[ch - NVDRO_FONT_FIRST];
}

/* Copy w x h pixels from g_buf to the screen at (x, y), clipped */

static void nvdro_blit(int x, int y, int w, int h)
{
  struct lcddev_area_s area;
  int stride = w * sizeof(uint16_t);
  int x0 = x;
  int y0 = y;
  int x1 = x + w - 1;
  int y1 = y + h - 1;

  if (g_lcdfd < 0 || w <= 0 || h <= 0)
    {
      return;
    }

  if (x0 < 0)
    {
      x0 = 0;
    }

  if (y0 < 0)
    {
      y0 = 0;
    }

  if (x1 >= g_xres)
    {
      x1 = g_xres - 1;
    }

  if (y1 >= g_yres)
    {
      y1 = g_yres - 1;
    }

  if (x0 > x1 || y0 > y1)
    {
      return;
    }

  area.row_start = y0;
  area.row_end   = y1;
  area.col_start = x0;
  area.col_end   = x1;
  area.stride    = stride;
  area.data      = (FAR uint8_t *)&g_buf[(y0 - y) * w + (x0 - x)];

  ioctl(g_lcdfd, LCDDEVIO_PUTAREA, (unsigned long)((uintptr_t)&area));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int nvdro_lcd_open(FAR const char *path)
{
  struct fb_videoinfo_s vinfo;
  int ret;

  g_lcdfd = open(path, O_RDWR);
  if (g_lcdfd < 0)
    {
      return -errno;
    }

  ret = ioctl(g_lcdfd, LCDDEVIO_GETVIDEOINFO,
              (unsigned long)((uintptr_t)&vinfo));
  if (ret < 0 || vinfo.fmt != FB_FMT_RGB16_565)
    {
      ret = ret < 0 ? -errno : -ENOTSUP;
      nvdro_lcd_close();
      return ret;
    }

  g_xres = vinfo.xres;
  g_yres = vinfo.yres;

  ioctl(g_lcdfd, LCDDEVIO_SETPOWER, CONFIG_LCD_MAXPOWER);
  return 0;
}

void nvdro_lcd_close(void)
{
  if (g_lcdfd >= 0)
    {
      close(g_lcdfd);
      g_lcdfd = -1;
    }
}

int nvdro_lcd_width(void)
{
  return g_xres;
}

int nvdro_lcd_height(void)
{
  return g_yres;
}

void nvdro_fill(int x, int y, int w, int h, uint16_t color)
{
  int band;
  int i;

  if (w <= 0 || h <= 0)
    {
      return;
    }

  /* Paint in horizontal bands that fit in the scratch buffer */

  band = NVDRO_BUFPIXELS / w;
  if (band > h)
    {
      band = h;
    }

  for (i = 0; i < band * w; i++)
    {
      g_buf[i] = color;
    }

  while (h > 0)
    {
      int n = h < band ? h : band;

      nvdro_blit(x, y, w, n);
      y += n;
      h -= n;
    }
}

void nvdro_putrow(int x, int y, int w, FAR const uint16_t *pixels)
{
  if (w > NVDRO_BUFPIXELS)
    {
      w = NVDRO_BUFPIXELS;
    }

  memcpy(g_buf, pixels, w * sizeof(uint16_t));
  nvdro_blit(x, y, w, 1);
}

void nvdro_frame(int x, int y, int w, int h, int t, uint16_t color)
{
  nvdro_fill(x, y, w, t, color);
  nvdro_fill(x, y + h - t, w, t, color);
  nvdro_fill(x, y + t, t, h - 2 * t, color);
  nvdro_fill(x + w - t, y + t, t, h - 2 * t, color);
}

int nvdro_text_width(int nchars, int scale, int gap)
{
  return nchars * (NVDRO_FONT_W * scale + gap);
}

int nvdro_text(int x, int y, FAR const char *str, int scale, int gap,
               uint16_t fg, uint16_t bg)
{
  int cw = NVDRO_FONT_W * scale + gap;
  int ch = NVDRO_FONT_H * scale;
  int x0 = x;

  if (cw * ch > NVDRO_BUFPIXELS)
    {
      return 0;
    }

  for (; *str != '\0'; str++)
    {
      FAR const uint8_t *glyph = nvdro_glyph(*str);
      int row;
      int col;

      for (row = 0; row < ch; row++)
        {
          FAR uint16_t *dst = &g_buf[row * cw];
          int bit = 1 << (row / scale);

          for (col = 0; col < cw; col++)
            {
              int gc = col / scale;

              dst[col] = (gc < NVDRO_FONT_W && (glyph[gc] & bit)) ? fg : bg;
            }
        }

      nvdro_blit(x, y, cw, ch);
      x += cw;
    }

  return x - x0;
}
