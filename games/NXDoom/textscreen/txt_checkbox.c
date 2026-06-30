/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_checkbox.c
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

#include "txt_checkbox.h"
#include "txt_gui.h"
#include "txt_io.h"
#include "txt_main.h"
#include "txt_utf8.h"
#include "txt_window.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void txt_checkbox_size_calc(TXT_UNCAST_ARG(checkbox));
static void txt_checkbox_drawer(TXT_UNCAST_ARG(checkbox));
static void txt_checkbox_destructor(TXT_UNCAST_ARG(checkbox));
static int txt_check_box_keypress(TXT_UNCAST_ARG(checkbox), int key);
static void txt_checkbox_mousepress(TXT_UNCAST_ARG(checkbox), int x, int y,
                                   int b);

/****************************************************************************
 * Public Data
 ****************************************************************************/

txt_widget_class_t txt_checkbox_class =
{
  txt_always_selectable,
  txt_checkbox_size_calc,
  txt_checkbox_drawer,
  txt_check_box_keypress,
  txt_checkbox_destructor,
  txt_checkbox_mousepress,
  NULL,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void txt_checkbox_size_calc(TXT_UNCAST_ARG(checkbox))
{
  TXT_CAST_ARG(txt_checkbox_t, checkbox);

  /* Minimum width is the string length + right-side space for padding */

  checkbox->widget.w = txt_utf8_strlen(checkbox->label) + 5;
  checkbox->widget.h = 1;
}

static void txt_checkbox_drawer(TXT_UNCAST_ARG(checkbox))
{
  TXT_CAST_ARG(txt_checkbox_t, checkbox);
  txt_saved_colors_t colors;
  int i;
  int w;

  w = checkbox->widget.w;

  txt_save_colours(&colors);
  txt_fgcolour(TXT_COLOR_BRIGHT_CYAN);
  txt_draw_string("(");

  txt_fgcolour(TXT_COLOR_BRIGHT_WHITE);

  if ((*checkbox->variable != 0) ^ checkbox->inverted)
    {
      txt_draw_code_page_string("\x07");
    }
  else
    {
      txt_draw_string(" ");
    }

  txt_fgcolour(TXT_COLOR_BRIGHT_CYAN);

  txt_draw_string(") ");

  txt_restore_colours(&colors);
  txt_set_widget_bg(checkbox);
  txt_draw_string(checkbox->label);

  for (i = txt_utf8_strlen(checkbox->label); i < w - 4; ++i)
    {
      txt_draw_string(" ");
    }
}

static void txt_checkbox_destructor(TXT_UNCAST_ARG(checkbox))
{
  TXT_CAST_ARG(txt_checkbox_t, checkbox);

  free(checkbox->label);
}

static int txt_check_box_keypress(TXT_UNCAST_ARG(checkbox), int key)
{
  TXT_CAST_ARG(txt_checkbox_t, checkbox);

  if (key == KEY_ENTER || key == ' ')
    {
      *checkbox->variable = !*checkbox->variable;
      txt_emit_signal(checkbox, "changed");
      return 1;
    }

  return 0;
}

static void txt_checkbox_mousepress(TXT_UNCAST_ARG(checkbox), int x, int y,
                                    int b)
{
  TXT_CAST_ARG(txt_checkbox_t, checkbox);

  if (b == TXT_MOUSE_LEFT)
    {
      /* Equivalent to pressing enter */

      txt_check_box_keypress(checkbox, KEY_ENTER);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

txt_checkbox_t *txt_new_check_box(const char *label, int *variable)
{
  txt_checkbox_t *checkbox;

  checkbox = malloc(sizeof(txt_checkbox_t));

  txt_init_widget(checkbox, &txt_checkbox_class);
  checkbox->label = strdup(label);
  checkbox->variable = variable;
  checkbox->inverted = 0;

  return checkbox;
}

txt_checkbox_t *txt_new_inverted_checkbox(const char *label, int *variable)
{
  txt_checkbox_t *result;

  result = txt_new_check_box(label, variable);
  result->inverted = 1;

  return result;
}
