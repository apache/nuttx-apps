/********************************************************************************************
 * apps/graphics/nxglyphs/src/glyph_men42x42.cxx
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

#include "graphics/nxwidgets/crlepalettebitmap.hxx"
#include "graphics/nxglyphs.hxx"

/********************************************************************************************
 * Pre-Processor Definitions
 ********************************************************************************************/

#define BITMAP_NROWS     42
#define BITMAP_NCOLUMNS  42
#define BITMAP_NLUTCODES 7

/********************************************************************************************
 * Private Bitmap Data
 ********************************************************************************************/

using namespace NXWidgets;

#if CONFIG_NXWIDGETS_BPP == 24 ||  CONFIG_NXWIDGETS_BPP == 32
// RGB24 (8-8-8) Colors

static const uint32_t g_menuNormalLut[BITMAP_NLUTCODES] =
{
  0xc49c4c, 0xdc9000, 0xfcb800, 0x9c5800, 0xfcfcfc, 0xdcb468, 0xc47c00,  /* Codes 0-6 */
};

static const uint32_t g_menuBrightLut[BITMAP_NLUTCODES] =
{
  0xd2b478, 0xe4ab3f, 0xfcc93f, 0xb4813f, 0xfcfcfc, 0xe4c68d, 0xd29c3f,  /* Codes 0-6 */
};

#elif CONFIG_NXWIDGETS_BPP == 16
// RGB16 (565) Colors (four of the colors in this map are duplicates)

static const uint16_t g_menuNormalLut[BITMAP_NLUTCODES] =
{
  0xc4e9, 0xdc80, 0xfdc0, 0x9ac0, 0xffff, 0xddad, 0xc3e0,  /* Codes 0-6 */
};

static const uint16_t g_menuBrightLut[BITMAP_NLUTCODES] =
{
  0xd5af, 0xe547, 0xfe47, 0xb407, 0xffff, 0xe631, 0xd4e7,  /* Codes 0-6 */
};

#elif CONFIG_NXWIDGETS_BPP == 8
// 8-bit color lookups.  NOTE:  This is really dumb!  The lookup index is 8-bits and it used
// to lookup an 8-bit value.  There is no savings in that!  It would be better to just put
// the 8-bit color/greyscale value in the run-length encoded image and save the cost of these
// pointless lookups.  But these p;ointless lookups do make the logic compatible with the
// 16- and 24-bit types.
///

#  ifdef CONFIG_NXWIDGETS_GREYSCALE
// 8-bit Greyscale

static const uint8_t g_menuNormalLut[BITMAP_NLUTCODES] =
{
  0x9e, 0x96, 0xb7, 0x62, 0xfc, 0xb7, 0x83,  /* Codes 0-6 */
}

static const uint8_t g_menuBrightLut[BITMAP_NLUTCODES] =
{
  0xb6, 0xaf, 0xc8, 0x88, 0xfc, 0xc8, 0xa1,  /* Codes 0-6 */
};

#  else /* CONFIG_NXWIDGETS_GREYSCALE */
// RGB8 (332) Colors

static const nxgl_mxpixel_t g_menuNormalLut[BITMAP_NLUTCODES] =
{
  0xd1, 0xd0, 0xf4, 0x88, 0xff, 0xd5, 0xcc,  /* Codes 0-6 */
};

static const uint8_t g_menuBrightLut[BITMAP_NLUTCODES] =
{
  0xd5, 0xf5, 0xf9, 0xb1, 0xff, 0xfa, 0xd1,  /* Codes 0-6 */
};

#  endif
#else
#  error Unsupported pixel format
#endif

