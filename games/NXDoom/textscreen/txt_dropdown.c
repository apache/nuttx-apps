/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_dropdown.c
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

#include "doomkeys.h"

#include "txt_button.h"
#include "txt_dropdown.h"
#include "txt_gui.h"
#include "txt_io.h"
#include "txt_main.h"
#include "txt_utf8.h"
#include "txt_window.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct
{
  txt_window_t *window;
  txt_dropdown_list_t *list;
  int item;
} callback_data_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void txt_dropdown_list_size_calc(TXT_UNCAST_ARG(list));
static void txt_dropdown_list_drawer(TXT_UNCAST_ARG(list));
static void txt_dropdown_list_destructor(TXT_UNCAST_ARG(list));
static int txt_dropdown_lis_keypress(TXT_UNCAST_ARG(list), int key);
static void txt_dropdown_list_mousepress(TXT_UNCAST_ARG(list), int x, int y,
                                         int b);
/****************************************************************************
 * Public Data
 ****************************************************************************/

txt_widget_class_t txt_dropdown_list_class =
{
  txt_always_selectable,
  txt_dropdown_list_size_calc,
  txt_dropdown_list_drawer,
  txt_dropdown_lis_keypress,
  txt_dropdown_list_destructor,
  txt_dropdown_list_mousepress,
  NULL,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Check if the selected value for a list is valid */

static int valid_selection(txt_dropdown_list_t *list)
{
  return *list->variable >= 0 && *list->variable < list->num_values;
}

/* Calculate the Y position for the selector window */

static int selector_window_y(txt_dropdown_list_t *list)
{
  int result;

  if (valid_selection(list))
    {
      result = list->widget.y - 1 - *list->variable;
    }
  else
    {
      result = list->widget.y - 1 - (list->num_values / 2);
    }

  /* Keep dropdown inside the screen. */

  if (result < 1)
    {
      result = 1;
    }
  else if (result + list->num_values > (TXT_SCREEN_H - 3))
    {
      result = TXT_SCREEN_H - list->num_values - 3;
    }

  return result;
}

/* Called when a button in the selector window is pressed */

static void item_selected(TXT_UNCAST_ARG(button),
                          TXT_UNCAST_ARG(callback_data))
{
  TXT_CAST_ARG(callback_data_t, callback_data);

  /* Set the variable */

  *callback_data->list->variable = callback_data->item;

  txt_emit_signal(callback_data->list, "changed");

  /* Close the window */

  txt_close_window(callback_data->window);
}

/* Free callback data when the window is closed */

static void free_callback_data(TXT_UNCAST_ARG(list),
                               TXT_UNCAST_ARG(callback_data))
{
  TXT_CAST_ARG(callback_data_t, callback_data);
  free(callback_data);
}

/* Catch presses of escape and close the window. */

static int selector_window_listener(txt_window_t *window, int key,
                                    void *user_data)
{
  if (key == KEY_ESCAPE)
    {
      txt_close_window(window);
      return 1;
    }

  return 0;
}

static int selector_mouse_listener(txt_window_t *window, int x, int y, int b,
                                   void *unused)
{
  txt_widget_t *win;

  win = (txt_widget_t *)window;

  if (x < win->x || x > win->x + win->w || y < win->y || y > win->y + win->h)
    {
      txt_close_window(window);
      return 1;
    }

  return 0;
}

/* Open the dropdown list window to select an item */

static void open_selector_window(txt_dropdown_list_t *list)
{
  txt_window_t *window;
  int i;

  /* Open a simple window with no title bar or action buttons. */

  window = txt_new_window(NULL);

  txt_set_window_action(window, TXT_HORIZ_LEFT, NULL);
  txt_set_window_action(window, TXT_HORIZ_CENTER, NULL);
  txt_set_window_action(window, TXT_HORIZ_RIGHT, NULL);

  /* Position the window so that the currently selected item appears
   * over the top of the list widget.
   */

  txt_set_window_position(window, TXT_HORIZ_LEFT, TXT_VERT_TOP,
                          list->widget.x - 2, selector_window_y(list));

  /* Add a button to the window for each option in the list. */

  for (i = 0; i < list->num_values; ++i)
    {
      txt_button_t *button;
      callback_data_t *data;

      button = txt_new_button(list->values[i]);

      txt_add_widget(window, button);

      /* Callback struct */

      data = malloc(sizeof(callback_data_t));
      data->list = list;
      data->window = window;
      data->item = i;

      /* When the button is pressed, invoke the button press callback */

      txt_signal_connect(button, "pressed", item_selected, data);

      /* When the window is closed, free back the callback struct */

      txt_signal_connect(window, "closed", free_callback_data, data);

      /* Is this the currently-selected value?  If so, select the button
       * in the window as the default.
       */

      if (i == *list->variable)
        {
          txt_select_widget(window, button);
        }
    }

  /* Catch presses of escape in this window and close it. */

  txt_set_key_listener(window, selector_window_listener, NULL);
  txt_set_mouse_listener(window, selector_mouse_listener, NULL);
}

static int dropdown_list_width(txt_dropdown_list_t *list)
{
  int i;
  int result;

  /* Find the maximum string width */

  result = 0;

  for (i = 0; i < list->num_values; ++i)
    {
      int w = txt_utf8_strlen(list->values[i]);
      if (w > result)
        {
          result = w;
        }
    }

  return result;
}

static void txt_dropdown_list_size_calc(TXT_UNCAST_ARG(list))
{
  TXT_CAST_ARG(txt_dropdown_list_t, list);

  list->widget.w = dropdown_list_width(list);
  list->widget.h = 1;
}

static void txt_dropdown_list_drawer(TXT_UNCAST_ARG(list))
{
  TXT_CAST_ARG(txt_dropdown_list_t, list);
  unsigned int i;
  const char *str;

  /* Set bg/fg text colors. */

  txt_set_widget_bg(list);

  /* Select a string to draw from the list, if the current value is
   * in range.  Otherwise fall back to a default.
   */

  if (valid_selection(list))
    {
      str = list->values[*list->variable];
    }
  else
    {
      str = "???";
    }

  /* Draw the string and fill to the end with spaces */

  txt_draw_string(str);

  for (i = txt_utf8_strlen(str); i < list->widget.w; ++i)
    {
      txt_draw_string(" ");
    }
}

static void txt_dropdown_list_destructor(TXT_UNCAST_ARG(list))
{
}

static int txt_dropdown_lis_keypress(TXT_UNCAST_ARG(list), int key)
{
  TXT_CAST_ARG(txt_dropdown_list_t, list);

  if (key == KEY_ENTER)
    {
      open_selector_window(list);
      return 1;
    }

  return 0;
}

static void txt_dropdown_list_mousepress(TXT_UNCAST_ARG(list), int x, int y,
                                         int b)
{
  TXT_CAST_ARG(txt_dropdown_list_t, list);

  /* Left mouse click does the same as selecting and pressing enter */

  if (b == TXT_MOUSE_LEFT)
    {
      txt_dropdown_lis_keypress(list, KEY_ENTER);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

txt_dropdown_list_t *txt_new_dropdown_list(int *variable,
        const char **values, int num_values)
{
  txt_dropdown_list_t *list;

  list = malloc(sizeof(txt_dropdown_list_t));

  txt_init_widget(list, &txt_dropdown_list_class);
  list->variable = variable;
  list->values = values;
  list->num_values = num_values;

  return list;
}
