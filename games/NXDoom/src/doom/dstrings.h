/****************************************************************************
 * apps/games/NXDoom/src/doom/dstrings.h
 *
 * SPDX-License-Identifier: GPLv2
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
 * DOOM strings, by language.
 *
 ****************************************************************************/

#ifndef __DSTRINGS__
#define __DSTRINGS__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/* All important printed strings. */

#if defined(CONFIG_GAMES_NXDOOM_LANG_EN)
#include "d_englsh.h"
#elif defined(CONFIG_GAMES_NXDOOM_LANG_FR)
#include "d_french.h"
#else
#error "Unsupported language choice."
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Misc. other strings. */

#define SAVEGAMENAME "doomsav"

/* QuitDOOM messages; 8 per each game type */

#define NUM_QUITMESSAGES (8)

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern const char *doom1_endmsg[];
extern const char *doom2_endmsg[];

#endif /* __DSTRINGS__ */
