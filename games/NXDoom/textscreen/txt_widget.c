/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_widget.c
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

#include "txt_desktop.h"
#include "txt_gui.h"
#include "txt_io.h"
#include "txt_widget.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct
{
  char *signal_name;
  txt_widget_signal_f func;
  void *user_data;
} txt_callback_t;

struct txt_callback_table_s
{
  int refcount;
  txt_callback_t *callbacks;
  int num_callbacks;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int txt_contains_widget(TXT_UNCAST_ARG(haystack),
                               TXT_UNCAST_ARG(needle))
{
  TXT_CAST_ARG(txt_widget_t, haystack);
  TXT_CAST_ARG(txt_widget_t, needle);

  while (needle != NULL)
    {
      if (needle == haystack)
        {
          return 1;
        }

      needle = needle->parent;
    }

  return 0;
}

static txt_callback_table_t *txt_new_callback_table(void)
{
  txt_callback_table_t *table;

  table = malloc(sizeof(txt_callback_table_t));
  table->callbacks = NULL;
  table->num_callbacks = 0;
  table->refcount = 1;

  return table;
}

static void txt_ref_callback_table(txt_callback_table_t *table)
{
  ++table->refcount;
}

static void txt_unref_callback_table(txt_callback_table_t *table)
{
  int i;

  --table->refcount;

  if (table->refcount == 0)
    {
      /* No more references to this table */

      for (i = 0; i < table->num_callbacks; ++i)
        {
          free(table->callbacks[i].signal_name);
        }

      free(table->callbacks);
      free(table);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void txt_init_widget(TXT_UNCAST_ARG(widget),
        txt_widget_class_t *widget_class)
{
  TXT_CAST_ARG(txt_widget_t, widget);

  widget->widget_class = widget_class;
  widget->callback_table = txt_new_callback_table();
  widget->parent = NULL;

  /* Not focused until we hear otherwise. */

  widget->focused = 0;

  /* Visible by default. */

  widget->visible = 1;

  /* Align left by default */

  widget->align = TXT_HORIZ_LEFT;
}

void txt_signal_connect(TXT_UNCAST_ARG(widget), const char *signal_name,
                        txt_widget_signal_f func, void *user_data)
{
  TXT_CAST_ARG(txt_widget_t, widget);
  txt_callback_table_t *table;
  txt_callback_t *callback;

  table = widget->callback_table;

  /* Add a new callback to the table */

  table->callbacks = realloc(
      table->callbacks, sizeof(txt_callback_t) * (table->num_callbacks + 1));
  callback = &table->callbacks[table->num_callbacks];
  ++table->num_callbacks;

  callback->signal_name = strdup(signal_name);
  callback->func = func;
  callback->user_data = user_data;
}

void txt_emit_signal(TXT_UNCAST_ARG(widget), const char *signal_name)
{
  TXT_CAST_ARG(txt_widget_t, widget);
  txt_callback_table_t *table;
  int i;

  table = widget->callback_table;

  /* Don't destroy the table while we're searching through it
   * (one of the callbacks may destroy this window)
   */

  txt_ref_callback_table(table);

  /* Search the table for all callbacks with this name and invoke
   * the functions.
   */

  for (i = 0; i < table->num_callbacks; ++i)
    {
      if (!strcmp(table->callbacks[i].signal_name, signal_name))
        {
          table->callbacks[i].func(widget, table->callbacks[i].user_data);
        }
    }

  /* Finished using the table */

  txt_unref_callback_table(table);
}

void txt_calc_widget_size(TXT_UNCAST_ARG(widget))
{
  TXT_CAST_ARG(txt_widget_t, widget);

  widget->widget_class->size_calc(widget);
}

void txt_draw_widget(TXT_UNCAST_ARG(widget))
{
  TXT_CAST_ARG(txt_widget_t, widget);
  txt_saved_colors_t colors;

  /* The drawing function might change the fg/bg colors,
   * so make sure we restore them after it's done.
   */

  txt_save_colours(&colors);

  /* For convenience... */

  txt_goto_xy(widget->x, widget->y);

  /* Call drawer method */

  widget->widget_class->drawer(widget);

  txt_restore_colours(&colors);
}

void txt_destroy_widget(TXT_UNCAST_ARG(widget))
{
  TXT_CAST_ARG(txt_widget_t, widget);

  widget->widget_class->destructor(widget);
  txt_unref_callback_table(widget->callback_table);
  free(widget);
}

int txt_widget_key_press(TXT_UNCAST_ARG(widget), int key)
{
  TXT_CAST_ARG(txt_widget_t, widget);

  if (widget->widget_class->key_press != NULL)
    {
      return widget->widget_class->key_press(widget, key);
    }

  return 0;
}

void txt_set_widget_focus(TXT_UNCAST_ARG(widget), int focused)
{
  TXT_CAST_ARG(txt_widget_t, widget);

  if (widget == NULL)
    {
      return;
    }

  if (widget->focused != focused)
    {
      widget->focused = focused;

      if (widget->widget_class->focus_change != NULL)
        {
          widget->widget_class->focus_change(widget, focused);
        }
    }
}

void txt_set_widget_align(TXT_UNCAST_ARG(widget),
                          txt_horiz_align_t horiz_align)
{
  TXT_CAST_ARG(txt_widget_t, widget);

  widget->align = horiz_align;
}

void txt_widget_mouse_press(TXT_UNCAST_ARG(widget), int x, int y, int b)
{
  TXT_CAST_ARG(txt_widget_t, widget);

  if (widget->widget_class->mouse_press != NULL)
    {
      widget->widget_class->mouse_press(widget, x, y, b);
    }
}

void txt_layout_widget(TXT_UNCAST_ARG(widget))
{
  TXT_CAST_ARG(txt_widget_t, widget);

  if (widget->widget_class->layout != NULL)
    {
      widget->widget_class->layout(widget);
    }
}

int txt_always_selectable(TXT_UNCAST_ARG(widget))
{
  return 1;
}

int txt_never_selectable(TXT_UNCAST_ARG(widget))
{
  return 0;
}

int txt_selectable_widget(TXT_UNCAST_ARG(widget))
{
  TXT_CAST_ARG(txt_widget_t, widget);

  if (widget->widget_class->selectable != NULL)
    {
      return widget->widget_class->selectable(widget);
    }
  else
    {
      return 0;
    }
}

int txt_hovering_over_widget(TXT_UNCAST_ARG(widget))
{
  TXT_CAST_ARG(txt_widget_t, widget);
  txt_window_t *active_window;
  int x;
  int y;

  /* We can only be hovering over widgets in the active window. */

  active_window = txt_get_active_window();

  if (active_window == NULL || !txt_contains_widget(active_window, widget))
    {
      return 0;
    }

  /* Is the mouse cursor within the bounds of the widget? */

  txt_get_mouse_position(&x, &y);

  return (x >= widget->x && x < widget->x + widget->w && y >= widget->y &&
          y < widget->y + widget->h);
}

void txt_set_widget_bg(TXT_UNCAST_ARG(widget))
{
  TXT_CAST_ARG(txt_widget_t, widget);

  if (widget->focused)
    {
      txt_bgcolour(TXT_COLOR_GREY, 0);
    }
  else if (txt_hovering_over_widget(widget))
    {
      txt_bgcolour(TXT_HOVER_BACKGROUND, 0);
    }
  else
    {
      /* Use normal window background. */
    }
}
