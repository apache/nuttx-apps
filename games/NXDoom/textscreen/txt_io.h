/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_io.h
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
 * Text mode emulation in SDL
 *
 ****************************************************************************/

#ifndef TXT_IO_H
#define TXT_IO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "txt_main.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef struct
{
  int bgcolor;
  int fgcolor;
} txt_saved_colors_t;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void txt_put_symbol(int c);
void txt_putchar(int c);
void txt_puts(const char *s);
void txt_goto_xy(int x, int y);
void txt_get_xy(int *x, int *y);
void txt_fgcolour(txt_color_t color);
void txt_bgcolour(int color, int blinking);
void txt_save_colours(txt_saved_colors_t *save);
void txt_restore_colours(txt_saved_colors_t *save);

#endif /* TXT_IO_H */
