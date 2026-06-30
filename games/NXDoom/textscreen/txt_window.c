/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_window.c
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "doomkeys.h"

#include "txt_desktop.h"
#include "txt_gui.h"
#include "txt_io.h"
#include "txt_label.h"
#include "txt_main.h"
#include "txt_separator.h"
#include "txt_window.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void calc_window_position(txt_window_t *window)
{
  switch (window->horiz_align)
    {
    case TXT_HORIZ_LEFT:
      window->window_x = window->x;
      break;
    case TXT_HORIZ_CENTER:
      window->window_x = window->x - (window->window_w / 2);
      break;
    case TXT_HORIZ_RIGHT:
      window->window_x = window->x - (window->window_w - 1);
      break;
    }

  switch (window->vert_align)
    {
    case TXT_VERT_TOP:
      window->window_y = window->y;
      break;
    case TXT_VERT_CENTER:
      window->window_y = window->y - (window->window_h / 2);
      break;
    case TXT_VERT_BOTTOM:
      window->window_y = window->y - (window->window_h - 1);
      break;
    }
}

static void layout_action_area(txt_window_t *window)
{
  txt_widget_t *widget;
  int space_available;
  int space_left_offset;

  /* We need to calculate the available horizontal space for the center
   * action widget, so that we can center it within it.
   * To start with, we have the entire action area available.
   */

  space_available = window->window_w;
  space_left_offset = 0;

  /* Left action */

  if (window->actions[TXT_HORIZ_LEFT] != NULL)
    {
      widget = window->actions[TXT_HORIZ_LEFT];

      txt_calc_widget_size(widget);

      widget->x = window->window_x + 1;
      widget->y = window->window_y + window->window_h - widget->h - 1;

      /* Adjust available space: */

      space_available -= widget->w;
      space_left_offset += widget->w;

      txt_layout_widget(widget);
    }

  /* Draw the right action */

  if (window->actions[TXT_HORIZ_RIGHT] != NULL)
    {
      widget = window->actions[TXT_HORIZ_RIGHT];

      txt_calc_widget_size(widget);

      widget->x = window->window_x + window->window_w - 1 - widget->w;
      widget->y = window->window_y + window->window_h - widget->h - 1;

      /* Adjust available space: */

      space_available -= widget->w;

      txt_layout_widget(widget);
    }

  /* Draw the center action */

  if (window->actions[TXT_HORIZ_CENTER] != NULL)
    {
      widget = window->actions[TXT_HORIZ_CENTER];

      txt_calc_widget_size(widget);

      /* The left and right widgets have left a space sandwiched between
       * them.  Center this widget within that space.
       */

      widget->x = window->window_x + space_left_offset +
                  (space_available - widget->w) / 2;
      widget->y = window->window_y + window->window_h - widget->h - 1;

      txt_layout_widget(widget);
    }
}

static void draw_action_area(txt_window_t *window)
{
  int i;

  for (i = 0; i < 3; ++i)
    {
      if (window->actions[i] != NULL)
        {
          txt_draw_widget(window->actions[i]);
        }
    }
}

static void calc_action_area_size(txt_window_t *window, unsigned int *w,
                                  unsigned int *h)
{
  txt_widget_t *widget;
  int i;

  *w = 0;
  *h = 0;

  /* Calculate the width of all the action widgets and use this
   * to create an overall min. width of the action area
   */

  for (i = 0; i < 3; ++i)
    {
      widget = (txt_widget_t *)window->actions[i];

      if (widget != NULL)
        {
          txt_calc_widget_size(widget);
          *w += widget->w;

          if (widget->h > *h)
            {
              *h = widget->h;
            }
        }
    }
}

/* Sets size and position of all widgets in a window */

static void txt_layout_window(txt_window_t *window)
{
  txt_widget_t *widgets = (txt_widget_t *)window;
  unsigned int widgets_w;
  unsigned int actionarea_w;
  unsigned int actionarea_h;

  /* Calculate size of table */

  txt_calc_widget_size(window);

  /* Widgets area: add one character of padding on each side */

  widgets_w = widgets->w + 2;

  /* Calculate the size of the action area
   * Make window wide enough to action area
   */

  calc_action_area_size(window, &actionarea_w, &actionarea_h);

  if (actionarea_w > widgets_w) widgets_w = actionarea_w;

  /* Set the window size based on widgets_w */

  window->window_w = widgets_w + 2;
  window->window_h = widgets->h + 1;

  /* If the window has a title, add an extra two lines */

  if (window->title != NULL)
    {
      window->window_h += 2;
    }

  /* If the window has an action area, add extra lines */

  if (actionarea_h > 0)
    {
      window->window_h += actionarea_h + 1;
    }

  /* Use the x,y position as the centerpoint and find the location to
   * draw the window.
   */

  calc_window_position(window);

  /* Set the table size and position */

  widgets->w = widgets_w - 2;

  /* widgets->h        (already set) */

  widgets->x = window->window_x + 2;
  widgets->y = window->window_y;

  if (window->title != NULL)
    {
      widgets->y += 2;
    }

  /* Layout the table and action area */

  layout_action_area(window);
  txt_layout_widget(widgets);
}

