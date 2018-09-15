/****************************************************************************
 * apps/graphics/traveler/tools/tcledit/tcl_x11graphics.h
 *
 *   Copyright (C) 2016 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __APPS_GRAPHICS_TRAVELER_TOOLS_TCLEDIT_TCL_X11GRAPHICS_H
#define __APPS_GRAPHICS_TRAVELER_TOOLS_TCLEDIT_TCL_X11GRAPHICS_H

/****************************************************************************
 * Included Files
 ***************************************************************************/

#include <X11/Xlib.h>
#include "wld_color.h"
#include "wld_bitmaps.h"
#include "wld_plane.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NUM_PLANES      3
#define WINDOW_SIZE     400

/* WORLD_INIFINITY is the largest distance that can be represent with
 * int16_t .  This results in the following WORLD_SIZE.
 */

#define WORLD_SIZE      0x8000
#define WORLD_INFINITY  0x7fff

/* Each display can contain the following color types.  These values are
 * table indices.
 */

#define BKGD_COLOR      0 /* Background color */
#define HLINE_COLOR     1 /* Horizontal line color */
#define VLINE_COLOR     2 /* Vertical line color */
#define GRID_COLOR      3 /* Color of grid lines */
#define BRECT_COLOR     4 /* Rectangles beyond current plane */
#define NRECT_COLOR     5 /* Non-selected rectangles in current plane */
#define SRECT_COLOR     6 /* Selected Rectangles in current plane */
#define FRECT_COLOR     7 /* Rectangles before current plane */
#define NUM_COLORS      8 /* Total number of colors used in the window */

/* There are three color states:  Positioning, editing this (selected)
 * plane, or editing another plane (non-selected).
 */

#define POS_COLORS      0 /* Position mode colors */
#define SEL_COLORS      1 /* Edit mode colors, selected plane */
#define NSEL_COLORS     2 /* Edit mode colors, non-selected plane */
#define NUM_COLOR_SETS  3 /* Number of color sets used in each window */

#define POS_BKGD_COLOR  (BKGD_COLOR   + (POS_COLORS * NUM_COLORS))
#define POS_HLINE_COLOR (HLINE_COLOR  + (POS_COLORS * NUM_COLORS))
#define POS_VLINE_COLOR (VLINE_COLOR  + (POS_COLORS * NUM_COLORS))
#define POS_BRECT_COLOR (BRECT_COLOR  + (POS_COLORS * NUM_COLORS))
#define POS_NRECT_COLOR (NRECT_COLOR  + (POS_COLORS * NUM_COLORS))
#define POS_SRECT_COLOR (SRECT_COLOR  + (POS_COLORS * NUM_COLORS))
#define POS_FRECT_COLOR (FRECT_COLOR  + (POS_COLORS * NUM_COLORS))

#define SEL_BKGD_COLOR  (BKGD_COLOR   + (SEL_COLORS * NUM_COLORS))
#define SEL_HLINE_COLOR (HLINE_COLOR  + (SEL_COLORS * NUM_COLORS))
#define SEL_VLINE_COLOR (VLINE_COLOR  + (SEL_COLORS * NUM_COLORS))
#define SEL_BRECT_COLOR (BRECT_COLOR  + (SEL_COLORS * NUM_COLORS))
#define SEL_NRECT_COLOR (NRECT_COLOR  + (SEL_COLORS * NUM_COLORS))
#define SEL_SRECT_COLOR (SRECT_COLOR  + (SEL_COLORS * NUM_COLORS))
#define SEL_FRECT_COLOR (FRECT_COLOR  + (SEL_COLORS * NUM_COLORS))

#define NSEL_BKGD_COLOR  (BKGD_COLOR  + (NSEL_COLORS * NUM_COLORS))
#define NSEL_HLINE_COLOR (HLINE_COLOR + (NSEL_COLORS * NUM_COLORS))
#define NSEL_VLINE_COLOR (VLINE_COLOR + (NSEL_COLORS * NUM_COLORS))
#define NSEL_BRECT_COLOR (BRECT_COLOR + (NSEL_COLORS * NUM_COLORS))
#define NSEL_NRECT_COLOR (NRECT_COLOR + (NSEL_COLORS * NUM_COLORS))
#define NSEL_SRECT_COLOR (SRECT_COLOR + (NSEL_COLORS * NUM_COLORS))
#define NSEL_FRECT_COLOR (FRECT_COLOR + (NSEL_COLORS * NUM_COLORS))

/* This is the size of the palette for each window */

#define PALETTE_SIZE (NUM_COLORS * NUM_COLOR_SETS)

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

/* Assume 32-bit TrueColor */

typedef uint32_t dev_pixel_t;

enum edit_mode_e
{
  EDITMODE_NONE = 0, /* Initial mode has not been selected */
  EDITMODE_POS,      /* Positioning in world */
  EDITMODE_NEW       /* Creating a new rectangle */
};

enum edit_plane_e
{
  EDITPLANE_X = 0,   /* Editting rectangle in the x plane */
  EDITPLANE_Y,       /* Editting rectangle in the y plane */
  EDITPLANE_Z        /* Editting rectangle in the z plane */
};

typedef struct
{
  char *title;                             /* Title on window */
  enum edit_plane_e plane;                 /* Identifies plane of window */
  int width;                               /* Width of window */
  int height;                              /* Height of window */
  int ncolors;                             /* Number of colors allocated (PALETTE_SIZE) */
  Display *display;                        /* X stuff */
  Window win;
  XImage *image;
  int screen;
  dev_pixel_t *frameBuffer;                /* Pointer to framebuffer */
  color_rgb_t palette[PALETTE_SIZE];       /* Colors requested */
  unsigned long colorLookup[PALETTE_SIZE]; /* Color values to use */
} tcl_window_t;

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern enum edit_mode_e  g_edit_mode;
extern enum edit_plane_e g_edit_plane;

extern int g_view_size;
extern int g_grid_step;
extern int g_coord_offset[NUM_PLANES];
extern int g_plane_position[NUM_PLANES];
extern tcl_window_t g_windows[NUM_PLANES];
extern rect_data_t g_edit_rect;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void x11_initialize_graphics(tcl_window_t *w);
void x11_end_graphics(tcl_window_t *w);
void x11_update_screen(tcl_window_t *w);

void tcl_paint_background(tcl_window_t *w);
void tcl_paint_position(tcl_window_t *w);
void tcl_paint_grid(tcl_window_t *w);
void tcl_paint_rectangles(tcl_window_t *w);
void tcl_update_screen(tcl_window_t *w);

#endif /* __APPS_GRAPHICS_TRAVELER_TOOLS_TCLEDIT_TCL_X11GRAPHICS_H */
