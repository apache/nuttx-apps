/****************************************************************************
 * apps/games/NXDoom/src/doom/f_wipe.c
 *
 * SPDX-License-Identifer: GPLv2
 *
 * Copyright(C) 1993-1996 Id Software, Inc.
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
 *  Mission begin melt/wipe screen special effect.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <string.h>

#include "i_video.h"
#include "m_random.h"
#include "v_video.h"
#include "z_zone.h"

#include "doomtype.h"

#include "f_wipe.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void wipe_shitty_col_major_xform(dpixel_t *array, int width,
                                        int height);
static int wip_init_colour_xform(int width, int height, int ticks);
static int wipe_do_colour_xform(int width, int height, int ticks);
static int wip_exit_color_xform(int width, int height, int ticks);
static int wipe_init_melt(int width, int height, int ticks);
static int wipe_do_melt(int width, int height, int ticks);
static int wipe_exit_melt(int width, int height, int ticks);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* when zero, stop the wipe */

static boolean go = 0;

static pixel_t *wipe_scr_start;
static pixel_t *wipe_scr_end;
static pixel_t *wipe_scr;

static int *g_y;

static int (*g_wipes[])(int, int, int) =
{
  wip_init_colour_xform, wipe_do_colour_xform, wip_exit_color_xform,
  wipe_init_melt,        wipe_do_melt,         wipe_exit_melt,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void wipe_shitty_col_major_xform(dpixel_t *array, int width,
                                        int height)
{
  int x;
  int y;
  dpixel_t *dest;

  dest = (dpixel_t *)z_malloc(width * height * sizeof(*dest), PU_STATIC, 0);

  for (y = 0; y < height; y++)
    {
      for (x = 0; x < width; x++)
        {
          dest[x * height + y] = array[y * width + x];
        }
    }

  memcpy(array, dest, width * height * sizeof(*dest));

  z_free(dest);
}

static int wip_init_colour_xform(int width, int height, int ticks)
{
  memcpy(wipe_scr, wipe_scr_start, width * height * sizeof(*wipe_scr));
  return 0;
}

static int wipe_do_colour_xform(int width, int height, int ticks)
{
  boolean changed;
  pixel_t *w;
  pixel_t *e;
  int newval;

  changed = false;
  w = wipe_scr;
  e = wipe_scr_end;

  while (w != wipe_scr + width * height)
    {
      if (*w != *e)
        {
          if (*w > *e)
            {
              newval = *w - ticks;
              if (newval < *e)
                *w = *e;
              else
                *w = newval;
              changed = true;
            }
          else if (*w < *e)
            {
              newval = *w + ticks;
              if (newval > *e)
                *w = *e;
              else
                *w = newval;
              changed = true;
            }
        }

      w++;
      e++;
    }

  return !changed;
}

static int wip_exit_color_xform(int width, int height, int ticks)
{
  return 0;
}

static int wipe_init_melt(int width, int height, int ticks)
{
  int i;
  int r;

  /* copy start screen to main screen */

  memcpy(wipe_scr, wipe_scr_start, width * height * sizeof(*wipe_scr));

  /* makes this wipe faster (in theory)
   * to have stuff in column-major format
   */

  wipe_shitty_col_major_xform((dpixel_t *)wipe_scr_start, width / 2, height);
  wipe_shitty_col_major_xform((dpixel_t *)wipe_scr_end, width / 2, height);

  /* setup initial column positions
   * (y<0 => not ready to scroll yet)
   */

  g_y = (int *)z_malloc(width * sizeof(int), PU_STATIC, 0);
  g_y[0] = -(m_random() % 16);
  for (i = 1; i < width; i++)
    {
      r = (m_random() % 3) - 1;
      g_y[i] = g_y[i - 1] + r;
      if (g_y[i] > 0)
        g_y[i] = 0;
      else if (g_y[i] == -16)
        g_y[i] = -15;
    }

  return 0;
}

static int wipe_do_melt(int width, int height, int ticks)
{
  int i;
  int j;
  int dy;
  int idx;

  dpixel_t *s;
  dpixel_t *d;
  boolean done = true;

  width /= 2;

  while (ticks--)
    {
      for (i = 0; i < width; i++)
        {
          if (g_y[i] < 0)
            {
              g_y[i]++;
              done = false;
            }
          else if (g_y[i] < height)
            {
              dy = (g_y[i] < 16) ? g_y[i] + 1 : 8;
              if (g_y[i] + dy >= height) dy = height - g_y[i];
              s = &((dpixel_t *)wipe_scr_end)[i * height + g_y[i]];
              d = &((dpixel_t *)wipe_scr)[g_y[i] * width + i];
              idx = 0;
              for (j = dy; j; j--)
                {
                  d[idx] = *(s++);
                  idx += width;
                }

              g_y[i] += dy;
              s = &((dpixel_t *)wipe_scr_start)[i * height];
              d = &((dpixel_t *)wipe_scr)[g_y[i] * width + i];
              idx = 0;
              for (j = height - g_y[i]; j; j--)
                {
                  d[idx] = *(s++);
                  idx += width;
                }

              done = false;
            }
        }
    }

  return done;
}

static int wipe_exit_melt(int width, int height, int ticks)
{
  z_free(g_y);
  z_free(wipe_scr_start);
  z_free(wipe_scr_end);
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int wipe_start_screen(int x, int y, int width, int height)
{
  wipe_scr_start = z_malloc(
      SCREENWIDTH * SCREENHEIGHT * sizeof(*wipe_scr_start), PU_STATIC, NULL);
  i_read_screen(wipe_scr_start);
  return 0;
}

int wipe_endscreen(int x, int y, int width, int height)
{
  wipe_scr_end = z_malloc(SCREENWIDTH * SCREENHEIGHT * sizeof(*wipe_scr_end),
                          PU_STATIC, NULL);
  i_read_screen(wipe_scr_end);
  v_draw_block(x, y, width, height, wipe_scr_start); /* restore start scr. */
  return 0;
}

int wipe_screen_wipe(int wipeno, int x, int y, int width, int height,
                     int ticks)
{
  int rc;

  /* initial stuff */

  if (!go)
    {
      go = 1;
      wipe_scr = i_video_buffer;
      (*g_wipes[wipeno * 3])(width, height, ticks);
    }

  /* do a piece of wipe-in */

  v_mark_rect(0, 0, width, height);
  rc = (*g_wipes[wipeno * 3 + 1])(width, height, ticks);

  /* final stuff */

  if (rc)
    {
      go = 0;
      (*g_wipes[wipeno * 3 + 2])(width, height, ticks);
    }

  return !go;
}
