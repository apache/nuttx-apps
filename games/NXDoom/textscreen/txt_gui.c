/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_gui.c
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "txt_gui.h"
#include "txt_io.h"
#include "txt_main.h"
#include "txt_utf8.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define VALID_X(x) ((x) >= g_cliparea->x1 && (x) < g_cliparea->x2)
#define VALID_Y(y) ((y) >= g_cliparea->y1 && (y) < g_cliparea->y2)

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct txt_cliparea_s txt_cliparea_t;

struct txt_cliparea_s
{
  int x1;
  int x2;
  int y1;
  int y2;
  txt_cliparea_t *next;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Array of border characters for drawing windows. The array looks like this:
 *
 * +-++
 * | ||
 * +-++
 * +-++
 */

static const int g_borders[4][4] =
{
  {0xda, 0xc4, 0xc2, 0xbf},
  {0xb3, ' ', 0xb3, 0xb3},
  {0xc3, 0xc4, 0xc5, 0xb4},
  {0xc0, 0xc4, 0xc1, 0xd9},
};

static txt_cliparea_t *g_cliparea = NULL;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

#if 0 /* UNUSED */
static void txt_draw_desktop_background(const char *title)
{
  int i;
  unsigned char *screendata;
  unsigned char *p;

  screendata = txt_get_screen_data();

  /* Fill the screen with gradient characters */

  p = screendata;

  for (i = 0; i < TXT_SCREEN_W * TXT_SCREEN_H; ++i)
    {
      *p++ = 0xb1;
      *p++ = TXT_COLOR_GREY | (TXT_COLOR_BLUE << 4);
    }

  /* Draw the top and bottom banners */

  p = screendata;

  for (i = 0; i < TXT_SCREEN_W; ++i)
    {
      *p++ = ' ';
      *p++ = TXT_COLOR_BLACK | (TXT_COLOR_GREY << 4);
    }

  p = screendata + (TXT_SCREEN_H - 1) * TXT_SCREEN_W * 2;

  for (i = 0; i < TXT_SCREEN_W; ++i)
    {
      *p++ = ' ';
      *p++ = TXT_COLOR_BLACK | (TXT_COLOR_GREY << 4);
    }

  /* Print the title */

  txt_goto_xy(0, 0);
  txt_fgcolour(TXT_COLOR_BLACK);
  txt_bgcolour(TXT_COLOR_GREY, 0);

  txt_draw_string(" ");
  txt_draw_string(title);
}
#endif

static void txt_draw_shadow(int x, int y, int w, int h)
{
  unsigned char *screendata;
  unsigned char *p;
  int x1;
  int y1;

  screendata = txt_get_screen_data();

  for (y1 = y; y1 < y + h; ++y1)
    {
      p = screendata + (y1 * TXT_SCREEN_W + x) * 2;

      for (x1 = x; x1 < x + w; ++x1)
        {
          if (VALID_X(x1) && VALID_Y(y1))
            {
              p[1] = TXT_COLOR_DARK_GREY;
            }

          p += 2;
        }
    }
}

static void put_unicode_char(unsigned int c)
{
  int d;

  /* Treat control characters specially. */

  if (c == '\n' || c == '\b')
    {
      txt_putchar(c);
      return;
    }

  /* Map Unicode character into the symbol used to represent it in this
   * code page. For unrepresentable characters, print a fallback instead.
   * Note that we use txt_put_symbol() here because we just want to do a
   * raw write into the screen buffer.
   */

  d = txt_unicode_character(c);

  if (d >= 0)
    {
      txt_put_symbol(d);
    }
  else
    {
      txt_put_symbol('\xa8');
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void txt_draw_window_frame(const char *title, int x, int y, int w, int h)
{
  txt_saved_colors_t colors;
  int x1;
  int y1;
  int bx;
  int by;

  txt_save_colours(&colors);
  txt_fgcolour(TXT_COLOR_BRIGHT_CYAN);

  for (y1 = y; y1 < y + h; ++y1)
    {
      /* Select the appropriate row and column in the borders
       * array to pick the appropriate character to draw at
       * this location.
       *
       * Draw a horizontal line on the third line down, so we
       * draw a box around the title.
       */

      by = y1 == y                        ? 0
           : y1 == y + 2 && title != NULL ? 2
           : y1 == y + h - 1              ? 3
                                          : 1;

      for (x1 = x; x1 < x + w; ++x1)
        {
          bx = x1 == x ? 0 : x1 == x + w - 1 ? 3 : 1;

          if (VALID_X(x1) && VALID_Y(y1))
            {
              txt_goto_xy(x1, y1);
              txt_putchar(g_borders[by][bx]);
            }
        }
    }

  /* Draw the title */

  if (title != NULL)
    {
      txt_goto_xy(x + 1, y + 1);
      txt_bgcolour(TXT_COLOR_GREY, 0);
      txt_fgcolour(TXT_COLOR_BLUE);

      for (x1 = 0; x1 < w - 2; ++x1)
        {
          txt_draw_string(" ");
        }

      txt_goto_xy(x + (w - txt_utf8_strlen(title)) / 2, y + 1);
      txt_draw_string(title);
    }

  /* Draw the window's shadow. */

  txt_draw_shadow(x + 2, y + h, w, 1);
  txt_draw_shadow(x + w, y + 1, 2, h);

  txt_restore_colours(&colors);
}

void txt_draw_separator(int x, int y, int w)
{
  txt_saved_colors_t colors;
  unsigned char *data;
  int x1;
  int b;

  data = txt_get_screen_data();

  txt_save_colours(&colors);
  txt_fgcolour(TXT_COLOR_BRIGHT_CYAN);

  if (!VALID_Y(y))
    {
      return;
    }

  data += (y * TXT_SCREEN_W + x) * 2;

  for (x1 = x; x1 < x + w; ++x1)
    {
      txt_goto_xy(x1, y);

      b = x1 == x ? 0 : x1 == x + w - 1 ? 3 : 1;

      if (VALID_X(x1))
        {
          /* Read the current value from the screen
           * Check that it matches what the window should look like if
           * there is no separator, then apply the separator
           */

          if (*data == g_borders[1][b])
            {
              txt_putchar(g_borders[2][b]);
            }
        }

      data += 2;
    }

  txt_restore_colours(&colors);
}

/* Alternative to txt_draw_string() where the argument is a "code page
 * string" - characters are in native code page format and not UTF-8.
 */

void txt_draw_code_page_string(const char *s)
{
  int x;
  int y;
  int x1;
  const char *p;

  txt_get_xy(&x, &y);

  if (VALID_Y(y))
    {
      x1 = x;

      for (p = s; *p != '\0'; ++p)
        {
          if (VALID_X(x1))
            {
              txt_goto_xy(x1, y);
              txt_putchar(*p);
            }

          x1 += 1;
        }
    }

  txt_goto_xy(x + strlen(s), y);
}

void txt_draw_string(const char *s)
{
  int x;
  int y;
  int x1;
  const char *p;
  unsigned int c;

  txt_get_xy(&x, &y);

  if (VALID_Y(y))
    {
      x1 = x;

      for (p = s; *p != '\0'; )
        {
          c = txt_decode_utf8(&p);

          if (c == 0)
            {
              break;
            }

          if (VALID_X(x1))
            {
              txt_goto_xy(x1, y);
              put_unicode_char(c);
            }

          x1 += 1;
        }
    }

  txt_goto_xy(x + txt_utf8_strlen(s), y);
}

void txt_draw_horiz_scrollbar(int x, int y, int w, int cursor, int range)
{
  txt_saved_colors_t colors;
  int x1;
  int cursor_x;

  if (!VALID_Y(y))
    {
      return;
    }

  txt_save_colours(&colors);
  txt_fgcolour(TXT_COLOR_BLACK);
  txt_bgcolour(TXT_COLOR_GREY, 0);

  txt_goto_xy(x, y);
  txt_putchar('\x1b');

  cursor_x = x + 1;

  if (range > 0)
    {
      cursor_x += (cursor * (w - 3)) / range;
    }

  if (cursor_x > x + w - 2)
    {
      cursor_x = x + w - 2;
    }

  for (x1 = x + 1; x1 < x + w - 1; ++x1)
    {
      if (VALID_X(x1))
        {
          if (x1 == cursor_x)
            {
              txt_putchar('\xdb');
            }
          else
            {
              txt_putchar('\xb1');
            }
        }
    }

  txt_putchar('\x1a');
  txt_restore_colours(&colors);
}

void txt_draw_vert_scrollbar(int x, int y, int h, int cursor, int range)
{
  txt_saved_colors_t colors;
  int y1;
  int cursor_y;

  if (!VALID_X(x))
    {
      return;
    }

  txt_save_colours(&colors);
  txt_fgcolour(TXT_COLOR_BLACK);
  txt_bgcolour(TXT_COLOR_GREY, 0);

  txt_goto_xy(x, y);
  txt_putchar('\x18');

  cursor_y = y + 1;

  if (cursor_y > y + h - 2)
    {
      cursor_y = y + h - 2;
    }

  if (range > 0)
    {
      cursor_y += (cursor * (h - 3)) / range;
    }

  for (y1 = y + 1; y1 < y + h - 1; ++y1)
    {
      if (VALID_Y(y1))
        {
          txt_goto_xy(x, y1);

          if (y1 == cursor_y)
            {
              txt_putchar('\xdb');
            }
          else
            {
              txt_putchar('\xb1');
            }
        }
    }

  txt_goto_xy(x, y + h - 1);
  txt_putchar('\x19');
  txt_restore_colours(&colors);
}

void txt_init_clip_area(void)
{
  if (g_cliparea == NULL)
    {
      g_cliparea = malloc(sizeof(txt_cliparea_t));
      g_cliparea->x1 = 0;
      g_cliparea->x2 = TXT_SCREEN_W;
      g_cliparea->y1 = 0;
      g_cliparea->y2 = TXT_SCREEN_H;
      g_cliparea->next = NULL;
    }
}

void txt_push_clip_area(int x1, int x2, int y1, int y2)
{
  txt_cliparea_t *newarea;

  newarea = malloc(sizeof(txt_cliparea_t));

  /* Set the new clip area to the intersection of the old
   * area and the new one.
   */

  newarea->x1 = g_cliparea->x1;
  newarea->x2 = g_cliparea->x2;
  newarea->y1 = g_cliparea->y1;
  newarea->y2 = g_cliparea->y2;

  if (x1 > newarea->x1) newarea->x1 = x1;
  if (x2 < newarea->x2) newarea->x2 = x2;
  if (y1 > newarea->y1) newarea->y1 = y1;
  if (y2 < newarea->y2) newarea->y2 = y2;

#if 0
    printf("New scrollable area: %i,%i-%i,%i\n", x1, y1, x2, y2);
#endif

  /* Hook into the list */

  newarea->next = g_cliparea;
  g_cliparea = newarea;
}

void txt_pop_clip_area(void)
{
  txt_cliparea_t *next_cliparea;

  /* Never pop the last entry */

  if (g_cliparea->next == NULL) return;

  /* Unlink the last entry and delete */

  next_cliparea = g_cliparea->next;
  free(g_cliparea);
  g_cliparea = next_cliparea;
}
