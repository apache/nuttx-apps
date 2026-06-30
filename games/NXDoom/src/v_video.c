/****************************************************************************
 * apps/games/NXDoom/src/v_video.c
 *
 * SPDX-License-Identifier: GPLv2
 *
 * Copyright(C) 1993-1996 Id Software, Inc.
 * Copyright(C) 1993-2008 Raven Software
 * Copyright(C) 2005-2014 Simon Howard
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * DESCRIPTION:
 *  Gamma correction LUT stuff.
 *  Functions to draw patches (by post) directly to screen.
 *  Functions to blit a block to the screen.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "i_system.h"

#include "doomtype.h"

#include "deh_str.h"
#include "i_input.h"
#include "i_swap.h"
#include "i_video.h"
#include "m_bbox.h"
#include "m_misc.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#include "config.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MOUSE_SPEED_BOX_WIDTH 120
#define MOUSE_SPEED_BOX_HEIGHT 9
#define MOUSE_SPEED_BOX_X (SCREENWIDTH - MOUSE_SPEED_BOX_WIDTH - 10)
#define MOUSE_SPEED_BOX_Y 15

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* SCREEN SHOTS */

begin_packed_struct struct pcx_t
{
  char manufacturer;
  char version;
  char encoding;
  char bits_per_pixel;

  unsigned short xmin;
  unsigned short ymin;
  unsigned short xmax;
  unsigned short ymax;

  unsigned short hres;
  unsigned short vres;

  unsigned char palette[48];

  char reserved;
  char color_planes;
  unsigned short bytes_per_line;
  unsigned short palette_type;

  char filler[58];
  unsigned char data; /* unbounded */
} end_packed_struct;

typedef struct pcx_t pcx_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* The screen buffer that the v_video.c code draws to. */

static pixel_t *dest_screen = NULL;

/* haleyjd 08/28/10: clipping callback function for patches.
 * This is needed for Chocolate Strife, which clips patches to the screen.
 */

static vpatchclipfunc_t patchclip_callback = NULL;

/* Highest seen mouse turn speed. We scale the range of the thermometer
 * according to this value, so that it never exceeds the range. Initially
 * this is set to a 1:1 setting where 1 pixel = 1 unit of speed.
 */

static int max_seen_speed = MOUSE_SPEED_BOX_WIDTH - 1;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Blending table used for fuzzpatch, etc. Only used in Heretic/Hexen */

byte *tinttable = NULL;

/* villsa [STRIFE] Blending table used for Strife */

byte *xlatab = NULL;

int dirtybox[4];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void draw_accelerating_box(int speed)
{
  int red;
  int white;
  int yellow;
  int original_speed;
  int redline_x;
  int linelen;

  red = i_get_palette_index(0xff, 0x00, 0x00);
  white = i_get_palette_index(0xff, 0xff, 0xff);
  yellow = i_get_palette_index(0xff, 0xff, 0x00);

  /* Calculate the position of the red threshold line when calibrating
   * acceleration.  This is 1/3 of the way along the box.
   */

  redline_x = MOUSE_SPEED_BOX_WIDTH / 3;

  if (speed >= mouse_threshold)
    {
      /* Undo acceleration and get back the original mouse speed */

      original_speed = speed - mouse_threshold;
      original_speed = (int)(original_speed / mouse_acceleration);
      original_speed += mouse_threshold;

      linelen = (original_speed * redline_x) / mouse_threshold;
    }
  else
    {
      linelen = (speed * redline_x) / mouse_threshold;
    }

  /* Horizontal "thermometer" */

  if (linelen > MOUSE_SPEED_BOX_WIDTH - 1)
    {
      linelen = MOUSE_SPEED_BOX_WIDTH - 1;
    }

  if (linelen < redline_x)
    {
      v_draw_horiz_line(MOUSE_SPEED_BOX_X + 1,
                        MOUSE_SPEED_BOX_Y + MOUSE_SPEED_BOX_HEIGHT / 2,
                        linelen, white);
    }
  else
    {
      v_draw_horiz_line(MOUSE_SPEED_BOX_X + 1,
                        MOUSE_SPEED_BOX_Y + MOUSE_SPEED_BOX_HEIGHT / 2,
                        redline_x, white);
      v_draw_horiz_line(MOUSE_SPEED_BOX_X + redline_x,
                        MOUSE_SPEED_BOX_Y + MOUSE_SPEED_BOX_HEIGHT / 2,
                        linelen - redline_x, yellow);
    }

  /* Draw acceleration threshold line */

  v_draw_vert_line(MOUSE_SPEED_BOX_X + redline_x, MOUSE_SPEED_BOX_Y + 1,
                   MOUSE_SPEED_BOX_HEIGHT - 2, red);
}

