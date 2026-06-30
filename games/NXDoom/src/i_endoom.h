/****************************************************************************
 * apps/games/NXDoom/src/i_endoom.h
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
 *   Exit text-mode ENDOOM screen.
 *
 ****************************************************************************/

#ifndef __I_ENDOOM__
#define __I_ENDOOM__

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_GAMES_NXDOOM_ENDOOM

/* Display the Endoom screen on shutdown.  Pass a pointer to the
 * ENDOOM lump.
 */

void i_endoom(byte *data);
#endif

#endif /* __I_ENDOOM__ */
