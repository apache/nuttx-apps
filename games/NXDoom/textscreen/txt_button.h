/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_button.h
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

#ifndef TXT_BUTTON_H
#define TXT_BUTTON_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "txt_widget.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Button widget.
 *
 * A button is a widget that can be selected to perform some action.
 * When a button is pressed, it emits the "pressed" signal.
 */

struct txt_button_s
{
  txt_widget_t widget;
  char *label;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * Create a new button widget.
 *
 * @param label        The label to use on the new button (UTF-8 format).
 * @return             Pointer to the new button widget.
 */

txt_button_t *txt_new_button(const char *label);

/**
 * Create a new button widget, binding the "pressed" signal to a
 * specified callback function.
 *
 * @param label        The label to use on the new button (UTF-8 format).
 * @param func         The callback function to invoke.
 * @param user_data    User-specified pointer to pass to the callback.
 * @return             Pointer to the new button widget.
 */

txt_button_t *txt_new_button2(const char *label, txt_widget_signal_f func,
                              void *user_data);

/**
 * Change the label used on a button.
 *
 * @param button       The button.
 * @param label        The new label (UTF-8 format).
 */

void txt_set_button_label(txt_button_t *button, const char *label);

#endif /* TXT_BUTTON_H */
