/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_window_action.c
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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "doomkeys.h"

#include "txt_gui.h"
#include "txt_io.h"
#include "txt_main.h"
#include "txt_utf8.h"
#include "txt_window.h"
#include "txt_window_action.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void txt_window_action_size_calc(TXT_UNCAST_ARG(action));
static void txt_window_action_drawer(TXT_UNCAST_ARG(action));
static void txt_window_action_destructor(TXT_UNCAST_ARG(action));
static int txt_window_action_key_press(TXT_UNCAST_ARG(action), int key);
static void txt_window_action_mouse_press(TXT_UNCAST_ARG(action), int x,
                                          int y, int b);

/****************************************************************************
 * Public Data
 ****************************************************************************/

txt_widget_class_t txt_window_action_class =
{
  txt_always_selectable,
  txt_window_action_size_calc,
  txt_window_action_drawer,
  txt_window_action_key_press,
  txt_window_action_destructor,
  txt_window_action_mouse_press,
  NULL,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void txt_window_action_size_calc(TXT_UNCAST_ARG(action))
{
  TXT_CAST_ARG(txt_window_action_t, action);
  char buf[10];

  txt_get_key_description(action->key, buf, sizeof(buf));

  /* Width is label length, plus key description length, plus '='
   * and two surrounding spaces.
   */

  action->widget.w =
      txt_utf8_strlen(action->label) + txt_utf8_strlen(buf) + 3;
  action->widget.h = 1;
}

static void txt_window_action_drawer(TXT_UNCAST_ARG(action))
{
  TXT_CAST_ARG(txt_window_action_t, action);
  int hovering;
  char buf[10];

  txt_get_key_description(action->key, buf, sizeof(buf));

  hovering = txt_hovering_over_widget(action);
  txt_set_widget_bg(action);

  txt_draw_string(" ");
  txt_fgcolour(hovering ? TXT_COLOR_BRIGHT_WHITE : TXT_COLOR_BRIGHT_GREEN);
  txt_draw_string(buf);
  txt_fgcolour(TXT_COLOR_BRIGHT_CYAN);
  txt_draw_string("=");

  txt_fgcolour(TXT_COLOR_BRIGHT_WHITE);
  txt_draw_string(action->label);
  txt_draw_string(" ");
}

static void txt_window_action_destructor(TXT_UNCAST_ARG(action))
{
  TXT_CAST_ARG(txt_window_action_t, action);

  free(action->label);
}

static int txt_window_action_key_press(TXT_UNCAST_ARG(action), int key)
{
  TXT_CAST_ARG(txt_window_action_t, action);

  if (tolower(key) == tolower(action->key))
    {
      txt_emit_signal(action, "pressed");
      return 1;
    }

  return 0;
}

static void txt_window_action_mouse_press(TXT_UNCAST_ARG(action), int x,
                                          int y, int b)
{
  TXT_CAST_ARG(txt_window_action_t, action);

  /* Simulate a press of the key */

  if (b == TXT_MOUSE_LEFT)
    {
      txt_window_action_key_press(action, action->key);
    }
}

static void window_close_callback(TXT_UNCAST_ARG(widget),
                                  TXT_UNCAST_ARG(window))
{
  TXT_CAST_ARG(txt_window_t, window);

  txt_close_window(window);
}

static void window_select_callback(TXT_UNCAST_ARG(widget),
                                   TXT_UNCAST_ARG(window))
{
  TXT_CAST_ARG(txt_window_t, window);

  txt_widget_key_press(window, KEY_ENTER);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

txt_window_action_t *txt_new_window_action(int key, const char *label)
{
  txt_window_action_t *action;

  action = malloc(sizeof(txt_window_action_t));

  txt_init_widget(action, &txt_window_action_class);
  action->key = key;
  action->label = strdup(label);

  return action;
}

/* An action with the name "close" the closes the window */

txt_window_action_t *txt_new_window_escape_action(txt_window_t *window)
{
  txt_window_action_t *action;

  action = txt_new_window_action(KEY_ESCAPE, "Close");
  txt_signal_connect(action, "pressed", window_close_callback, window);

  return action;
}

/* Exactly the same as the above, but the button is named "abort" */

txt_window_action_t *txt_new_window_abort_action(txt_window_t *window)
{
  txt_window_action_t *action;

  action = txt_new_window_action(KEY_ESCAPE, "Abort");
  txt_signal_connect(action, "pressed", window_close_callback, window);

  return action;
}

txt_window_action_t *txt_new_window_select_action(txt_window_t *window)
{
  txt_window_action_t *action;

  action = txt_new_window_action(KEY_ENTER, "Select");
  txt_signal_connect(action, "pressed", window_select_callback, window);

  return action;
}
