/****************************************************************************
 * apps/games/NXDoom/src/doom/r_main.h
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
 *  System specific interface stuff.
 *
 ****************************************************************************/

#ifndef __R_MAIN__
#define __R_MAIN__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "d_player.h"
#include "r_data.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Lighting LUT.
 * Used for z-depth cuing per column/row, and other lighting effects (sector
 * ambient, flash).
 */

/* Lighting constants. Now why not 32 levels here? */

#define LIGHTLEVELS 16
#define LIGHTSEGSHIFT 4

#define MAXLIGHTSCALE 48
#define LIGHTSCALESHIFT 12
#define MAXLIGHTZ 128
#define LIGHTZSHIFT 20

/* Number of diminishing brightness levels.
 * There a 0-31, i.e. 32 LUT in the COLORMAP lump.
 */

#define NUMCOLORMAPS 32

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Function pointers to switch refresh/drawing functions.
 * Used to select shadow mode etc.
 */

extern void (*colfunc)(void);
extern void (*transcolfunc)(void);
extern void (*basecolfunc)(void);
extern void (*fuzzcolfunc)(void);

/* No shadow effects on floors. */

extern void (*spanfunc)(void);

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* POV related. */

extern fixed_t viewcos;
extern fixed_t viewsin;

extern int viewwindowx;
extern int viewwindowy;

extern int centerx;
extern int centery;

extern fixed_t centerxfrac;
extern fixed_t centeryfrac;
extern fixed_t projection;

extern int validcount;

extern int linecount;
extern int loopcount;

extern boolean setsizeneeded;

extern lighttable_t *scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
extern lighttable_t *scalelightfixed[MAXLIGHTSCALE];
extern lighttable_t *zlight[LIGHTLEVELS][MAXLIGHTZ];

extern int extralight;
extern lighttable_t *fixedcolormap;

/* Blocky/low detail mode. B remove this? 0 = high, 1 = low
 */

extern int detailshift;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Utility functions. */

int r_point_on_side(fixed_t x, fixed_t y, node_t *node);

int r_point_on_seg_side(fixed_t x, fixed_t y, seg_t *line);

angle_t r_point_to_angle(fixed_t x, fixed_t y);

angle_t r_point_to_angle2(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2);

fixed_t r_point_to_dist(fixed_t x, fixed_t y);

fixed_t r_scale_from_global_angle(angle_t visangle);

subsector_t *r_point_in_subsector(fixed_t x, fixed_t y);

/* REFRESH - the actual rendering functions. */

/* Called by G_Drawer. */

void r_render_player_view(player_t *player);

/* Called by startup code. */

void r_init(void);

/* Called by m_responder. */

void r_set_view_size(int blocks, int detail);

void r_execute_set_view_size(void);

#endif /* __R_MAIN__ */
