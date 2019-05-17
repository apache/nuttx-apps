/********************************************************************************************
 * apps/graphics/nxglyphs/src/glyph_mediaplayer24x24.cxx
 *
 *   Copyright (C) 2013 Ken Pettit. All rights reserved.
 *   Author: Ken Pettit <pettitkd@gmail.com>
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
 * 3. Neither the name NuttX, NxWidgets, nor the names of its contributors
 *    me be used to endorse or promote products derived from this software
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
 ********************************************************************************************/

/********************************************************************************************
 * Included Files
 ********************************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>

#include <nuttx/nx/nxglib.h>
#include <nuttx/video/fb.h>
#include <nuttx/video/rgbcolors.h>

#include "graphics/nxwidgets/crlepalettebitmap.hxx"

#include "graphics/nxglyphs.hxx"

/********************************************************************************************
 * Pre-Processor Definitions
 ********************************************************************************************/

#define BITMAP_WIDTH       24
#define BITMAP_HEIGHT      24
#define BITMAP_PALETTESIZE 9

/********************************************************************************************
 * Private Bitmap Data
 ********************************************************************************************/

using namespace NXWidgets;

/* RGB24 (8-8-8) Colors */

static const nxwidget_pixel_t palette[BITMAP_PALETTESIZE] =
{
  MKRGB(248,252,251), MKRGB(120,198,241), MKRGB( 97,177,228), MKRGB( 31,104,177),
  MKRGB( 73,153,213), MKRGB( 46,122,193), MKRGB(140,196,230), MKRGB(  9, 77,154),
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR
};

static const nxwidget_pixel_t hilight_palette[BITMAP_PALETTESIZE] =
{
  MKRGB(255,255,255), MKRGB(170,248,255), MKRGB(147,227,255), MKRGB( 81,154,227),
  MKRGB(123,203,255), MKRGB( 96,172,243), MKRGB(190,246,255), MKRGB( 59,127,204),
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR
};

