/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_radiobutton.c
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

#include "txt_gui.h"
#include "txt_io.h"
#include "txt_main.h"
#include "txt_radiobutton.h"
#include "txt_utf8.h"
#include "txt_window.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

static void txt_radio_button_size_calc(TXT_UNCAST_ARG(radiobutton));
static void txt_radio_button_drawer(TXT_UNCAST_ARG(radiobutton));
static void txt_radio_button_destructor(TXT_UNCAST_ARG(radiobutton));
static int txt_radio_button_keypress(TXT_UNCAST_ARG(radiobutton), int key);
static void txt_radio_button_mousepress(TXT_UNCAST_ARG(radiobutton), int x,
                                        int y, int b);

/****************************************************************************
 * Public Data
 ****************************************************************************/

txt_widget_class_t txt_radiobutton_class =
{
  txt_always_selectable,
  txt_radio_button_size_calc,
  txt_radio_button_drawer,
  txt_radio_button_keypress,
  txt_radio_button_destructor,
  txt_radio_button_mousepress,
  NULL,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void txt_radio_button_size_calc(TXT_UNCAST_ARG(radiobutton))
{
  TXT_CAST_ARG(txt_radiobutton_t, radiobutton);

  /* Minimum width is the string length + right-side spaces for padding */

  radiobutton->widget.w = txt_utf8_strlen(radiobutton->label) + 5;
  radiobutton->widget.h = 1;
}

static void txt_radio_button_drawer(TXT_UNCAST_ARG(radiobutton))
{
  TXT_CAST_ARG(txt_radiobutton_t, radiobutton);
  txt_saved_colors_t colors;
  int i;
  int w;

  w = radiobutton->widget.w;

  txt_save_colours(&colors);
  txt_fgcolour(TXT_COLOR_BRIGHT_CYAN);
  txt_draw_string("(");

  txt_fgcolour(TXT_COLOR_BRIGHT_WHITE);

  if (*radiobutton->variable == radiobutton->value)
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
  txt_set_widget_bg(radiobutton);

  txt_draw_string(radiobutton->label);

  for (i = txt_utf8_strlen(radiobutton->label); i < w - 5; ++i)
    {
      txt_draw_string(" ");
    }
}

static void txt_radio_button_destructor(TXT_UNCAST_ARG(radiobutton))
{
  TXT_CAST_ARG(txt_radiobutton_t, radiobutton);

  free(radiobutton->label);
}

static int txt_radio_button_keypress(TXT_UNCAST_ARG(radiobutton), int key)
{
  TXT_CAST_ARG(txt_radiobutton_t, radiobutton);

  if (key == KEY_ENTER || key == ' ')
    {
      if (*radiobutton->variable != radiobutton->value)
        {
          *radiobutton->variable = radiobutton->value;
          txt_emit_signal(radiobutton, "selected");
        }

      return 1;
    }

  return 0;
}

static void txt_radio_button_mousepress(TXT_UNCAST_ARG(radiobutton), int x,
                                      int y, int b)
{
  TXT_CAST_ARG(txt_radiobutton_t, radiobutton);

  if (b == TXT_MOUSE_LEFT)
    {
      /* Equivalent to pressing enter */

      txt_radio_button_keypress(radiobutton, KEY_ENTER);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

txt_radiobutton_t *txt_new_radio_button(const char *label, int *variable,
                                        int value)
{
  txt_radiobutton_t *radiobutton;

  radiobutton = malloc(sizeof(txt_radiobutton_t));

  txt_init_widget(radiobutton, &txt_radiobutton_class);
  radiobutton->label = strdup(label);
  radiobutton->variable = variable;
  radiobutton->value = value;

  return radiobutton;
}

#if 0 /* UNUSED */
static void txt_set_radio_button_label(txt_radiobutton_t *radiobutton,
                                       const char *value)
{
  free(radiobutton->label);
  radiobutton->label = strdup(value);
}
#endif
