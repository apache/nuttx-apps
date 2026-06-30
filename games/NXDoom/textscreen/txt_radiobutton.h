/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_radiobutton.h
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

#ifndef TXT_RADIOBUTTON_H
#define TXT_RADIOBUTTON_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "txt_widget.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* A radio button widget.
 *
 * Radio buttons are typically used in groups, to allow a value to be
 * selected from a range of options.  Each radio button corresponds
 * to a particular option that may be selected.  A radio button
 * has an indicator to indicate whether it is the currently-selected
 * value, and a text label.
 *
 * Internally, a radio button tracks an integer variable that may take
 * a range of different values.  Each radio button has a particular
 * value associated with it; if the variable is equal to that value,
 * the radio button appears selected.  If a radio button is pressed
 * by the user through the GUI, the variable is set to its value.
 *
 * When a radio button is selected, the "selected" signal is emitted.
 */

struct txt_radiobutton_s
{
  txt_widget_t widget;
  char *label;
  int *variable;
  int value;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * Create a new radio button widget.
 *
 * @param label          The label to display next to the radio button
 *                       (UTF-8 format).
 * @param variable       Pointer to the variable tracking whether this
 *                       radio button is selected.
 * @param value          If the variable is equal to this value, the
 *                       radio button appears selected.
 * @return               Pointer to the new radio button widget.
 */

txt_radiobutton_t *txt_new_radio_button(const char *label, int *variable,
                                        int value);

#endif /* TXT_RADIOBUTTON_H */
