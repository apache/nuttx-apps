/****************************************************************************
 * apps/games/NXDoom/src/doom/r_draw.h
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

#ifndef __R_DRAW__
#define __R_DRAW__

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern lighttable_t *dc_colormap;
extern int dc_x;
extern int dc_yl;
extern int dc_yh;
extern fixed_t dc_iscale;
extern fixed_t dc_texturemid;

/* first pixel in a column */

extern byte *dc_source;

extern int ds_y;
extern int ds_x1;
extern int ds_x2;

extern lighttable_t *ds_colormap;

extern fixed_t ds_xfrac;
extern fixed_t ds_yfrac;
extern fixed_t ds_xstep;
extern fixed_t ds_ystep;

/* start of a 64*64 tile image */

extern byte *ds_source;

extern byte *translationtables;
extern byte *dc_translation;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Span blitting for rows, floor/ceiling. No Spectre effect needed. */

void r_draw_span(void);

/* Low resolution mode, 160x200? */

void r_draw_span_low(void);

void r_init_buffer(int width, int height);

/* Initialize color translation tables, for player rendering etc. */

void r_init_translation_table(void);

/* Rendering function. */

void r_fill_back_screen(void);

/* If the view size is not full screen, draws a border around it. */

void r_draw_view_border(void);

/* The span blitting interface.
 * Hook in assembler or system specific BLT here.
 */

void r_draw_column(void);
void r_draw_column_low(void);

/* The Spectre/Invisibility effect. */

void r_draw_fuzz_column(void);
void r_draw_fuzz_column_low(void);

/* Draw with color translation tables, for player sprite rendering,
 * Green/Red/Blue/Indigo shirts.
 */

void r_draw_translated_column(void);
void r_draw_translated_column_low(void);

void r_video_erase(unsigned ofs, int count);

#endif /* __R_DRAW__ */
