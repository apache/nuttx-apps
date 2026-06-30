/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_separator.c
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
#include "txt_separator.h"
#include "txt_utf8.h"
#include "txt_window.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void txt_separator_size_calc(TXT_UNCAST_ARG(separator));
static void txt_separator_drawer(TXT_UNCAST_ARG(separator));
static void txt_separator_destructor(TXT_UNCAST_ARG(separator));

/****************************************************************************
 * Public Data
 ****************************************************************************/

txt_widget_class_t txt_separator_class =
{
  txt_never_selectable,
  txt_separator_size_calc,
  txt_separator_drawer,
  NULL,
  txt_separator_destructor,
  NULL,
  NULL,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void txt_separator_size_calc(TXT_UNCAST_ARG(separator))
{
  TXT_CAST_ARG(txt_separator_t, separator);

  if (separator->label != NULL)
    {
      /* Minimum width is the string length + two spaces for padding */

      separator->widget.w = txt_utf8_strlen(separator->label) + 2;
    }
  else
    {
      separator->widget.w = 0;
    }

  separator->widget.h = 1;
}

static void txt_separator_drawer(TXT_UNCAST_ARG(separator))
{
  TXT_CAST_ARG(txt_separator_t, separator);
  int x;
  int y;
  int w;

  w = separator->widget.w;

  txt_get_xy(&x, &y);

  /* Draw separator.  Go back one character and draw two extra
   * to overlap the window borders.
   */

  txt_draw_separator(x - 2, y, w + 4);

  if (separator->label != NULL)
    {
      txt_goto_xy(x, y);

      txt_fgcolour(TXT_COLOR_BRIGHT_GREEN);
      txt_draw_string(" ");
      txt_draw_string(separator->label);
      txt_draw_string(" ");
    }
}

static void txt_separator_destructor(TXT_UNCAST_ARG(separator))
{
  TXT_CAST_ARG(txt_separator_t, separator);

  free(separator->label);
}

static void txt_set_separator_label(txt_separator_t *separator,
                                    const char *label)
{
  free(separator->label);

  if (label != NULL)
    {
      separator->label = strdup(label);
    }
  else
    {
      separator->label = NULL;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

txt_separator_t *txt_new_separator(const char *label)
{
  txt_separator_t *separator;

  separator = malloc(sizeof(txt_separator_t));

  txt_init_widget(separator, &txt_separator_class);

  separator->label = NULL;
  txt_set_separator_label(separator, label);

  return separator;
}