static void draw_non_accelerating_box(int speed)
{
  int white;
  int linelen;

  white = i_get_palette_index(0xff, 0xff, 0xff);

  if (speed > max_seen_speed)
    {
      max_seen_speed = speed;
    }

  /* Draw horizontal "thermometer": */

  linelen = speed * (MOUSE_SPEED_BOX_WIDTH - 1) / max_seen_speed;

  v_draw_horiz_line(MOUSE_SPEED_BOX_X + 1,
                    MOUSE_SPEED_BOX_Y + MOUSE_SPEED_BOX_HEIGHT / 2, linelen,
                    white);
}

static void write_pcx_file(char *filename, pixel_t *data, int width,
                           int height, byte *palette)
{
  int i;
  int length;
  pcx_t *pcx;
  byte *pack;

  pcx = z_malloc(width * height * 2 + 1000, PU_STATIC, NULL);

  pcx->manufacturer = 0x0a; /* PCX id */
  pcx->version = 5;         /* 256 color */
  pcx->encoding = 1;        /* uncompressed */
  pcx->bits_per_pixel = 8;  /* 256 color */
  pcx->xmin = 0;
  pcx->ymin = 0;
  pcx->xmax = SHORT(width - 1);
  pcx->ymax = SHORT(height - 1);
  pcx->hres = SHORT(1);
  pcx->vres = SHORT(1);
  memset(pcx->palette, 0, sizeof(pcx->palette));
  pcx->reserved = 0;     /* PCX spec: reserved byte must be zero */
  pcx->color_planes = 1; /* chunky image */
  pcx->bytes_per_line = SHORT(width);
  pcx->palette_type = SHORT(2); /* not a grey scale */
  memset(pcx->filler, 0, sizeof(pcx->filler));

  /* pack the image */

  pack = &pcx->data;

  for (i = 0; i < width * height; i++)
    {
      if ((*data & 0xc0) != 0xc0)
        *pack++ = *data++;
      else
        {
          *pack++ = 0xc1;
          *pack++ = *data++;
        }
    }

  /* write the palette */

  *pack++ = 0x0c; /* palette ID byte */
  for (i = 0; i < 768; i++)
    *pack++ = *palette++;

  /* write output file */

  length = pack - (byte *)pcx;
  m_write_file(filename, pcx, length);

  z_free(pcx);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void v_mark_rect(int x, int y, int width, int height)
{
  /* If we are temporarily using an alternate screen, do not affect the
   * update box.
   */

  if (dest_screen == i_video_buffer)
    {
      m_add_to_box(dirtybox, x, y);
      m_add_to_box(dirtybox, x + width - 1, y + height - 1);
    }
}

void v_copy_rect(int srcx, int srcy, pixel_t *source, int width, int height,
                 int destx, int desty)
{
  pixel_t *src;
  pixel_t *dest;

#ifdef CONFIG_GAMES_NXDOOM_RANGECHECK
  if (srcx < 0 || srcx + width > SCREENWIDTH || srcy < 0 ||
      srcy + height > SCREENHEIGHT || destx < 0 ||
      destx + width > SCREENWIDTH || desty < 0 ||
      desty + height > SCREENHEIGHT)
    {
      i_error("Bad v_copy_rect");
    }
#endif

  v_mark_rect(destx, desty, width, height);

  src = source + SCREENWIDTH * srcy + srcx;
  dest = dest_screen + SCREENWIDTH * desty + destx;

  for (; height > 0; height--)
    {
      memcpy(dest, src, width * sizeof(*dest));
      src += SCREENWIDTH;
      dest += SCREENWIDTH;
    }
}

/* v_set_patch_clip_callback
 *
 * haleyjd 08/28/10: Added for Strife support.
 * By calling this function, you can setup runtime error checking for patch
 * clipping. Strife never caused errors by drawing patches partway
 * off-screen.
 *
 * Some versions of vanilla DOOM also behaved differently than the default
 * implementation, so this could possibly be extended to those as well for
 * accurate emulation.
 */

void v_set_patch_clip_callback(vpatchclipfunc_t func)
{
  patchclip_callback = func;
}

/* V_DrawPatch
 * Masks a column based masked pic to the screen.
 */

void v_draw_patch(int x, int y, patch_t *patch)
{
  int count;
  int col;
  column_t *column;
  pixel_t *desttop;
  pixel_t *dest;
  byte *source;
  int w;

  y -= SHORT(patch->topoffset);
  x -= SHORT(patch->leftoffset);

  /* haleyjd 08/28/10: Strife needs silent error checking here. */

  if (patchclip_callback)
    {
      if (!patchclip_callback(patch, x, y)) return;
    }

#ifdef CONFIG_GAMES_NXDOOM_RANGECHECK
  if (x < 0 || x + SHORT(patch->width) > SCREENWIDTH || y < 0 ||
      y + SHORT(patch->height) > SCREENHEIGHT)
    {
      i_error("Bad V_DrawPatch");
    }
#endif

  v_mark_rect(x, y, SHORT(patch->width), SHORT(patch->height));

  col = 0;
  desttop = dest_screen + y * SCREENWIDTH + x;

  w = SHORT(patch->width);

  for (; col < w; x++, col++, desttop++)
    {
      column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));

      /* step through the posts in a column */

      while (column->topdelta != 0xff)
        {
          source = (byte *)column + 3;
          dest = desttop + column->topdelta * SCREENWIDTH;
          count = column->length;

          while (count--)
            {
              *dest = *source++;
              dest += SCREENWIDTH;
            }

          column = (column_t *)((byte *)column + column->length + 4);
        }
    }
}

