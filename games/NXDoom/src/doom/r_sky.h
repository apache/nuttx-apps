/****************************************************************************
 * apps/games/NXDoom/src/doom/r_sky.h
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
 *  Sky rendering.
 *
 ****************************************************************************/

#ifndef __R_SKY__
#define __R_SKY__

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* SKY, store the number for name. */

#define SKYFLATNAME "F_SKY1"

/* The sky map is 256*128*4 maps. */

#define ANGLETOSKYSHIFT 22

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern int skytexture;
extern int skytexturemid;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Called whenever the view size changes. */

void r_init_sky_map(void);

#endif
