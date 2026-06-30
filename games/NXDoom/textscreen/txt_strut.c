/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_strut.c
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

#include "txt_io.h"
#include "txt_main.h"
#include "txt_strut.h"
#include "txt_window.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

static void txt_strut_size_calc(TXT_UNCAST_ARG(strut));
static void txt_strut_drawer(TXT_UNCAST_ARG(strut));
static void txt_strut_destructor(TXT_UNCAST_ARG(strut));
static int txt_strut_keypress(TXT_UNCAST_ARG(strut), int key);

/****************************************************************************
 * Public Data
 ****************************************************************************/

txt_widget_class_t txt_strut_class =
{
  txt_never_selectable,
  txt_strut_size_calc,
  txt_strut_drawer,
  txt_strut_keypress,
  txt_strut_destructor,
  NULL,
  NULL,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void txt_strut_size_calc(TXT_UNCAST_ARG(strut))
{
  TXT_CAST_ARG(txt_strut_t, strut);

  /* Minimum width is the string length + two spaces for padding */

  strut->widget.w = strut->width;
  strut->widget.h = strut->height;
}

static void txt_strut_drawer(TXT_UNCAST_ARG(strut))
{
  /* Nothing is drawn for a strut. */
}

static void txt_strut_destructor(TXT_UNCAST_ARG(strut))
{
}

static int txt_strut_keypress(TXT_UNCAST_ARG(strut), int key)
{
  return 0;
}

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

txt_strut_t *txt_new_strut(int width, int height)
{
  txt_strut_t *strut;

  strut = malloc(sizeof(txt_strut_t));

  txt_init_widget(strut, &txt_strut_class);
  strut->width = width;
  strut->height = height;

  return strut;
}
