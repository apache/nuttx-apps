/********************************************************************************************
 * apps/graphics/nxglyphs/src/glyph_calibration48x42.cxx
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

#define BITMAP_NROWS     42
#define BITMAP_NCOLUMNS  48
#define BITMAP_NLUTCODES 7

#define DARK_CALIB_ICON  1

/********************************************************************************************
 * Private Bitmap Data
 ********************************************************************************************/

using namespace NXWidgets;

/* RGB24 (8-8-8) Colors */

#if CONFIG_NXWIDGETS_BPP == 24 ||  CONFIG_NXWIDGETS_BPP == 32
#  ifdef DARK_CALIB_ICON

static const uint32_t g_calibrationNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                       /* Code 0 */
  0xbdbdbd, 0xababab, 0x818181, 0x3f3f3f, 0xab6c15, 0x813f15             /* Codes 1-6 */
};

static const uint32_t g_calibrationBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                       /* Code 0 */
  0xfcfcfc, 0xe4e4e4, 0xacacac, 0x545454, 0xe4901c, 0xac541c             /* Codes 1-6 */
};

#  else /* DARK_CALIB_ICON */

static const uint32_t g_calibrationNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                       /* Code 0 */
  0xfcfcfc, 0xe4e4e4, 0xacacac, 0x545454, 0xe4901c, 0xac541c             /* Codes 1-6 */
};

static const uint32_t g_calibrationBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                       /* Code 0 */
  0xffffff, 0xffffff, 0xd7d7d7, 0x696969, 0xffb423, 0xd76923             /* Codes 1-6 */
};
#  endif /* DARK_CALIB_ICON */

/* RGB16 (565) Colors */

#elif CONFIG_NXWIDGETS_BPP == 16
#  ifdef DARK_CALIB_ICON

static const uint16_t g_calibrationNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                       /* Code 0 */
  0xbdf7, 0xad55, 0x8410, 0x39e7, 0xab62, 0x81e2                         /* Codes 1-6 */
};

static const uint16_t g_calibrationBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                       /* Code 0 */
  0xffff, 0xe73c, 0xad75, 0x52aa, 0xe483, 0xaaa3                         /* Codes 1-6 */
};

#  else /* DARK_CALIB_ICON */

static const uint16_t g_calibrationNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                       /* Code 0 */
  0xffff, 0xe73c, 0xad75, 0x52aa, 0xe483, 0xaaa3                         /* Codes 1-6 */
};

static const uint16_t g_calibrationBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                       /* Code 0 */
  0xffff, 0xffff, 0xd6ba, 0x6b4d, 0xfda4, 0xd344                         /* Codes 1-6 */
};

#  endif /* DARK_CALIB_ICON */

/* 8-bit color lookups.  NOTE:  This is really dumb!  The lookup index is 8-bits and it used
 * to lookup an 8-bit value.  There is no savings in that!  It would be better to just put
 * the 8-bit color/greyscale value in the run-length encoded image and save the cost of these
 * pointless lookups.  But these pointless lookups do make the logic compatible with the
 * 16- and 24-bit types.
 */

#elif CONFIG_NXWIDGETS_BPP == 8
#  ifdef CONFIG_NXWIDGETS_GREYSCALE

/* 8-bit Greyscale */

#    ifdef DARK_CALIB_ICON

static const uint8_t g_calibrationNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                       /* Code 0 */
  0xbd, 0xab, 0x81, 0x3f, 0x74, 0x4d                                     /* Codes 1-6 */
};

static const uint8_t g_calibrationBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                       /* Code 0 */
  0xfc, 0xe4, 0xac, 0x54, 0x9b, 0x67                                     /* Codes 1-6 */
};

#    else /* DARK_CALIB_ICON */

static const uint8_t g_calibrationNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                       /* Code 0 */
  0xfc, 0xe4, 0xac, 0x54, 0x9b, 0x67                                     /* Codes 1-6 */
};

static const uint8_t g_calibrationBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                       /* Code 0 */
  0xff, 0xff, 0xd7, 0x69, 0xb9, 0x81                                     /* Codes 1-6 */
};

#    endif /* DARK_CALIB_ICON */

#  else /* CONFIG_NXWIDGETS_GREYSCALE */

/* RGB8 (332) Colors */

#    ifdef DARK_CALIB_ICON

static const nxgl_mxpixel_t g_calibrationNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                       /* Code 0 */
  0xb6, 0xb6, 0x92, 0x24, 0xac, 0x84                                     /* Codes 1-6 */
};