/* v_draw_patch_flipped
 * Masks a column based masked pic to the screen.
 * Flips horizontally, e.g. to mirror face.
 */

void v_draw_patch_flipped(int x, int y, patch_t *patch)
{
  int count;
  int col;
  column_t *column;
  pixel_t *desttop;
  pixel_t *dest;
  byte *source;
  int w;

  y -= SHORT(patch->topoffset);
  x -= SHORT(patch->leftoffset);

  /* haleyjd 08/28/10: Strife needs silent error checking here. */

  if (patchclip_callback)
    {
      if (!patchclip_callback(patch, x, y)) return;
    }

#ifdef CONFIG_GAMES_NXDOOM_RANGECHECK
  if (x < 0 || x + SHORT(patch->width) > SCREENWIDTH || y < 0 ||
      y + SHORT(patch->height) > SCREENHEIGHT)
    {
      i_error("Bad v_draw_patch_flipped");
    }
#endif

  v_mark_rect(x, y, SHORT(patch->width), SHORT(patch->height));

  col = 0;
  desttop = dest_screen + y * SCREENWIDTH + x;

  w = SHORT(patch->width);

  for (; col < w; x++, col++, desttop++)
    {
      column =
          (column_t *)((byte *)patch + LONG(patch->columnofs[w - 1 - col]));

      /* step through the posts in a column */

      while (column->topdelta != 0xff)
        {
          source = (byte *)column + 3;
          dest = desttop + column->topdelta * SCREENWIDTH;
          count = column->length;

          while (count--)
            {
              *dest = *source++;
              dest += SCREENWIDTH;
            }

          column = (column_t *)((byte *)column + column->length + 4);
        }
    }
}

/* v_draw_patch_direct
 * Draws directly to the screen on the pc.
 */

void v_draw_patch_direct(int x, int y, patch_t *patch)
{
  v_draw_patch(x, y, patch);
}

/* v_draw_tl_patch
 * Masks a column based translucent masked pic to the screen.
 */

void v_draw_tl_patch(int x, int y, patch_t *patch)
{
  int count;
  int col;
  column_t *column;
  pixel_t *desttop;
  pixel_t *dest;
  byte *source;
  int w;

  y -= SHORT(patch->topoffset);
  x -= SHORT(patch->leftoffset);

  if (x < 0 || x + SHORT(patch->width) > SCREENWIDTH || y < 0 ||
      y + SHORT(patch->height) > SCREENHEIGHT)
    {
      i_error("Bad v_draw_tl_patch");
    }

  col = 0;
  desttop = dest_screen + y * SCREENWIDTH + x;

  w = SHORT(patch->width);
  for (; col < w; x++, col++, desttop++)
    {
      column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));

      /* step through the posts in a column */

      while (column->topdelta != 0xff)
        {
          source = (byte *)column + 3;
          dest = desttop + column->topdelta * SCREENWIDTH;
          count = column->length;

          while (count--)
            {
              *dest = tinttable[*dest + ((*source++) << 8)];
              dest += SCREENWIDTH;
            }

          column = (column_t *)((byte *)column + column->length + 4);
        }
    }
}

/* v_draw_xla_patch
 * villsa [STRIFE] Masks a column based translucent masked pic to the screen.
 */

