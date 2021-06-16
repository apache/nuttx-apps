/********************************************************************************************
 * apps/graphics/nxglyphs/src/glyph_mediaplayer24x24.cxx
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
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
