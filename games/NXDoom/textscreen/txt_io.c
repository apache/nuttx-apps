/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_io.c
 *
 * SPDX-License-Identifer: GPLv2
 *
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
 *
 * Text mode I/O functions, similar to C stdio
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "txt_io.h"
#include "txt_main.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_cur_x = 0;
static int g_cur_y = 0;
static txt_color_t g_fgcolour = TXT_COLOR_GREY;
static txt_color_t g_bgcolour = TXT_COLOR_BLACK;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#if 0 /* UNUSED */
static void txt_clear_screen(void)
{
  unsigned char *screen;
  int i;

  screen = txt_get_screen_data();

  for (i = 0; i < TXT_SCREEN_W * TXT_SCREEN_H; ++i)
    {
      screen[i * 2] = ' ';
      screen[i * 2 + 1] = (bgcolor << 4) | fgcolor;
    }

  cur_x = 0;
  cur_y = 0;
}
#endif

static void new_line(unsigned char *screendata)
{
  int i;
  unsigned char *p;

  g_cur_x = 0;
  ++g_cur_y;

  if (g_cur_y >= TXT_SCREEN_H)
    {
      /* Scroll the screen up */

      g_cur_y = TXT_SCREEN_H - 1;

      memmove(screendata, screendata + TXT_SCREEN_W * 2,
              TXT_SCREEN_W * 2 * (TXT_SCREEN_H - 1));

      /* Clear the bottom line */

      p = screendata + (TXT_SCREEN_H - 1) * 2 * TXT_SCREEN_W;

      for (i = 0; i < TXT_SCREEN_W; ++i)
        {
          *p++ = ' ';
          *p++ = g_fgcolour | (g_bgcolour << 4);
        }
    }
}

static void put_symbol(unsigned char *screendata, int c)
{
  unsigned char *p;

  p = screendata + g_cur_y * TXT_SCREEN_W * 2 + g_cur_x * 2;

  /* Add a new character to the buffer */

  p[0] = c;
  p[1] = g_fgcolour | (g_bgcolour << 4);

  ++g_cur_x;

  if (g_cur_x >= TXT_SCREEN_W)
    {
      new_line(screendata);
    }
}

static void put_char(unsigned char *screendata, int c)
{
  switch (c)
    {
    case '\n':
      new_line(screendata);
      break;

    case '\b':

      /* backspace */

      --g_cur_x;
      if (g_cur_x < 0) g_cur_x = 0;
      break;

    default:
      put_symbol(screendata, c);
      break;
    }
}

/* "Blind" version of txt_putchar() below which doesn't do any interpretation
 * of control signals. Just write a particular symbol to the screen buffer.
 */

void txt_put_symbol(int c)
{
  put_symbol(txt_get_screen_data(), c);
}

void txt_putchar(int c)
{
  put_char(txt_get_screen_data(), c);
}

void txt_puts(const char *s)
{
  unsigned char *screen;
  const char *p;

  screen = txt_get_screen_data();

  for (p = s; *p != '\0'; ++p)
    {
      put_char(screen, *p);
    }

  put_char(screen, '\n');
}

void txt_goto_xy(int x, int y)
{
  g_cur_x = x;
  g_cur_y = y;
}

void txt_get_xy(int *x, int *y)
{
  *x = g_cur_x;
  *y = g_cur_y;
}

void txt_fgcolour(txt_color_t color)
{
  g_fgcolour = color;
}

void txt_bgcolour(int color, int blinking)
{
  g_bgcolour = color;
  if (blinking) g_bgcolour |= TXT_COLOR_BLINKING;
}

void txt_save_colours(txt_saved_colors_t *save)
{
  save->bgcolor = g_bgcolour;
  save->fgcolor = g_fgcolour;
}

void txt_restore_colours(txt_saved_colors_t *save)
{
  g_bgcolour = save->bgcolor;
  g_fgcolour = save->fgcolor;
}
