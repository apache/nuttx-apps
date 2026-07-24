/****************************************************************************
 * apps/games/NXDoom/src/doom/r_plane.h
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
 *  Refresh, visplane stuff (floor, ceilings).
 *
 ****************************************************************************/

#ifndef __R_PLANE__
#define __R_PLANE__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "r_data.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef void (*planefunction_t)(int top, int bottom);

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Visplane related. */

extern short *lastopening;

extern planefunction_t floorfunc;
extern planefunction_t ceilingfunc_t;

extern short floorclip[SCREENWIDTH];
extern short ceilingclip[SCREENWIDTH];

extern fixed_t yslope[SCREENHEIGHT];
extern fixed_t distscale[SCREENWIDTH];

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void r_init_planes(void);
void r_shutdown_planes(void);
void r_clear_planes(void);

void r_draw_planes(void);

visplane_t *r_find_plane(fixed_t height, int picnum, int lightlevel);

visplane_t *r_check_plane(visplane_t *pl, int start, int stop);

#endif /* __R_PLANE__ */
