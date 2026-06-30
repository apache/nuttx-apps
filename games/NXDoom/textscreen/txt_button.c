/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_button.c
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
#include "txt_gui.h"
#include "txt_io.h"
#include "txt_main.h"
#include "txt_utf8.h"
#include "txt_window.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

static void txt_button_size_calc(TXT_UNCAST_ARG(button));
static void txt_button_drawer(TXT_UNCAST_ARG(button));
static void txt_button_destructor(TXT_UNCAST_ARG(button));
static int txt_button_keypress(TXT_UNCAST_ARG(button), int key);
static void txt_button_mousepress(TXT_UNCAST_ARG(button), int x, int y,
                                  int b);

/****************************************************************************
 * Public Data
 ****************************************************************************/

txt_widget_class_t txt_button_class =
{
  txt_always_selectable,
  txt_button_size_calc,
  txt_button_drawer,
  txt_button_keypress,
  txt_button_destructor,
  txt_button_mousepress,
  NULL,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void txt_button_size_calc(TXT_UNCAST_ARG(button))
{
  TXT_CAST_ARG(txt_button_t, button);

  button->widget.w = txt_utf8_strlen(button->label);
  button->widget.h = 1;
}

static void txt_button_drawer(TXT_UNCAST_ARG(button))
{
  TXT_CAST_ARG(txt_button_t, button);
  int i;
  int w;

  w = button->widget.w;

  txt_set_widget_bg(button);

  txt_draw_string(button->label);

  for (i = txt_utf8_strlen(button->label); i < w; ++i)
    {
      txt_draw_string(" ");
    }
}

static void txt_button_destructor(TXT_UNCAST_ARG(button))
{
  TXT_CAST_ARG(txt_button_t, button);

  free(button->label);
}

static int txt_button_keypress(TXT_UNCAST_ARG(button), int key)
{
  TXT_CAST_ARG(txt_button_t, button);

  if (key == KEY_ENTER)
    {
      txt_emit_signal(button, "pressed");
      return 1;
    }

  return 0;
}

static void txt_button_mousepress(TXT_UNCAST_ARG(button), int x, int y,
        int b)
{
  TXT_CAST_ARG(txt_button_t, button);

  if (b == TXT_MOUSE_LEFT)
    {
      /* Equivalent to pressing enter */

      txt_button_keypress(button, KEY_ENTER);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void txt_set_button_label(txt_button_t *button, const char *label)
{
  free(button->label);
  button->label = strdup(label);
}

txt_button_t *txt_new_button(const char *label)
{
  txt_button_t *button;

  button = malloc(sizeof(txt_button_t));

  txt_init_widget(button, &txt_button_class);
  button->label = strdup(label);

  return button;
}

/* Button with a callback set automatically */

txt_button_t *txt_new_button2(const char *label, txt_widget_signal_f func,
                              void *user_data)
{
  txt_button_t *button;

  button = txt_new_button(label);

  txt_signal_connect(button, "pressed", func, user_data);

  return button;
}