static const nxgl_mxpixel_t g_calibrationBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                       /* Code 0 */
  0xff, 0xff, 0xb6, 0x49, 0xf0, 0xa8                                     /* Codes 1-6 */
};

#    else /* DARK_CALIB_ICON */

static const nxgl_mxpixel_t g_calibrationNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                       /* Code 0 */
  0xff, 0xff, 0xb6, 0x49, 0xf0, 0xa8                                     /* Codes 1-6 */
};

static const nxgl_mxpixel_t g_calibrationBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                       /* Code 0 */
  0xff, 0xff, 0xdb, 0x6d, 0xf4, 0xcc                                     /* Codes 1-6 */
};

#    endif /* DARK_CALIB_ICON */
#  endif /* CONFIG_NXWIDGETS_GREYSCALE */
#else
# error Unsupported pixel format
#endif

static const struct SRlePaletteBitmapEntry g_calibrationRleEntries[] =
{
  {22, 0}, { 1, 1}, { 2, 2}, { 1, 3}, {12, 0}, { 2, 1}, { 8, 0},          /* Row 0 */
  {22, 0}, { 1, 1}, { 2, 2}, { 1, 3}, {11, 0}, { 1, 2}, { 2, 1}, { 1, 3}, /* Row 1 */
  { 2, 0}, { 1, 2}, { 4, 0},
  {22, 0}, { 1, 1}, { 2, 2}, { 1, 3}, {10, 0}, { 2, 1}, { 2, 4}, { 4, 1}, /* Row 2 */
  { 4, 0},
  {22, 0}, { 1, 1}, { 2, 2}, { 1, 3}, { 8, 0}, { 1, 2}, { 2, 1}, { 1, 3}, /* Row 3 */
  { 2, 4}, { 1, 1}, { 3, 3}, { 4, 0},
  {22, 0}, { 1, 1}, { 2, 2}, { 1, 3}, { 6, 0}, { 1, 2}, { 1, 1}, { 2, 3}, /* Row 4 */
  { 2, 4}, { 2, 2}, { 1, 3}, { 7, 0},
  {22, 0}, { 1, 1}, { 2, 2}, { 1, 3}, { 4, 0}, { 1, 2}, { 2, 1}, { 1, 3}, /* Row 5 */
  { 4, 0}, { 2, 2}, { 8, 0},
  {22, 0}, { 2, 1}, { 2, 2}, { 2, 0}, { 1, 2}, { 1, 1}, { 2, 3}, { 1, 4}, /* Row 6 */
  { 5, 0}, { 1, 1}, { 1, 2}, { 8, 0},
  {22, 0}, { 2, 1}, { 2, 2}, { 3, 1}, { 1, 3}, { 1, 4}, { 7, 0}, { 1, 1}, /* Row 7 */
  { 1, 2}, { 8, 0},
  {20, 0}, { 1, 2}, { 1, 1}, { 4, 2}, { 2, 3}, { 1, 4}, { 9, 0}, { 1, 1}, /* Row 8 */
  { 1, 2}, { 8, 0},
  {18, 0}, { 1, 2}, { 3, 1}, { 4, 2}, {12, 0}, { 1, 1}, { 1, 2}, { 8, 0}, /* Row 9 */
  { 8, 0}, { 1, 1}, { 1, 3}, { 6, 0}, { 2, 1}, { 2, 3}, { 1, 4}, { 1, 0}, /* Row 10 */
  { 2, 1}, { 1, 2}, { 1, 3}, {11, 0}, { 1, 1}, { 2, 2}, { 1, 4}, { 7, 0},
  { 4, 0}, { 1, 2}, { 2, 0}, { 2, 1}, { 2, 3}, { 3, 0}, { 1, 2}, { 2, 1}, /* Row 11 */
  { 1, 3}, { 1, 4}, { 3, 0}, { 1, 1}, { 2, 2}, { 1, 3}, {10, 0}, { 2, 1},
  { 3, 2}, { 1, 4}, { 6, 0},
  { 4, 0}, { 4, 1}, { 2, 4}, { 1, 3}, { 3, 1}, { 2, 3}, { 1, 4}, { 5, 0}, /* Row 12 */
  { 1, 1}, { 2, 2}, { 1, 3}, { 9, 0}, { 1, 1}, { 2, 2}, { 2, 0}, { 2, 2},
  { 1, 4}, { 5, 0},
  { 4, 0}, { 3, 3}, { 1, 1}, { 2, 4}, { 2, 1}, { 1, 3}, { 1, 4}, { 8, 0}, /* Row 13 */
  { 1, 1}, { 2, 2}, { 1, 3}, { 8, 0}, { 1, 2}, { 1, 1}, { 1, 2}, { 4, 0},
  { 2, 2}, { 1, 4}, { 4, 0},
  { 7, 0}, { 1, 3}, { 2, 2}, { 1, 3}, {11, 0}, { 1, 1}, { 2, 2}, { 1, 3}, /* Row 14 */
  { 8, 0}, { 1, 1}, { 1, 2}, { 6, 0}, { 2, 2}, { 4, 0},
  { 8, 0}, { 2, 2}, {12, 0}, { 1, 1}, { 2, 2}, { 1, 3}, { 7, 0}, { 1, 2}, /* Row 15 */
  { 1, 1}, { 8, 0}, { 1, 2}, { 1, 4}, { 3, 0},
  { 8, 0}, { 1, 1}, { 1, 2}, {12, 0}, { 1, 1}, { 2, 2}, { 1, 3}, { 7, 0}, /* Row 16 */
  { 1, 1}, { 1, 2}, { 8, 0}, { 2, 2}, { 3, 0},
  { 8, 0}, { 1, 1}, { 1, 2}, {12, 0}, { 1, 1}, { 2, 2}, { 1, 3}, { 6, 0}, /* Row 17 */
  { 1, 2}, { 1, 1}, { 1, 2}, { 8, 0}, { 1, 1}, { 1, 2}, { 1, 4}, { 2, 0},
  { 7, 0}, { 1, 1}, { 2, 2}, { 1, 4}, {11, 0}, { 1, 1}, { 2, 2}, { 1, 3}, /* Row 18 */
  { 6, 0}, { 2, 1}, {10, 0}, { 1, 1}, { 1, 2}, { 2, 0},
  { 6, 0}, { 2, 1}, { 3, 2}, { 1, 4}, {10, 0}, { 1, 1}, { 2, 2}, { 1, 3}, /* Row 19 */
  { 6, 0}, { 1, 1}, { 1, 2}, {10, 0}, { 1, 1}, { 1, 2}, { 2, 0},
  { 5, 0}, { 1, 1}, { 2, 2}, { 2, 0}, { 2, 2}, { 1, 4}, { 9, 0}, { 1, 1}, /* Row 20 */
  { 2, 2}, { 1, 3}, { 6, 0}, { 1, 1}, { 1, 2}, { 2, 0}, { 4, 5}, { 2, 6},
  { 2, 0}, { 1, 1}, { 1, 2}, { 2, 0},
  { 4, 0}, { 1, 2}, { 1, 1}, { 1, 2}, { 4, 0}, { 2, 2}, { 1, 4}, { 8, 0}, /* Row 21 */
  { 1, 1}, { 2, 2}, { 1, 3}, { 6, 0}, { 1, 1}, { 1, 2}, { 2, 0}, { 4, 5},
  { 2, 6}, { 2, 0}, { 1, 1}, { 1, 2}, { 2, 0},
  { 4, 0}, { 1, 1}, { 1, 2}, { 6, 0}, { 2, 2}, { 8, 0}, { 1, 1}, { 2, 2}, /* Row 22 */
  { 1, 3}, { 6, 0}, { 1, 1}, { 1, 2}, { 2, 0}, { 4, 5}, { 2, 6}, { 2, 0},
  { 1, 1}, { 1, 2}, { 2, 0},
  { 3, 0}, { 1, 2}, { 1, 1}, { 8, 0}, { 1, 2}, { 1, 4}, { 7, 0}, { 1, 1}, /* Row 23 */
  { 2, 2}, { 1, 3}, { 6, 0}, { 1, 1}, { 1, 2}, { 2, 0}, { 4, 5}, { 2, 6},
  { 2, 0}, { 1, 1}, { 1, 2}, { 2, 0},
  { 3, 0}, { 1, 1}, { 1, 2}, { 8, 0}, { 2, 2}, { 7, 0}, { 1, 1}, { 2, 2}, /* Row 24 */
  { 1, 3}, { 4, 0}, { 4, 1}, {10, 2}, { 4, 3},
  { 2, 0}, { 1, 2}, { 1, 1}, { 1, 2}, { 1, 0}, { 4, 5}, { 2, 6}, { 1, 0}, /* Row 25 */
  { 1, 1}, { 1, 2}, { 1, 4}, { 6, 0}, { 1, 1}, { 2, 2}, { 1, 3}, { 4, 0},
  { 4, 1}, {10, 2}, { 4, 3},
  { 2, 0}, { 2, 1}, { 2, 0}, { 4, 5}, { 2, 6}, { 2, 0}, { 1, 1}, { 1, 2}, /* Row 26 */
  { 6, 0}, { 1, 1}, { 2, 2}, { 1, 3}, {22, 0},
  { 2, 0}, { 1, 1}, { 1, 2}, { 2, 0}, { 4, 5}, { 2, 6}, { 2, 0}, { 1, 1}, /* Row 27 */
  { 1, 2}, { 6, 0}, { 1, 1}, { 2, 2}, { 1, 3}, {22, 0},
  { 2, 0}, { 1, 1}, { 1, 2}, { 2, 0}, { 4, 5}, { 2, 6}, { 2, 0}, { 1, 1}, /* Row 28 */
  { 1, 2}, { 6, 0}, { 1, 1}, { 2, 2}, { 1, 3}, {22, 0},
  { 2, 0}, { 1, 1}, { 1, 2}, { 2, 0}, { 4, 5}, { 2, 6}, { 2, 0}, { 1, 1}, /* Row 29 */
  { 1, 2}, { 6, 0}, { 1, 1}, { 2, 2}, { 1, 3}, {22, 0},
  { 2, 0}, { 1, 1}, { 1, 2}, { 2, 0}, { 4, 5}, { 2, 6}, { 2, 0}, { 1, 1}, /* Row 30 */
  { 1, 2}, { 6, 0}, { 1, 1}, { 2, 2}, { 1, 3}, {22, 0},
  { 2, 0}, { 1, 1}, { 1, 2}, { 2, 0}, { 4, 5}, { 2, 6}, { 2, 0}, { 1, 1}, /* Row 31 */
  { 1, 2}, { 6, 0}, { 1, 1}, { 2, 2}, { 1, 3}, {22, 0},
  { 4, 1}, {10, 2}, { 4, 3}, { 4, 0}, { 1, 1}, { 2, 2}, { 1, 3}, {22, 0}, /* Row 32 */
  { 4, 1}, {10, 2}, { 4, 3}, { 4, 0}, { 1, 1}, { 2, 2}, { 1, 3}, {22, 0}, /* Row 33 */
  {22, 0}, { 1, 1}, { 2, 2}, { 1, 3}, {22, 0},                            /* Row 34 */
  {22, 0}, { 1, 1}, { 2, 2}, { 1, 3}, {22, 0},                            /* Row 35 */
  {20, 0}, { 1, 2}, { 1, 1}, { 4, 2}, { 1, 3}, { 1, 4}, {20, 0},          /* Row 36 */
  {20, 0}, { 1, 2}, { 1, 1}, { 4, 2}, { 1, 3}, { 1, 4}, {20, 0},          /* Row 37 */
  {10, 0}, { 1, 2}, { 7, 1}, {12, 2}, { 7, 3}, { 1, 4}, {10, 0},          /* Row 38 */
  {10, 0}, { 1, 2}, { 7, 1}, {12, 2}, { 7, 3}, { 1, 4}, {10, 0},          /* Row 39 */
  { 8, 0}, { 1, 2}, { 7, 1}, {16, 2}, { 7, 3}, { 1, 4}, { 8, 0},          /* Row 40 */
  { 8, 0}, { 1, 2}, { 7, 1}, {16, 2}, { 7, 3}, { 1, 4}, { 8, 0}           /* Row 41 */
};

/********************************************************************************************
 * Public Bitmap Structure Definitions
 ********************************************************************************************/

const struct SRlePaletteBitmap NXWidgets::g_calibrationBitmap =
{
  CONFIG_NXWIDGETS_BPP,      // bpp    - Bits per pixel
  CONFIG_NXWIDGETS_FMT,      // fmt    - Color format
  BITMAP_NLUTCODES,          // nlut   - Number of colors in the lLook-Up Table (LUT)
  BITMAP_NCOLUMNS,           // width  - Width in pixels
  BITMAP_NROWS,              // height - Height in rows
  {                          // lut    - Pointer to the beginning of the Look-Up Table (LUT)
    g_calibrationNormalLut,  //          Index 0: Unselected LUT
    g_calibrationBrightLut,  //          Index 1: Selected LUT
  },
  g_calibrationRleEntries    // data   - Pointer to the beginning of the RLE data
};
