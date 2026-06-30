/****************************************************************************
 * apps/games/NXDoom/src/v_video.h
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
 *  Gamma correction LUT.
 *  Functions to draw patches (by post) directly to screen.
 *  Functions to blit a block to the screen.
 *
 ****************************************************************************/

#ifndef __V_VIDEO__
#define __V_VIDEO__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "doomtype.h"

/* Needed because we are referring to patches. */

#include "v_patch.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CENTERY (SCREENHEIGHT / 2)

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern int dirtybox[4];

extern byte *tinttable;

/* haleyjd 08/28/10: implemented for Strife support
 * haleyjd 08/28/10: Patch clipping callback, implemented to support Choco
 * Strife.
 */

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef boolean (*vpatchclipfunc_t)(patch_t *, int, int);

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void v_set_patch_clip_callback(vpatchclipfunc_t func);

/****************************************************************************
 * Name: v_init
 *
 * Description:
 *  Allocates buffer screens, call before r_init.
 *
 ****************************************************************************/

void v_init(void);

/****************************************************************************
 * Name: v_copy_rect
 *
 * Description:
 *  Draw a block from the specified source screen to the screen.
 *
 ****************************************************************************/

void v_copy_rect(int srcx, int srcy, pixel_t *source, int width, int height,
                 int destx, int desty);

void v_draw_patch(int x, int y, patch_t *patch);
void v_draw_patch_flipped(int x, int y, patch_t *patch);
void v_draw_tl_patch(int x, int y, patch_t *patch);
void v_draw_alt_tl_patch(int x, int y, patch_t *patch);
void v_draw_shadowed_patch(int x, int y, patch_t *patch);
void v_draw_xla_patch(int x, int y, patch_t *patch); /* villsa [STRIFE] */
void v_draw_patch_direct(int x, int y, patch_t *patch);

/****************************************************************************
 * Name: v_draw_block
 *
 * Description:
 *  Draw a linear block of pixels into the view buffer.
 *
 ****************************************************************************/

void v_draw_block(int x, int y, int width, int height, pixel_t *src);

void v_mark_rect(int x, int y, int width, int height);

void v_draw_filled_box(int x, int y, int w, int h, int c);
void v_draw_horiz_line(int x, int y, int w, int c);
void v_draw_vert_line(int x, int y, int h, int c);
void v_draw_box(int x, int y, int w, int h, int c);

/****************************************************************************
 * Name: v_draw_raw_screen
 *
 * Description:
 *  Draw a raw screen lump
 *
 ****************************************************************************/

void v_draw_raw_screen(pixel_t *raw);

/****************************************************************************
 * Name: v_use_buffer
 *
 * Description:
 *  Temporarily switch to using a different buffer to draw graphics, etc.
 *
 ****************************************************************************/

void v_use_buffer(pixel_t *buffer);

/****************************************************************************
 * Name: v_restore_buffer
 *
 * Description:
 *  Return to using the normal screen buffer to draw graphics.
 *
 ****************************************************************************/

void v_restore_buffer(void);

/****************************************************************************
 * Name: v_screenshot
 *
 * Description:
 *  Save a screenshot of the current screen to a file, named in the format
 *  described in the string passed to the function, eg. "DOOM%02i.pcx"
 *
 ****************************************************************************/

void v_screenshot(const char *format);

/****************************************************************************
 * Name: v_load_tint_table
 *
 * Description:
 *  Load the lookup table for translucency calculations from the TINTTAB
 *  lump.
 *
 ****************************************************************************/

void v_load_tint_table(void);

/****************************************************************************
 * Name: v_load_xla_table
 *
 * Description:
 *  villsa [STRIFE]
 *  Load the lookup table for translucency calculations from the XLATAB lump.
 *
 ****************************************************************************/

void v_load_xla_table(void);

void v_draw_mouse_speed_box(int speed);

#endif
