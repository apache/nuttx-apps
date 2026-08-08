/****************************************************************************
 * apps/games/NXDoom/src/doom/r_bsp.h
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
 *  Refresh module, BSP traversal and handling.
 *
 ****************************************************************************/

#ifndef __R_BSP__
#define __R_BSP__

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef void (*drawfunc_t)(int start, int stop);

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern seg_t *curline;
extern side_t *sidedef;
extern line_t *linedef;
extern sector_t *frontsector;
extern sector_t *backsector;

extern int rw_x;
extern int rw_stopx;

extern boolean segtextured;

/* false if the back side is the same plane */

extern boolean markfloor;
extern boolean markceiling;

extern boolean skymap;

#ifdef CONFIG_GAMES_NXDOOM_HEAP_BUFFERS
extern drawseg_t *drawsegs;
#else
extern drawseg_t drawsegs[CONFIG_GAMES_NXDOOM_MAXDRAWSEGS];
#endif
extern drawseg_t *ds_p;

extern lighttable_t **hscalelight;
extern lighttable_t **vscalelight;
extern lighttable_t **dscalelight;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* BSP? */

void r_clear_clip_segs(void);
void r_clear_draw_segs(void);

void r_render_bsp_node(int bspnum);

#endif /* __R_BSP__ */