void v_draw_xla_patch(int x, int y, patch_t *patch)
{
  int count;
  int col;
  column_t *column;
  pixel_t *desttop, *dest;
  byte *source;
  int w;

  y -= SHORT(patch->topoffset);
  x -= SHORT(patch->leftoffset);

  if (patchclip_callback)
    {
      if (!patchclip_callback(patch, x, y)) return;
    }

  col = 0;
  desttop = dest_screen + y * SCREENWIDTH + x;

  w = SHORT(patch->width);
  for (; col < w; x++, col++, desttop++)
    {
      column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));

      /* step through the posts in a column */

      while (column->topdelta != 0xff)
        {
          source = (byte *)column + 3;
          dest = desttop + column->topdelta * SCREENWIDTH;
          count = column->length;

          while (count--)
            {
              *dest = xlatab[*dest + ((*source) << 8)];
              source++;
              dest += SCREENWIDTH;
            }

          column = (column_t *)((byte *)column + column->length + 4);
        }
    }
}

/* v_draw_alt_tl_patch
 * Masks a column based translucent masked pic to the screen.
 */

void v_draw_alt_tl_patch(int x, int y, patch_t *patch)
{
  int count;
  int col;
  column_t *column;
  pixel_t *desttop;
  pixel_t *dest;
  byte *source;
  int w;

  y -= SHORT(patch->topoffset);
  x -= SHORT(patch->leftoffset);

  if (x < 0 || x + SHORT(patch->width) > SCREENWIDTH || y < 0 ||
      y + SHORT(patch->height) > SCREENHEIGHT)
    {
      i_error("Bad v_draw_alt_tl_patch");
    }

  col = 0;
  desttop = dest_screen + y * SCREENWIDTH + x;

  w = SHORT(patch->width);
  for (; col < w; x++, col++, desttop++)
    {
      column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));

      /* step through the posts in a column */

      while (column->topdelta != 0xff)
        {
          source = (byte *)column + 3;
          dest = desttop + column->topdelta * SCREENWIDTH;
          count = column->length;

          while (count--)
            {
              *dest = tinttable[((*dest) << 8) + *source++];
              dest += SCREENWIDTH;
            }

          column = (column_t *)((byte *)column + column->length + 4);
        }
    }
}

/* v_draw_shadowed_patch
 * Masks a column based masked pic to the screen.
 */

void v_draw_shadowed_patch(int x, int y, patch_t *patch)
{
  int count;
  int col;
  column_t *column;
  pixel_t *desttop;
  pixel_t *dest;
  byte *source;
  pixel_t *desttop2;
  pixel_t *dest2;
  int w;

  y -= SHORT(patch->topoffset);
  x -= SHORT(patch->leftoffset);

  if (x < 0 || x + SHORT(patch->width) > SCREENWIDTH || y < 0 ||
      y + SHORT(patch->height) > SCREENHEIGHT)
    {
      i_error("Bad v_draw_shadowed_patch");
    }

  col = 0;
  desttop = dest_screen + y * SCREENWIDTH + x;
  desttop2 = dest_screen + (y + 2) * SCREENWIDTH + x + 2;

  w = SHORT(patch->width);
  for (; col < w; x++, col++, desttop++, desttop2++)
    {
      column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));

      /* step through the posts in a column */

      while (column->topdelta != 0xff)
        {
          source = (byte *)column + 3;
          dest = desttop + column->topdelta * SCREENWIDTH;
          dest2 = desttop2 + column->topdelta * SCREENWIDTH;
          count = column->length;

          while (count--)
            {
              *dest2 = tinttable[((*dest2) << 8)];
              dest2 += SCREENWIDTH;
              *dest = *source++;
              dest += SCREENWIDTH;
            }

          column = (column_t *)((byte *)column + column->length + 4);
        }
    }
}

/* Load tint table from TINTTAB lump. */

void v_load_tint_table(void)
{
  tinttable = w_cache_lump_name("TINTTAB", PU_STATIC);
}

/* v_load_xla_table
 * villsa [STRIFE] Load xla table from XLATAB lump.
 */

void v_load_xla_table(void)
{
  xlatab = w_cache_lump_name("XLATAB", PU_STATIC);
}

/* v_draw_block
 * Draw a linear block of pixels into the view buffer.
 */

void v_draw_block(int x, int y, int width, int height, pixel_t *src)
{
  pixel_t *dest;

#ifdef CONFIG_GAMES_NXDOOM_RANGECHECK
  if (x < 0 || x + width > SCREENWIDTH || y < 0 || y + height > SCREENHEIGHT)
    {
      i_error("Bad v_draw_block");
    }
#endif

  v_mark_rect(x, y, width, height);

  dest = dest_screen + y * SCREENWIDTH + x;

  while (height--)
    {
      memcpy(dest, src, width * sizeof(*dest));
      src += width;
      dest += SCREENWIDTH;
    }
}

