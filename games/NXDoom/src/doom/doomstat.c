/****************************************************************************
 * apps/games/NXDoom/src/doom/doomstat.c
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
 *  Put all global state variables here.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>

#include "doomstat.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Game Mode - identify IWAD as shareware, retail etc. */

game_mode_t gamemode = indetermined;
gamemission_t gamemission = doom;
game_version_t gameversion = exe_final2;
game_variant_t gamevariant = vanilla;

/* Set if homebrew PWAD stuff has been added. */

boolean modifiedgame;

/****************************************************************************
 * Public Functions
 ****************************************************************************/
