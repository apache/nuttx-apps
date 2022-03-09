/********************************************************************************************
 * apps/graphics/nxglyphs/src/glyph_calibration24x21.cxx
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

#define BITMAP_NROWS     21
#define BITMAP_NCOLUMNS  24
#define BITMAP_NLUTCODES 6

/********************************************************************************************
 * Private Bitmap Data
 ********************************************************************************************/

using namespace NXWidgets;

/* RGB24 (8-8-8) Colors */

#if CONFIG_NXWIDGETS_BPP == 24 ||  CONFIG_NXWIDGETS_BPP == 32

static const uint32_t g_calibrationNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                       /* Code 0 */
  0xfcfcfc, 0xacacac, 0xd8d8d8, 0xd8881c, 0x9c6014                       /* Codes 1-5 */
};

static const uint32_t g_calibrationSelectedLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                       /* Code 0 */
  0xffffff, 0xc0c0c0, 0xe1e1e1, 0xe1a554, 0xb4874e                       /* Codes 1-5 */
};
/* RGB16 (565) Colors (four of the colors in this map are duplicates) */

#elif CONFIG_NXWIDGETS_BPP == 16

static const uint16_t g_calibrationNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                        /* Code 0 */
  0xffff, 0xad75, 0xdedb, 0xdc43, 0x9b02                                  /* Codes 1-5 */
};

static const uint16_t g_calibrationSelectedLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                        /* Code 0 */
  0xffff, 0xc618, 0xe71c, 0xe52a, 0xb429                                  /* Codes 1-5 */
};

/* 8-bit color lookups.  NOTE:  This is really dumb!  The lookup index is 8-bits and it used
 * to lookup an 8-bit value.  There is no savings in that!  It would be better to just put
 * the 8-bit color/greyscale value in the run-length encoded image and save the cost of these
 * pointless lookups.  But these pointless lookups do make the logic compatible with the
 * 16- and 24-bit types.
 */

#elif CONFIG_NXWIDGETS_BPP == 8
#  ifdef CONFIG_NXWIDGETS_GREYSCALE

/* 8-bit Greyscale */

static const uint8_t g_calibrationNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                         /* Code 0 */
  0xfc, 0xac, 0xd8, 0x93, 0x69                                             /* Codes 1-5 */
};

static const uint8_t g_calibrationSelectedLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                         /* Code 0 */
  0xff, 0xc0, 0xe1, 0xad, 0x8d                                             /* Codes 1-5 */
};

#  else /* CONFIG_NXWIDGETS_GREYSCALE */

/* RGB8 (332) Colors */

static const nxgl_mxpixel_t g_calibrationNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                         /* Code 0 */
  0xff, 0xb6, 0xdb, 0xd0, 0x8c                                             /* Codes 1-5 */
};

static const nxgl_mxpixel_t g_calibrationSelectedLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                         /* Code 0 */
  0xff, 0xdb, 0xff, 0xf5, 0xb1                                             /* Codes 1-5 */
};

#  endif
#else
# error "Unsupported pixel format"
#endif

