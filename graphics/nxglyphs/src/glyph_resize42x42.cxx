/********************************************************************************************
 * apps/graphics/nxglyphs/src/glyph_resize42x42.cxx
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

static const uint32_t g_resizeNormalLut[BITMAP_NLUTCODES] =
{
  0x90f0cc, 0x00b400, 0x00d800, 0x046414, 0xfcfcfc, 0xd8fcfc, 0x00a400,  /* Codes 0-6 */
};

static const uint32_t g_resizeBrightLut[BITMAP_NLUTCODES] =
{
  0xabf3d8, 0x3fc63f, 0x3fe13f, 0x428a4e, 0xfcfcfc, 0xe1fcfc, 0x3fba3f,  /* Codes 0-6 */
};

#elif CONFIG_NXWIDGETS_BPP == 16
// RGB16 (565) Colors (four of the colors in this map are duplicates)

static const uint16_t g_resizeNormalLut[BITMAP_NLUTCODES] =
{
  0x9799, 0x05a0, 0x06c0, 0x0322, 0xffff, 0xdfff, 0x0520,  /* Codes 0-6 */
};

static const uint16_t g_resizeBrightLut[BITMAP_NLUTCODES] =
{
  0xaf9b, 0x3e27, 0x3f07, 0x4449, 0xffff, 0xe7ff, 0x3dc7,  /* Codes 0-6 */
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

static const uint8_t g_resizeNormalLut[BITMAP_NLUTCODES] =
{
  0xcf, 0x69, 0x7e, 0x3e, 0xfc, 0xf1, 0x60,  /* Codes 0-6 */
}

static const uint8_t g_resizeBrightLut[BITMAP_NLUTCODES] =
{
  0xda, 0x8e, 0x9e, 0x6d, 0xfc, 0xf3, 0x87,  /* Codes 0-6 */
};

#  else /* CONFIG_NXWIDGETS_GREYSCALE */
// RGB8 (332) Colors

static const nxgl_mxpixel_t g_resizeNormalLut[BITMAP_NLUTCODES] =
{
  0x9f, 0x14, 0x18, 0x0c, 0xff, 0xdf, 0x14,  /* Codes 0-6 */
};

static const uint8_t g_resizeBrightLut[BITMAP_NLUTCODES] =
{
  0xbf, 0x59, 0x5d, 0x51, 0xff, 0xff, 0x55,  /* Codes 0-6 */
};

#  endif
#else
#  error Unsupported pixel format
#endif

static const struct SRlePaletteBitmapEntry g_resizeRleEntries[] =
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
  {  1,   0}, {  2,   1}, {  4,   2}, {  1,   4}, { 24,   5}, {  2,   4}, {  1,   5}, {  5,   1},    // Row 9
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
  {  1,   0}, {  6,   2}, {  1,   4}, {  1,   5}, {  7,   2}, { 17,   1}, {  1,   4}, {  1,   5},    // Row 15
  {  3,   1}, {  3,   6}, {  1,   3},
  {  1,   0}, {  6,   2}, {  1,   4}, {  1,   5}, {  6,   2}, { 18,   1}, {  1,   4}, {  1,   5},    // Row 16
  {  3,   1}, {  3,   6}, {  1,   3},
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
  {  1,   0}, {  6,   1}, {  1,   4}, {  1,   5}, { 24,   1}, {  1,   4}, {  1,   5}, {  6,   6},    // Row 22
  {  1,   3},
  {  1,   0}, {  6,   1}, {  1,   4}, {  1,   5}, { 24,   1}, {  1,   4}, {  1,   5}, {  6,   6},    // Row 23
  {  1,   3},
  {  1,   0}, {  6,   1}, {  1,   4}, {  1,   5}, { 23,   1}, {  1,   6}, {  1,   4}, {  1,   5},    // Row 24
  {  6,   6}, {  1,   3},
  {  1,   0}, {  6,   1}, {  1,   4}, {  1,   5}, { 22,   1}, {  2,   6}, {  1,   4}, {  1,   5},    // Row 25
  {  6,   6}, {  1,   3},
  {  1,   0}, {  6,   1}, {  1,   4}, {  1,   5}, { 21,   1}, {  3,   6}, {  1,   4}, {  1,   5},    // Row 26
  {  6,   6}, {  1,   3},
  {  1,   0}, {  6,   1}, {  1,   4}, {  1,   5}, { 20,   1}, {  4,   6}, {  1,   4}, {  1,   5},    // Row 27
  {  6,   6}, {  1,   3},
  {  1,   0}, {  6,   1}, {  1,   4}, {  1,   5}, { 19,   1}, {  5,   6}, {  1,   4}, {  1,   5},    // Row 28
  {  6,   6}, {  1,   3},
  {  1,   0}, {  6,   1}, {  1,   4}, {  1,   5}, { 18,   1}, {  6,   6}, {  1,   4}, {  1,   5},    // Row 29
  {  6,   6}, {  1,   3},
  {  1,   0}, {  6,   1}, {  1,   4}, {  1,   5}, { 16,   1}, {  8,   6}, {  1,   4}, {  1,   5},    // Row 30
  {  6,   6}, {  1,   3},
  {  1,   0}, {  6,   1}, {  1,   4}, {  1,   5}, { 14,   1}, { 10,   6}, {  1,   4}, {  1,   5},    // Row 31
  {  6,   6}, {  1,   3},
  {  1,   0}, {  6,   1}, {  1,   4}, {  1,   5}, { 13,   1}, { 11,   6}, {  1,   4}, {  1,   5},    // Row 32
  {  6,   6}, {  1,   3},
  {  1,   0}, {  6,   1}, {  2,   4}, { 12,   1}, { 12,   6}, {  1,   4}, {  1,   5}, {  6,   6},    // Row 33
  {  1,   3},
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

const struct SRlePaletteBitmap NXWidgets::g_resizeBitmap =
{
  CONFIG_NXWIDGETS_BPP,  // bpp    - Bits per pixel
  CONFIG_NXWIDGETS_FMT,  // fmt    - Color format
  BITMAP_NLUTCODES,      // nlut   - Number of colors in the lLook-Up Table (LUT)
  BITMAP_NCOLUMNS,       // width  - Width in pixels
  BITMAP_NROWS,          // height - Height in rows
  {                      // lut    - Pointer to the beginning of the Look-Up Table (LUT)
    g_resizeNormalLut,   //          Index 0: Unselected LUT
    g_resizeBrightLut,   //          Index 1: Selected LUT
  },
  g_resizeRleEntries     // data   - Pointer to the beginning of the RLE data
};
