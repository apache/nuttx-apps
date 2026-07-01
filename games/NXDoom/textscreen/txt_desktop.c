/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_desktop.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "doomkeys.h"

#include "txt_desktop.h"
#include "txt_gui.h"
#include "txt_io.h"
#include "txt_main.h"
#include "txt_separator.h"
#include "txt_window.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define HELP_KEY KEY_F1
#define MAXWINDOWS 128

/****************************************************************************
 * Private Data
 ****************************************************************************/

static char *g_desktop_title;
static txt_window_t *g_all_windows[MAXWINDOWS];
static int g_num_windows = 0;
static int g_main_loop_running = 0;

static txt_idle_callback_f g_periodic_callback = NULL;
static void *g_periodic_callback_data;
static unsigned int g_periodic_callback_period;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void txt_exit_mainloop(void)
{
  g_main_loop_running = 0;
}

#if 0 /* UNUSED */
static int txt_raise_window(txt_window_t *window)
{
  int i;

  for (i = 0; i < g_num_windows - 1; ++i)
    {
      if (g_all_windows[i] == window)
        {
          g_all_windows[i] = g_all_windows[i + 1];
          g_all_windows[i + 1] = window;

          if (i == g_num_windows - 2)
            {
              txt_set_window_focus(g_all_windows[i], 0);
              txt_set_window_focus(window, 1);
            }

          return 1;
        }
    }

  /* Window not in the list, or at the end of the list (top) already. */

  return 0;
}
#endif

static void draw_desktop_background(const char *title)
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

  txt_putchar(' ');
  txt_puts(title);
}

static void draw_help_indicator(void)
{
  char keybuf[10];
  int fgcolor;
  int x;
  int y;

  txt_get_key_description(HELP_KEY, keybuf, sizeof(keybuf));

  txt_get_mouse_position(&x, &y);

  if (y == 0 && x >= TXT_SCREEN_W - 9)
    {
      fgcolor = TXT_COLOR_GREY;
      txt_bgcolour(TXT_COLOR_BLACK, 0);
    }
  else
    {
      fgcolor = TXT_COLOR_BLACK;
      txt_bgcolour(TXT_COLOR_GREY, 0);
    }

  txt_goto_xy(TXT_SCREEN_W - 9, 0);

  txt_fgcolour(TXT_COLOR_BRIGHT_WHITE);
  txt_draw_string(" ");
  txt_draw_string(keybuf);

  txt_fgcolour(fgcolor);
  txt_draw_string("=Help ");
}

static void desktop_input_event(int c)
{
  txt_window_t *active_window;
  int x;
  int y;

  switch (c)
    {
    case TXT_MOUSE_LEFT:
      txt_get_mouse_position(&x, &y);

      /* Clicking the top-right of the screen is equivalent
       * to pressing the help key.
       */

      if (y == 0 && x >= TXT_SCREEN_W - 9)
        {
          desktop_input_event(HELP_KEY);
        }
      break;

    case HELP_KEY:
      active_window = txt_get_active_window();
      if (active_window != NULL)
        {
          txt_open_window_help_url(active_window);
        }
      break;

    default:
      break;
    }
}

#if 0 /* UNUSED */
static void txt_draw_ascii_table(void)
{
  unsigned char *screendata;
  char buf[10];
  int x;
  int y;
  int n;

  screendata = txt_get_screen_data();

  txt_fgcolour(TXT_COLOR_BRIGHT_WHITE);
  txt_bgcolour(TXT_COLOR_BLACK, 0);

  for (y = 0; y < 16; ++y)
    {
      for (x = 0; x < 16; ++x)
        {
          n = y * 16 + x;

          txt_goto_xy(x * 5, y);
          txt_snprintf(buf, sizeof(buf), "%02x   ", n);
          txt_puts(buf);

          /* Write the character directly to the screen memory buffer: */

          screendata[(y * TXT_SCREEN_W + x * 5 + 3) * 2] = n;
        }
    }

  txt_update_screen();
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void txt_add_desktop_window(txt_window_t *win)
{
  /* Previously-top window loses focus: */

  if (g_num_windows > 0)
    {
      txt_set_window_focus(g_all_windows[g_num_windows - 1], 0);
    }

  g_all_windows[g_num_windows] = win;
  ++g_num_windows;

  /* New window gains focus: */

  txt_set_window_focus(win, 1);
}

void txt_remove_desktop_window(txt_window_t *win)
{
  int from;
  int to;

  /* Window must lose focus if it's being removed: */

  txt_set_window_focus(win, 0);

  for (from = 0, to = 0; from < g_num_windows; ++from)
    {
      if (g_all_windows[from] != win)
        {
          g_all_windows[to] = g_all_windows[from];
          ++to;
        }
    }

  g_num_windows = to;

  /* Top window gains focus: */

  if (g_num_windows > 0)
    {
      txt_set_window_focus(g_all_windows[g_num_windows - 1], 1);
    }
}

txt_window_t *txt_get_active_window(void)
{
  if (g_num_windows == 0)
    {
      return NULL;
    }

  return g_all_windows[g_num_windows - 1];
}

int txt_lower_window(txt_window_t *window)
{
  int i;

  for (i = 0; i < g_num_windows - 1; ++i)
    {
      if (g_all_windows[i + 1] == window)
        {
          g_all_windows[i + 1] = g_all_windows[i];
          g_all_windows[i] = window;

          if (i == g_num_windows - 2)
            {
              txt_set_window_focus(window, 0);
              txt_set_window_focus(g_all_windows[i + 1], 1);
            }

          return 1;
        }
    }

  /* Window not in the list, or at the start of the list (bottom) already. */

  return 0;
}

void txt_set_desktop_title(const char *title)
{
  free(g_desktop_title);
  g_desktop_title = strdup(title);
  txt_set_window_title(title);
}

void txt_draw_desktop(void)
{
  txt_window_t *active_window;
  const char *title;
  int i;

  txt_init_clip_area();

  if (g_desktop_title == NULL)
    title = "";
  else
    title = g_desktop_title;

  draw_desktop_background(title);

  active_window = txt_get_active_window();
  if (active_window != NULL && active_window->help_url != NULL)
    {
      draw_help_indicator();
    }

  for (i = 0; i < g_num_windows; ++i)
    {
      txt_draw_window(g_all_windows[i]);
    }

  txt_update_screen();
}

/* Fallback function to handle key/mouse events that are not handled by
 * the active window.
 */

void txt_dispatch_events(void)
{
  txt_window_t *active_window;
  int c;

  while ((c = txt_getchar()) > 0)
    {
      active_window = txt_get_active_window();

      if (active_window != NULL && !txt_window_keypress(active_window, c))
        {
          desktop_input_event(c);
        }
    }
}

void txt_set_periodic_callback(txt_idle_callback_f callback, void *user_data,
                               unsigned int period)
{
  g_periodic_callback = callback;
  g_periodic_callback_data = user_data;
  g_periodic_callback_period = period;
}

void txt_gui_mainloop(void)
{
  g_main_loop_running = 1;

  while (g_main_loop_running)
    {
      txt_dispatch_events();

      /* After the last window is closed, exit the loop */

      if (g_num_windows <= 0)
        {
          txt_exit_mainloop();
          continue;
        }

      txt_draw_desktop();

      /*        TXT_DrawASCIITable(); */

      if (g_periodic_callback == NULL)
        {
          txt_sleep(0);
        }
      else
        {
          txt_sleep(g_periodic_callback_period);

          g_periodic_callback(g_periodic_callback_data);
        }
    }
}
