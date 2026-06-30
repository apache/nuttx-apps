/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_label.h
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

#ifndef TXT_LABEL_H
#define TXT_LABEL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "txt_main.h"
#include "txt_widget.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

/**
 * Label widget.
 *
 * A label widget does nothing except show a text label.
 */

typedef struct txt_label_s txt_label_t;

struct txt_label_s
{
  txt_widget_t widget;
  char *label;
  char **lines;
  unsigned int w;
  unsigned int h;
  int fgcolor;
  int bgcolor;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * Create a new label widget.
 *
 * @param label         String to display in the widget (UTF-8 format).
 * @return              Pointer to the new label widget.
 */

txt_label_t *txt_new_label(const char *label);

/**
 * Set the string displayed in a label widget.
 *
 * @param label         The widget.
 * @param value         The string to display (UTF-8 format).
 */

void txt_set_label(txt_label_t *label, const char *value);

/**
 * Set the foreground color of a label widget.
 *
 * @param label         The widget.
 * @param color         The foreground color to use.
 */

void txt_set_fg_colour(txt_label_t *label, txt_color_t color);

#endif /* TXT_LABEL_H */
