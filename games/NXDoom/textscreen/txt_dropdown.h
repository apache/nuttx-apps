/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_dropdown.h
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

#ifndef TXT_DROPDOWN_H
#define TXT_DROPDOWN_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "txt_widget.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Dropdown list widget.
 *
 * A dropdown list allows the user to select from a list of values,
 * which appears when the list is selected.
 *
 * When the value of a dropdown list is changed, the "changed" signal
 * is emitted.
 */

struct txt_dropdown_list_s
{
  txt_widget_t widget;
  int *variable;
  const char **values;
  int num_values;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * Create a new dropdown list widget.
 *
 * The parameters specify a list of string labels, and a pointer to an
 * integer variable.  The variable contains the current "value" of the
 * list, as an index within the list of labels.
 *
 * @param variable        Pointer to the variable containing the
 *                        list's value.
 * @param values          Pointer to an array of strings containing
 *                        the labels to use for the list (UTF-8 format).
 * @param num_values      The number of variables in the list.
 */

txt_dropdown_list_t *txt_new_dropdown_list(int *variable,
        const char **values, int num_values);

#endif /* TXT_DROPDOWN_H */