static const SRlePaletteBitmapEntry bitmap[] =
{
  { 8, 8}, { 8, 7}, { 8, 8},                              /* Row 0 */
  { 6, 8}, { 1, 7}, { 1, 3}, { 1, 5}, { 2, 2}, { 2, 1},
  { 2, 2}, { 1, 5}, { 1, 3}, { 1, 7}, { 6, 8},            /* Row 1 */
  { 4, 8}, { 2, 7}, { 1, 4}, { 1, 2}, { 1, 6}, { 6, 1},
  { 1, 6}, { 1, 2}, { 1, 4}, { 2, 7}, { 4, 8},            /* Row 2 */
  { 3, 8}, { 1, 7}, { 1, 3}, { 1, 2}, { 1, 6}, { 2, 1},
  { 4, 6}, { 2, 1}, { 2, 6}, { 2, 2}, { 1, 3}, { 1, 7},
  { 3, 8},                                                /* Row 3 */
  { 2, 8}, { 1, 7}, { 1, 3}, { 1, 1}, { 2, 6}, { 3, 1},
  { 2, 2}, { 1, 4}, { 1, 3}, { 5, 5}, { 1, 1}, { 1, 3},
  { 1, 7}, { 2, 8},                                       /* Row 4 */
  { 2, 8}, { 1, 7}, { 1, 2}, { 1, 6}, { 4, 1}, { 1, 4},
  { 3, 5}, { 2, 6}, { 3, 0}, { 1, 3}, { 2, 2}, { 1, 7},
  { 2, 8},                                                /* Row 5 */
  { 1, 8}, { 1, 7}, { 1, 4}, { 1, 6}, { 4, 1}, { 1, 2},
  { 1, 5}, { 1, 6}, { 7, 0}, { 1, 3}, { 1, 2}, { 1, 6},
  { 1, 4}, { 1, 7}, { 1, 8},                              /* Row 6 */
  { 1, 8}, { 1, 3}, { 1, 6}, { 4, 1}, { 1, 2}, { 1, 5},
  { 4, 0}, { 2, 6}, { 1, 3}, { 1, 5}, { 1, 0}, { 1, 3},
  { 1, 2}, { 1, 1}, { 1, 6}, { 1, 3}, { 1, 8},            /* Row 7 */
  { 1, 3}, { 1, 4}, { 1, 6}, { 4, 1}, { 1, 2}, { 1, 5},
  { 2, 0}, { 1, 6}, { 3, 3}, { 1, 4}, { 1, 5}, { 1, 0},
  { 1, 3}, { 1, 2}, { 1, 1}, { 1, 6}, { 1, 4}, { 1, 3},   /* Row 8 */
  { 1, 3}, { 6, 1}, { 1, 2}, { 1, 5}, { 1, 0}, { 1, 4},
  { 1, 3}, { 1, 4}, { 3, 2}, { 1, 5}, { 1, 0}, { 1, 3},
  { 1, 2}, { 3, 1}, { 1, 3},                              /* Row 9 */
  { 1, 3}, { 6, 1}, { 1, 2}, { 1, 5}, { 1, 0}, { 1, 3},
  { 1, 4}, { 4, 2}, { 1, 7}, { 1, 0}, { 1, 3}, { 1, 2},
  { 3, 1}, { 1, 3},                                       /* Row 10 */
  { 1, 3}, { 6, 1}, { 1, 2}, { 1, 5}, { 1, 0}, { 1, 3},
  { 1, 4}, { 4, 2}, { 1, 7}, { 1, 0}, { 1, 7}, { 1, 2},
  { 3, 1}, { 1, 3},                                       /* Row 11 */
  { 1, 3}, { 6, 1}, { 1, 2}, { 1, 5}, { 1, 0}, { 1, 3},
  { 1, 4}, { 4, 2}, { 1, 7}, { 1, 0}, { 1, 7}, { 1, 2},
  { 3, 1}, { 1, 5},                                       /* Row 12 */
  { 1, 3}, { 1, 6}, { 4, 1}, { 2, 2}, { 1, 5}, { 1, 0},
  { 1, 3}, { 1, 4}, { 2, 2}, { 1, 5}, { 1, 7}, { 1, 6},
  { 1, 0}, { 1, 7}, { 1, 2}, { 2, 1}, { 1, 6}, { 1, 3},   /* Row 13 */
  { 1, 3}, { 5, 1}, { 1, 2}, { 2, 3}, { 1, 0}, { 1, 3},
  { 1, 4}, { 1, 2}, { 1, 7}, { 1, 6}, { 3, 0}, { 1, 7},
  { 1, 2}, { 2, 1}, { 1, 2}, { 1, 3},                     /* Row 14 */
  { 1, 5}, { 1, 4}, { 3, 1}, { 1, 2}, { 1, 3}, { 1, 2},
  { 1, 6}, { 1, 0}, { 1, 3}, { 1, 2}, { 1, 5}, { 1, 6},
  { 4, 0}, { 1, 7}, { 1, 2}, { 2, 1}, { 1, 4}, { 1, 5},   /* Row 15 */
  { 1, 8}, { 1, 4}, { 2, 1}, { 1, 2}, { 1, 3}, { 4, 0},
  { 1, 3}, { 1, 2}, { 1, 3}, { 1, 6}, { 3, 0}, { 1, 6},
  { 1, 7}, { 1, 2}, { 2, 1}, { 1, 4}, { 1, 8},            /* Row 16 */
  { 1, 8}, { 1, 5}, { 1, 2}, { 1, 1}, { 1, 5}, { 1, 6},
  { 4, 0}, { 1, 3}, { 2, 2}, { 1, 3}, { 1, 6}, { 1, 0},
  { 1, 6}, { 1, 7}, { 1, 4}, { 2, 1}, { 1, 2}, { 1, 5},
  { 1, 8},                                                /* Row 17 */
  { 2, 8}, { 1, 5}, { 1, 1}, { 1, 5}, { 1, 1}, { 4, 0},
  { 1, 3}, { 1, 2}, { 1, 1}, { 1, 4}, { 1, 3}, { 1, 7},
  { 1, 3}, { 1, 2}, { 3, 1}, { 1, 5}, { 2, 8},            /* Row 18 */
  { 2, 8}, { 1, 5}, { 1, 4}, { 1, 1}, { 1, 3}, { 1, 6},
  { 1, 0}, { 1, 6}, { 1, 3}, { 1, 2}, { 9, 1}, { 1, 4},
  { 1, 5}, { 2, 8},                                       /* Row 19 */
  { 3, 8}, { 1, 5}, { 1, 4}, { 1, 1}, { 1, 3}, { 1, 5},
  { 1, 3}, {10, 1}, { 1, 4}, { 1, 5}, { 3, 8},            /* Row 20 */
  { 4, 8}, { 2, 4}, {12, 1}, { 2, 4}, { 4, 8},            /* Row 21 */
  { 6, 8}, { 2, 4}, { 8, 1}, { 2, 4}, { 6, 8},            /* Row 22 */
  { 8, 8}, { 8, 4}, { 8, 8},                              /* Row 23 */
};

/********************************************************************************************
 * Public Bitmap Structure Definitions
 ********************************************************************************************/

const struct SRlePaletteBitmap NXWidgets::g_mediaplayerBitmap =
{
  CONFIG_NXWIDGETS_BPP,
  CONFIG_NXWIDGETS_FMT,
  BITMAP_PALETTESIZE,
  BITMAP_WIDTH,
  BITMAP_HEIGHT,
  {palette, hilight_palette},
  bitmap
};
