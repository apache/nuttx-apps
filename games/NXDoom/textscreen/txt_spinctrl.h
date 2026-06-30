/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_spinctrl.h
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

#ifndef TXT_SPINCONTROL_H
#define TXT_SPINCONTROL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "txt_widget.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef enum
{
  TXT_SPINCONTROL_INT,
  TXT_SPINCONTROL_FLOAT,
} txt_spincontrol_type_t;

union _float_n_int
{
  float f;
  int i;
};

/* Spin control widget.
 *
 * A spin control widget works as an input box that can be used to
 * set numeric values, but also has buttons that allow its value
 * to be increased or decreased.
 */

struct txt_spincontrol_s
{
  txt_widget_t widget;
  txt_spincontrol_type_t type;
  union _float_n_int min;
  union _float_n_int max;
  union _float_n_int *value;
  union _float_n_int step;
  int editing;
  char *buffer;
  size_t buffer_len;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * Create a new spin control widget tracking an integer value.
 *
 * @param value        Pointer to the variable containing the value
 *                     displayed in the widget.
 * @param min          Minimum value that may be set.
 * @param max          Maximum value that may be set.
 * @return             Pointer to the new spin control widget.
 */

txt_spincontrol_t *txt_newspin_control(int *value, int min, int max);

/**
 * Create a new spin control widget tracking a float value.
 *
 * @param value        Pointer to the variable containing the value
 *                     displayed in the widget.
 * @param min          Minimum value that may be set.
 * @param max          Maximum value that may be set.
 * @return             Pointer to the new spin control widget.
 */

txt_spincontrol_t *txt_new_float_spincontrol(float *value, float min,
                                           float max);

#endif /* TXT_SPINCONTROL_H */
