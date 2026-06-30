/****************************************************************************
 * apps/games/NXDoom/src/doom/r_data.h
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
 *  Refresh module, data I/O, caching, retrieval of graphics
 *  by name.
 *
 ****************************************************************************/

#ifndef __R_DATA__
#define __R_DATA__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "r_defs.h"
#include "r_state.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern int numflats;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Retrieve column data for span blitting. */

byte *r_get_column(int tex, int col);

/* I/O, setting up the stuff. */

void r_init_data(void);
void r_precache_level(void);

/* Retrieval.
 * Floor/ceiling opaque texture tiles,
 * lookup by name. For animation?
 */

int r_flat_num_for_name(const char *name);

/* Called by p_ticker for switches and animations,
 * returns the texture number for the texture name.
 */

int r_texture_num_for_name(const char *name);
int r_check_texture_num_for_name(const char *name);

#endif /* __R_DATA__ */