void v_draw_filled_box(int x, int y, int w, int h, int c)
{
  pixel_t *buf, *buf1;
  int x1;
  int y1;

  buf = i_video_buffer + SCREENWIDTH * y + x;

  for (y1 = 0; y1 < h; ++y1)
    {
      buf1 = buf;

      for (x1 = 0; x1 < w; ++x1)
        {
          *buf1++ = c;
        }

      buf += SCREENWIDTH;
    }
}

void v_draw_horiz_line(int x, int y, int w, int c)
{
  pixel_t *buf;
  int x1;

  buf = i_video_buffer + SCREENWIDTH * y + x;

  for (x1 = 0; x1 < w; ++x1)
    {
      *buf++ = c;
    }
}

void v_draw_vert_line(int x, int y, int h, int c)
{
  pixel_t *buf;
  int y1;

  buf = i_video_buffer + SCREENWIDTH * y + x;

  for (y1 = 0; y1 < h; ++y1)
    {
      *buf = c;
      buf += SCREENWIDTH;
    }
}

void v_draw_box(int x, int y, int w, int h, int c)
{
  v_draw_horiz_line(x, y, w, c);
  v_draw_horiz_line(x, y + h - 1, w, c);
  v_draw_vert_line(x, y, h, c);
  v_draw_vert_line(x + w - 1, y, h, c);
}

/* Draw a "raw" screen (lump containing raw data to blit directly
 * to the screen)
 */

void v_draw_raw_screen(pixel_t *raw)
{
  memcpy(dest_screen, raw,
         SCREENWIDTH * SCREENHEIGHT * sizeof(*dest_screen));
}

void v_init(void)
{
  /* no-op!
   * There used to be separate screens that could be drawn to; these are
   * now handled in the upper layers.
   */
}

/* Set the buffer that the code draws to. */

void v_use_buffer(pixel_t *buffer)
{
  dest_screen = buffer;
}

/* Restore screen buffer to the i_video screen buffer. */

void v_restore_buffer(void)
{
  dest_screen = i_video_buffer;
}

void v_screenshot(const char *format)
{
  int i;
  char lbmname[16]; /* haleyjd 20110213: BUG FIX - 12 is too small! */
  const char *ext;

  /* find a file name to save it to */

  ext = "pcx";

  for (i = 0; i <= 99; i++)
    {
      snprintf(lbmname, sizeof(lbmname), format, i, ext);

      if (!m_file_exists(lbmname))
        {
          break; /* file doesn't exist */
        }
    }

  if (i == 100)
    {
      i_error("v_screenshot: Couldn't create a PCX");
    }

  /* save the pcx file */

  write_pcx_file(lbmname, i_video_buffer, SCREENWIDTH, SCREENHEIGHT,
                 w_cache_lump_name(("PLAYPAL"), PU_CACHE));
}

void v_draw_mouse_speed_box(int speed)
{
  int bgcolor;
  int bordercolor;
  int black;

  /* If the mouse is turned off, don't draw the box at all. */

  if (!usemouse)
    {
      return;
    }

  /* Get palette indices for colors for widget. These depend on the
   * palette of the game being played.
   */

  bgcolor = i_get_palette_index(0x77, 0x77, 0x77);
  bordercolor = i_get_palette_index(0x55, 0x55, 0x55);
  black = i_get_palette_index(0x00, 0x00, 0x00);

  /* Calculate box position */

  v_draw_filled_box(MOUSE_SPEED_BOX_X, MOUSE_SPEED_BOX_Y,
                    MOUSE_SPEED_BOX_WIDTH, MOUSE_SPEED_BOX_HEIGHT, bgcolor);
  v_draw_box(MOUSE_SPEED_BOX_X, MOUSE_SPEED_BOX_Y, MOUSE_SPEED_BOX_WIDTH,
             MOUSE_SPEED_BOX_HEIGHT, bordercolor);
  v_draw_horiz_line(MOUSE_SPEED_BOX_X + 1, MOUSE_SPEED_BOX_Y + 4,
                    MOUSE_SPEED_BOX_WIDTH - 2, black);

  /* If acceleration is used, draw a box that helps to calibrate the
   * threshold point.
   */

  if (mouse_threshold > 0 && fabs(mouse_acceleration - 1) > 0.01)
    {
      draw_accelerating_box(speed);
    }
  else
    {
      draw_non_accelerating_box(speed);
    }
}
