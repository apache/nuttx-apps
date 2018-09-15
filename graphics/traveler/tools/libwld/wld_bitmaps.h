/****************************************************************************
 * apps/graphics/traveler/tools/libwld/wld_bitmaps.h
 * This file contains definitions for the texture bitmaps
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

#ifndef __APPS_GRAPHICS_TRAVELER_TOOSL_LIBWLD_WLD_BITMAPS_H
#define __APPS_GRAPHICS_TRAVELER_TOOSL_LIBWLD_WLD_BITMAPS_H

#ifdef __cplusplus
extern "C"
{
#endif  /* __cplusplus */

/*************************************************************************
 * Included Files
 *************************************************************************/

#include <stdio.h>
#include "wld_color.h"

/*************************************************************************
 * Pre-processor Definitions
 *************************************************************************/

/* Return codes */

enum
{
  BMAP_SUCCESS = 0,
  BMAP_TOO_MANY = 20,
  BMAP_BML_OPEN_ERROR,
  BMAP_BML_READ_ERROR
};

#define BITMAP_WIDTH  64
#define BITMAP_HEIGHT 64
#define BITMAP_LOG2H   6
#define BITMAP_SIZE   (BITMAP_WIDTH * BITMAP_HEIGHT)
#define BITMAP_IMASK  (BITMAP_HEIGHT-1)
#define BITMAP_JMASK  (BITMAP_WIDTH-1)
#define BITMAP_JSHIFT  6
#define BMICLIP(i)    ((i) & BITMAP_IMASK)
#define BMJCLIP(i)    ((i) & BITMAP_JMASK)
#define BMOFFSET(i,j) (((i) << BITMAP_JSHIFT) | (j))
#define MAX_BITMAPS   256

/*************************************************************************
 * Public Type Definitions
 *************************************************************************/

struct wld_bitmap_s
{
  uint16_t w;
  uint16_t h;
  uint8_t log2h;
  wld_pixel_t *bm;
};

typedef struct wld_bitmap_s wld_bitmap_t;

/*************************************************************************
 * Global Variables
 *************************************************************************/

/* These point to the (allocated) bit map buffers for the even and odd
 * bitmaps
 */

extern wld_bitmap_t *g_even_bitmaps[MAX_BITMAPS];
#ifndef WEDIT
extern wld_bitmap_t *g_odd_bitmaps[MAX_BITMAPS];
#endif

/* This is the palette to use for the selected world */

#if MSWINDOWS
extern color_rgb_t worldPalette[256];
#endif

/* This is the maximum value + 1 of a texture code */

extern uint16_t g_nbitmaps;

/* These are the colors from the worldPalette which should used to rend
 * the sky and ground
 */

extern uint8_t g_sky_color;
extern uint8_t g_ground_color;

/*************************************************************************
 * Global Function Prototypes
 *************************************************************************/

uint8_t wld_initialize_bitmaps(void);
void    wld_discard_bitmaps(void);
uint8_t wld_load_bitmapfile(char *bmlfile);

void        wld_free_texture(wld_bitmap_t *t);
wld_bitmap_t *wld_read_texturefile(char *filename);

#ifdef  __cplusplus
}
#endif

#endif /* __APPS_GRAPHICS_TRAVELER_TOOSL_LIBWLD_WLD_BITMAPS_H */