static const struct SRlePaletteBitmapEntry g_calibrationRleEntries[] =
{
  {11, 0}, { 1, 1}, { 1, 2}, { 6, 0}, { 1, 3}, { 1, 2}, { 3, 0},           /* Row 0 */
  {11, 0}, { 1, 1}, { 1, 2}, { 4, 0}, { 2, 1}, { 1, 3}, { 2, 1}, { 2, 0},  /* Row 1 */
  {11, 0}, { 1, 1}, { 1, 2}, { 2, 0}, { 2, 1}, { 2, 2}, { 1, 3}, { 1, 2},  /* Row 2 */
  { 3, 0},
  {11, 0}, { 1, 1}, { 1, 3}, { 2, 1}, { 2, 2}, { 2, 0}, { 1, 3}, { 4, 0},  /* Row 3 */
  { 9, 0}, { 2, 1}, { 2, 3}, { 2, 2}, { 4, 0}, { 1, 3}, { 4, 0},            /* Row 4 */
  { 3, 0}, { 1, 1}, { 1, 2}, { 2, 0}, { 2, 1}, { 2, 2}, { 1, 1}, { 1, 2},  /* Row 5 */
  { 5, 0}, { 1, 1}, { 2, 3}, { 3, 0},
  { 2, 0}, { 2, 1}, { 1, 3}, { 2, 1}, { 2, 2}, { 2, 0}, { 1, 1}, { 1, 2},  /* Row 6 */
  { 4, 0}, { 1, 1}, { 1, 3}, { 1, 0}, { 2, 3}, { 2, 0},
  { 3, 0}, { 1, 2}, { 1, 3}, { 2, 2}, { 4, 0}, { 1, 1}, { 1, 2}, { 4, 0},  /* Row 7 */
  { 1, 1}, { 3, 0}, { 1, 3}, { 2, 0},
  { 4, 0}, { 1, 3}, { 6, 0}, { 1, 1}, { 1, 2}, { 3, 0}, { 1, 1}, { 1, 3},  /* Row 8 */
  { 3, 0}, { 2, 3}, { 1, 0},
  { 3, 0}, { 1, 1}, { 2, 3}, { 5, 0}, { 1, 1}, { 1, 2}, { 3, 0}, { 1, 1},  /* Row 9 */
  { 5, 0}, { 1, 3}, { 1, 0},
  { 2, 0}, { 1, 1}, { 1, 3}, { 1, 0}, { 2, 3}, { 4, 0}, { 1, 1}, { 1, 2},  /* Row 10 */
  { 3, 0}, { 1, 1}, { 1, 0}, { 2, 4}, { 1, 5}, { 1, 0}, { 1, 3}, { 1, 0},
  { 2, 0}, { 1, 1}, { 3, 0}, { 1, 3}, { 4, 0}, { 1, 1}, { 1, 2}, { 3, 0},  /* Row 11 */
  { 1, 1}, { 1, 0}, { 2, 4}, { 1, 5}, { 1, 0}, { 1, 3}, { 1, 0},
  { 1, 0}, { 1, 1}, { 1, 3}, { 2, 4}, { 1, 5}, { 2, 3}, { 3, 0}, { 1, 1},  /* Row 12 */
  { 1, 2}, { 2, 0}, { 9, 3},
  { 1, 0}, { 1, 1}, { 1, 0}, { 2, 4}, { 1, 5}, { 1, 0}, { 1, 3}, { 3, 0},  /* Row 13 */
  { 1, 1}, { 1, 2}, {11, 0},
  { 1, 0}, { 1, 1}, { 1, 0}, { 2, 4}, { 1, 5}, { 1, 0}, { 1, 3}, { 3, 0},  /* Row 14 */
  { 1, 1}, { 1, 2}, {11, 0},
  { 1, 0}, { 1, 1}, { 1, 0}, { 2, 4}, { 1, 5}, { 1, 0}, { 1, 3}, { 3, 0},  /* Row 15 */
  { 1, 1}, { 1, 2}, {11, 0},
  { 9, 3}, { 2, 0}, { 1, 1}, { 1, 2}, {11, 0},                             /* Row 16 */
  {11, 0}, { 1, 1}, { 1, 2}, {11, 0},                                      /* Row 17 */
  {10, 0}, { 1, 1}, { 2, 3}, { 1, 2}, {10, 0},                             /* Row 18 */
  { 5, 0}, { 4, 1}, { 6, 3}, { 4, 2}, { 5, 0},                             /* Row 19 */
  { 4, 0}, { 4, 1}, { 8, 3}, { 4, 2}, { 4, 0},                             /* Row 20 */
};

/********************************************************************************************
 * Public Bitmap Structure Definitions
 ********************************************************************************************/

const struct SRlePaletteBitmap NXWidgets::g_calibrationBitmap =
{
  CONFIG_NXWIDGETS_BPP,     // bpp    - Bits per pixel
  CONFIG_NXWIDGETS_FMT,     // fmt    - Color format
  BITMAP_NLUTCODES,         // nlut   - Number of colors in the lLook-Up Table (LUT)
  BITMAP_NCOLUMNS,          // width  - Width in pixels
  BITMAP_NROWS,             // height - Height in rows
  {                          // lut    - Pointer to the beginning of the Look-Up Table (LUT)
    g_calibrationNormalLut, //          Index 0: Unselected LUT
    g_calibrationSelectedLut, //          Index 1: Selected LUT
  },
  g_calibrationRleEntries     // data   - Pointer to the beginning of the RLE data
};