static const struct SRlePaletteBitmapEntry g_menuRleEntries[] =
{
  { 42,   0},                                                                                        // Row 0
  {  1,   0}, { 11,   1}, { 10,   2}, { 19,   1}, {  1,   0},                                        // Row 1
  {  1,   0}, {  9,   1}, { 14,   2}, { 17,   1}, {  1,   3},                                        // Row 2
  {  1,   0}, {  7,   1}, { 17,   2}, { 16,   1}, {  1,   3},                                        // Row 3
  {  1,   0}, {  6,   1}, { 18,   2}, { 16,   1}, {  1,   3},                                        // Row 4
  {  1,   0}, {  5,   1}, { 19,   2}, { 16,   1}, {  1,   3},                                        // Row 5
  {  1,   0}, {  4,   1}, { 20,   2}, { 16,   1}, {  1,   3},                                        // Row 6
  {  1,   0}, {  3,   1}, { 20,   2}, { 17,   1}, {  1,   3},                                        // Row 7
  {  1,   0}, {  3,   1}, {  3,   2}, { 27,   4}, {  7,   1}, {  1,   3},                            // Row 8
  {  1,   0}, {  2,   1}, {  4,   2}, {  1,   4}, { 25,   5}, {  1,   4}, {  1,   5}, {  5,   1},    // Row 9
  {  1,   6}, {  1,   3},
  {  1,   0}, {  2,   1}, {  4,   2}, {  1,   4}, {  1,   5}, { 13,   2}, { 11,   1}, {  1,   4},    // Row 10
  {  1,   5}, {  5,   1}, {  1,   6}, {  1,   3},
  {  1,   0}, {  1,   1}, {  5,   2}, {  1,   4}, {  1,   5}, { 12,   2}, { 12,   1}, {  1,   4},    // Row 11
  {  1,   5}, {  5,   1}, {  1,   6}, {  1,   3},
  {  1,   0}, {  1,   1}, {  5,   2}, {  1,   4}, {  1,   5}, { 11,   2}, { 13,   1}, {  1,   4},    // Row 12
  {  1,   5}, {  5,   1}, {  1,   6}, {  1,   3},
  {  1,   0}, {  6,   2}, {  1,   4}, {  1,   5}, { 10,   2}, { 14,   1}, {  1,   4}, {  1,   5},    // Row 13
  {  4,   1}, {  2,   6}, {  1,   3},
  {  1,   0}, {  6,   2}, {  1,   4}, {  1,   5}, {  9,   2}, { 15,   1}, {  1,   4}, {  1,   5},    // Row 14
  {  4,   1}, {  2,   6}, {  1,   3},
  {  1,   0}, {  6,   2}, { 27,   4}, {  1,   5}, {  3,   1}, {  3,   6}, {  1,   3},                // Row 15
  {  1,   0}, {  6,   2}, {  1,   4}, { 25,   5}, {  1,   4}, {  1,   5}, {  3,   1}, {  3,   6},    // Row 16
  {  1,   3},
  {  1,   0}, {  6,   2}, {  1,   4}, {  1,   5}, {  4,   2}, { 20,   1}, {  1,   4}, {  1,   5},    // Row 17
  {  2,   1}, {  4,   6}, {  1,   3},
  {  1,   0}, {  6,   2}, {  1,   4}, {  1,   5}, {  2,   2}, { 22,   1}, {  1,   4}, {  1,   5},    // Row 18
  {  2,   1}, {  4,   6}, {  1,   3},
  {  1,   0}, {  1,   1}, {  5,   2}, {  1,   4}, {  1,   5}, {  1,   2}, { 23,   1}, {  1,   4},    // Row 19
  {  1,   5}, {  1,   1}, {  5,   6}, {  1,   3},
  {  1,   0}, {  2,   1}, {  4,   2}, {  1,   4}, {  1,   5}, { 24,   1}, {  1,   4}, {  1,   5},    // Row 20
  {  1,   1}, {  5,   6}, {  1,   3},
  {  1,   0}, {  4,   1}, {  2,   2}, {  1,   4}, {  1,   5}, { 24,   1}, {  1,   4}, {  1,   5},    // Row 21
  {  6,   6}, {  1,   3},
  {  1,   0}, {  6,   1}, { 27,   4}, {  1,   5}, {  6,   6}, {  1,   3},                            // Row 22
  {  1,   0}, {  6,   1}, {  1,   4}, { 25,   5}, {  1,   4}, {  1,   5}, {  6,   6}, {  1,   3},    // Row 23
  {  1,   0}, {  6,   1}, {  1,   4}, {  1,   5}, { 23,   1}, {  1,   6}, {  1,   4}, {  1,   5},    // Row 24
  {  6,   6}, {  1,   3},
  {  1,   0}, {  6,   1}, {  1,   4}, {  1,   5}, { 22,   1}, {  2,   6}, {  1,   4}, {  1,   5},    // Row 25
  {  6,   6}, {  1,   3},
  {  1,   0}, {  6,   1}, {  1,   4}, {  1,   5}, { 21,   1}, {  3,   6}, {  1,   4}, {  1,   5},    // Row 26
  {  6,   6}, {  1,   3},
  {  1,   0}, {  6,   1}, {  1,   4}, {  1,   5}, { 20,   1}, {  4,   6}, {  1,   4}, {  1,   5},    // Row 27
  {  6,   6}, {  1,   3},
  {  1,   0}, {  6,   1}, { 27,   4}, {  1,   5}, {  6,   6}, {  1,   3},                            // Row 28
  {  1,   0}, {  6,   1}, {  1,   4}, { 25,   5}, {  1,   4}, {  1,   5}, {  6,   6}, {  1,   3},    // Row 29
  {  1,   0}, {  6,   1}, {  1,   4}, {  1,   5}, { 16,   1}, {  8,   6}, {  1,   4}, {  1,   5},    // Row 30
  {  6,   6}, {  1,   3},
  {  1,   0}, {  6,   1}, {  1,   4}, {  1,   5}, { 14,   1}, { 10,   6}, {  1,   4}, {  1,   5},    // Row 31
  {  6,   6}, {  1,   3},
  {  1,   0}, {  6,   1}, {  1,   4}, {  1,   5}, { 13,   1}, { 11,   6}, {  1,   4}, {  1,   5},    // Row 32
  {  6,   6}, {  1,   3},
  {  1,   0}, {  6,   1}, {  1,   4}, {  1,   5}, { 12,   1}, { 12,   6}, {  1,   4}, {  1,   5},    // Row 33
  {  6,   6}, {  1,   3},
  {  1,   0}, {  6,   1}, { 27,   4}, {  1,   5}, {  6,   6}, {  1,   3},                            // Row 34
  {  1,   0}, {  7,   1}, { 27,   5}, {  6,   6}, {  1,   3},                                        // Row 35
  {  1,   0}, {  8,   1}, { 32,   6}, {  1,   3},                                                    // Row 36
  {  1,   0}, {  6,   1}, { 34,   6}, {  1,   3},                                                    // Row 37
  {  1,   0}, {  4,   1}, { 36,   6}, {  1,   3},                                                    // Row 38
  {  1,   0}, {  2,   1}, { 37,   6}, {  2,   3},                                                    // Row 39
  {  1,   0}, { 39,   6}, {  2,   3},                                                                // Row 40
  {  2,   0}, { 40,   3},                                                                            // Row 41
 };

/********************************************************************************************
 * Public Bitmap Structure Definitions
 ********************************************************************************************/

const struct SRlePaletteBitmap NXWidgets::g_menuBitmap =
{
  CONFIG_NXWIDGETS_BPP,  // bpp    - Bits per pixel
  CONFIG_NXWIDGETS_FMT,  // fmt    - Color format
  BITMAP_NLUTCODES,      // nlut   - Number of colors in the lLook-Up Table (LUT)
  BITMAP_NCOLUMNS,       // width  - Width in pixels
  BITMAP_NROWS,          // height - Height in rows
  {                      // lut    - Pointer to the beginning of the Look-Up Table (LUT)
    g_menuNormalLut,     //          Index 0: Unselected LUT
    g_menuBrightLut,     //          Index 1: Selected LUT
  },
  g_menuRleEntries       // data   - Pointer to the beginning of the RLE data
};
