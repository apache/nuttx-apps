/****************************************************************************
 * apps/games/NXDoom/src/doom/f_wipe.h
 *
 * SPDX-License-Identifer: GPLv2
 *
 * Copyright(C) 1993-1996 Id Software, Inc.
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
 * DESCRIPTION:
 *  Mission start screen wipe/melt, special effects.
 *
 ****************************************************************************/

#ifndef __F_WIPE_H__
#define __F_WIPE_H__

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* SCREEN WIPE PACKAGE */

enum
{
  /* simple gradual pixel change for 8-bit only */

  WIPE_COLORXFORM,
  WIPE_MELT, /* weird screen melt */
  WIPE_NUMWIPES
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifndef CONFIG_GAMES_NXDOOM_NOWIPE
int wipe_start_screen(int x, int y, int width, int height);

int wipe_endscreen(int x, int y, int width, int height);

int wipe_screen_wipe(int wipeno, int x, int y, int width, int height,
                     int ticks);
#else

/* Disable wipe animations with no-ops. */

#define wipe_start_screen(x, y, width, height) ((void)(0))
#define wipe_endscreen(x, y, width, height) ((void)(0))
#define wipe_screen_wipe(wipeno, x, y, width, height, ticks) ((int)(1))

#endif /* CONFIG_GAMES_NXDOOM_NOWIPE */
#endif /* __F_WIPE_H__ */