static void txt_open_url(const char *url)
{
#if 0
  char *cmd;
  size_t cmd_len;
  int retval;

  cmd_len = strlen(url) + 30;
  cmd = malloc(cmd_len);

  /* The Unix situation sucks as usual, but the closest thing to a
   * standard that exists is the xdg-utils package.
   */

  if (system("xdg-open --version 2>/dev/null") != 0)
    {
      fprintf(stderr,
              "xdg-utils is not installed. Can't open this URL:\n%s\n", url);
      free(cmd);
      return;
    }

  txt_snprintf(cmd, cmd_len, "xdg-open \"%s\"", url);

  retval = system(cmd);
  if (retval != 0)
    {
      fprintf(stderr, "txt_open_url: error executing '%s'; return code %d\n",
              cmd, retval);
    }

  free(cmd);
#endif
}

static int mouse_button_press(txt_window_t *window, int b)
{
  int x;
  int y;
  int i;
  txt_widget_t *widgets;
  txt_widget_t *widget;

  /* Lay out the window, set positions and sizes of all widgets */

  txt_layout_window(window);

  /* Get the current mouse position */

  txt_get_mouse_position(&x, &y);

  /* Try the mouse button listener
   * This happens whether it is in the window range or not
   */

  if (window->mouse_listener != NULL)
    {
      /* Mouse listener can eat button presses */

      if (window->mouse_listener(window, x, y, b,
                                 window->mouse_listener_data))
        {
          return 1;
        }
    }

  /* Is it within the table range? */

  widgets = (txt_widget_t *)window;

  if (x >= widgets->x && x < (signed)(widgets->x + widgets->w) &&
      y >= widgets->y && y < (signed)(widgets->y + widgets->h))
    {
      txt_widget_mouse_press(window, x, y, b);
      return 1;
    }

  /* Was one of the action area buttons pressed? */

  for (i = 0; i < 3; ++i)
    {
      widget = window->actions[i];

      if (widget != NULL && x >= widget->x &&
          x < (signed)(widget->x + widget->w) && y >= widget->y &&
          y < (signed)(widget->y + widget->h))
        {
          int was_focused;

          /* Main table temporarily loses focus when action area button
           * is clicked. This way, any active input boxes that depend
           * on having focus will save their values before the
           * action is performed.
           */

          was_focused = window->table.widget.focused;
          txt_set_widget_focus(window, 0);
          txt_set_widget_focus(window, was_focused);

          /* Pass through mouse press. */

          txt_widget_mouse_press(widget, x, y, b);
          return 1;
        }
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void txt_set_window_action(txt_window_t *window, txt_horiz_align_t position,
                           TXT_UNCAST_ARG(action))
{
  TXT_CAST_ARG(txt_widget_t, action);

  if (window->actions[position] != NULL)
    {
      txt_destroy_widget(window->actions[position]);
    }

  window->actions[position] = action;

  /* Maintain parent pointer. */

  if (action != NULL)
    {
      action->parent = &window->table.widget;
    }
}

txt_window_t *txt_new_window(const char *title)
{
  int i;

  txt_window_t *win;

  win = malloc(sizeof(txt_window_t));

  txt_init_table(&win->table, 1);

  if (title == NULL)
    {
      win->title = NULL;
    }
  else
    {
      win->title = strdup(title);
    }

  win->x = TXT_SCREEN_W / 2;
  win->y = TXT_SCREEN_H / 2;
  win->horiz_align = TXT_HORIZ_CENTER;
  win->vert_align = TXT_VERT_CENTER;
  win->key_listener = NULL;
  win->mouse_listener = NULL;
  win->help_url = NULL;

  txt_add_widget(win, txt_new_separator(NULL));

  for (i = 0; i < 3; ++i)
    {
      win->actions[i] = NULL;
    }

  txt_add_desktop_window(win);

  /* Default actions */

  txt_set_window_action(win, TXT_HORIZ_LEFT,
                        txt_new_window_escape_action(win));
  txt_set_window_action(win, TXT_HORIZ_RIGHT,
                        txt_new_window_select_action(win));

  return win;
}

void txt_close_window(txt_window_t *window)
{
  int i;

  txt_emit_signal(window, "closed");
  txt_remove_desktop_window(window);

  free(window->title);

  /* Destroy all actions */

  for (i = 0; i < 3; ++i)
    {
      if (window->actions[i] != NULL)
        {
          txt_destroy_widget(window->actions[i]);
        }
    }

  /* Destroy table and window */

  txt_destroy_widget(window);
}

void txt_draw_window(txt_window_t *window)
{
  txt_widget_t *widgets;

  txt_layout_window(window);

  if (window->table.widget.focused)
    {
      txt_bgcolour(TXT_ACTIVE_WINDOW_BACKGROUND, 0);
    }
  else
    {
      txt_bgcolour(TXT_INACTIVE_WINDOW_BACKGROUND, 0);
    }

  txt_fgcolour(TXT_COLOR_BRIGHT_WHITE);

  /* Draw the window */

  txt_draw_window_frame(window->title, window->window_x, window->window_y,
                      window->window_w, window->window_h);

  /* Draw all widgets */

  txt_draw_widget(window);

  /* Draw an action area, if we have one */

  widgets = (txt_widget_t *)window;

  if (widgets->y + widgets->h < window->window_y + window->window_h - 1)
    {
      /* Separator for action area */

      txt_draw_separator(window->window_x, widgets->y + widgets->h,
                        window->window_w);

      /* Action area at the window bottom */

      draw_action_area(window);
    }
}

void txt_set_window_position(txt_window_t *window,
                             txt_horiz_align_t horiz_align,
                             txt_vert_align_t vert_align, int x, int y)
{
  window->vert_align = vert_align;
  window->horiz_align = horiz_align;
  window->x = x;
  window->y = y;
}

int txt_window_keypress(txt_window_t *window, int c)
{
  int i;

  /* Is this a mouse button ? */

  if (c >= TXT_MOUSE_BASE && c < TXT_MOUSE_BASE + TXT_MAX_MOUSE_BUTTONS)
    {
      return mouse_button_press(window, c);
    }

  /* Try the window key spy */

  if (window->key_listener != NULL)
    {
      /* key listener can eat keys */

      if (window->key_listener(window, c, window->key_listener_data))
        {
          return 1;
        }
    }

  /* Send to the currently selected widget: */

  if (txt_widget_key_press(window, c))
    {
      return 1;
    }

  /* Try all of the action buttons */

  for (i = 0; i < 3; ++i)
    {
      if (window->actions[i] != NULL &&
          txt_widget_key_press(window->actions[i], c))
        {
          return 1;
        }
    }

  return 0;
}

void txt_set_key_listener(txt_window_t *window,
                          txt_window_keypress_t key_listener,
                          void *user_data)
{
  window->key_listener = key_listener;
  window->key_listener_data = user_data;
}

void txt_set_mouse_listener(txt_window_t *window,
                            txt_window_mouse_press_t mouse_listener,
                            void *user_data)
{
  window->mouse_listener = mouse_listener;
  window->mouse_listener_data = user_data;
}

void txt_set_window_focus(txt_window_t *window, int focused)
{
  txt_set_widget_focus(window, focused);
}

void txt_set_window_help_url(txt_window_t *window, const char *help_url)
{
  window->help_url = help_url;
}

void txt_open_window_help_url(txt_window_t *window)
{
  if (window->help_url != NULL)
    {
      txt_open_url(window->help_url);
    }
}

txt_window_t *txt_message_box(const char *title, const char *message, ...)
{
  txt_window_t *window;
  char buf[256];
  va_list args;

  va_start(args, message);
  txt_vsnprintf(buf, sizeof(buf), message, args);
  va_end(args);

  window = txt_new_window(title);
  txt_add_widget(window, txt_new_label(buf));

  txt_set_window_action(window, TXT_HORIZ_LEFT, NULL);
  txt_set_window_action(window, TXT_HORIZ_CENTER,
                        txt_new_window_escape_action(window));
  txt_set_window_action(window, TXT_HORIZ_RIGHT, NULL);

  return window;
}
