/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_strut.h
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

#ifndef TXT_STRUT_H
#define TXT_STRUT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "txt_widget.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Strut widget.
 *
 * A strut is a widget that takes up a fixed amount of space.  It can
 * be visualised as a transparent box.  Struts are used to provide
 * spacing between widgets.
 */

struct txt_strut_s
{
  txt_widget_t widget;
  int width;
  int height;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * Create a new strut.
 *
 * @param width      Width of the strut, in characters.
 * @param height     Height of the strut, in characters.
 */

txt_strut_t *txt_new_strut(int width, int height);

#endif /* TXT_STRUT_H */
