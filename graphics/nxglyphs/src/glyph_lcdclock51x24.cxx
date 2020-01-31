/********************************************************************************************
 * apps/graphics/nxglyphs/src/glyph_lcdclock51x24.cxx
 *
 *   Copyright (C) 2019 Gregory Nutt. All rights reserved.
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

#include "graphics/nxwidgets/crlepalettebitmap.hxx"
#include "graphics/nxglyphs.hxx"

/********************************************************************************************
 * Pre-Processor Definitions
 ********************************************************************************************/

#define BITMAP_NROWS     24
#define BITMAP_NCOLUMNS  51
#define BITMAP_NLUTCODES 51

/********************************************************************************************
 * Private Bitmap Data
 ********************************************************************************************/

using namespace NXWidgets;

#if CONFIG_NXWIDGETS_BPP == 24 ||  CONFIG_NXWIDGETS_BPP == 32
// RGB24 (8-8-8) Colors

static const uint32_t g_lcdClockNormalLut[BITMAP_NLUTCODES] =
{
  0x808c80, 0x7c887c, 0x5c6c5c, 0x607060, 0x788478, 0x687868, 0x707c70, 0x485c48,  /* Codes 0-7 */
  0x445844, 0x6c786c, 0x0c2c0c, 0x042404, 0x102c10, 0x284028, 0x082808, 0x0c280c,  /* Codes 8-15 */
  0x344c34, 0x244024, 0x647464, 0x002000, 0x304830, 0x143014, 0x203820, 0x183418,  /* Codes 16-23 */
  0x2c442c, 0x203c20, 0x4c604c, 0x3c543c, 0x506450, 0x1c381c, 0x405840, 0x082408,  /* Codes 24-31 */
  0x748074, 0x385038, 0x103010, 0x788878, 0x586c58, 0x042004, 0x486048, 0x546854,  /* Codes 32-39 */
  0x3c503c, 0x708070, 0x546454, 0x6c7c6c, 0x243c24, 0x748474, 0x405440, 0x445c44,  /* Codes 40-47 */
  0x2c482c, 0x5c705c, 0x90986c,  /* Codes 48-50 */
};

static const uint32_t g_lcdClockBrightLut[BITMAP_NLUTCODES] =
{
  0x9fa89f, 0x9ca59c, 0x849084, 0x879387, 0x99a299, 0x8d998d, 0x939c93, 0x758475,  /* Codes 0-7 */
  0x728172, 0x909990, 0x486048, 0x425a42, 0x4b604b, 0x5d6f5d, 0x455d45, 0x485d48,  /* Codes 8-15 */
  0x667866, 0x5a6f5a, 0x8a968a, 0x3f573f, 0x637563, 0x4e634e, 0x576957, 0x516651,  /* Codes 16-23 */
  0x607260, 0x576c57, 0x788778, 0x6c7e6c, 0x7b8a7b, 0x546954, 0x6f816f, 0x455a45,  /* Codes 24-31 */
  0x969f96, 0x697b69, 0x4b634b, 0x99a599, 0x819081, 0x425742, 0x758775, 0x7e8d7e,  /* Codes 32-39 */
  0x6c7b6c, 0x939f93, 0x7e8a7e, 0x909c90, 0x5a6c5a, 0x96a296, 0x6f7e6f, 0x728472,  /* Codes 40-47 */
  0x607560, 0x849384, 0xabb190,  /* Codes 48-50 */
};

#elif CONFIG_NXWIDGETS_BPP == 16
// RGB16 (565) Colors (four of the colors in this map are duplicates)

static const uint16_t g_lcdClockNormalLut[BITMAP_NLUTCODES] =
{
  0x8470, 0x7c4f, 0x5b6b, 0x638c, 0x7c2f, 0x6bcd, 0x73ee, 0x4ae9, 0x42c8, 0x6bcd,  /* Codes 0-9 */
  0x0961, 0x0120, 0x1162, 0x2a05, 0x0941, 0x0941, 0x3266, 0x2204, 0x63ac, 0x0100,  /* Codes 10-19 */
  0x3246, 0x1182, 0x21c4, 0x19a3, 0x2a25, 0x21e4, 0x4b09, 0x3aa7, 0x532a, 0x19c3,  /* Codes 20-29 */
  0x42c8, 0x0921, 0x740e, 0x3a87, 0x1182, 0x7c4f, 0x5b6b, 0x0100, 0x4b09, 0x534a,  /* Codes 30-39 */
  0x3a87, 0x740e, 0x532a, 0x6bed, 0x21e4, 0x742e, 0x42a8, 0x42e8, 0x2a45, 0x5b8b,  /* Codes 40-49 */
  0x94cd,  /* Codes 50-50 */
};

static const uint16_t g_lcdClockBrightLut[BITMAP_NLUTCODES] =
{
  0x9d53, 0x9d33, 0x8490, 0x8490, 0x9d13, 0x8cd1, 0x94f2, 0x742e, 0x740e, 0x94d2,  /* Codes 0-9 */
  0x4b09, 0x42c8, 0x4b09, 0x5b6b, 0x42e8, 0x4ae9, 0x63cc, 0x5b6b, 0x8cb1, 0x3aa7,  /* Codes 10-19 */
  0x63ac, 0x4b09, 0x534a, 0x532a, 0x638c, 0x536a, 0x7c2f, 0x6bed, 0x7c4f, 0x534a,  /* Codes 20-29 */
  0x6c0d, 0x42c8, 0x94f2, 0x6bcd, 0x4b09, 0x9d33, 0x8490, 0x42a8, 0x742e, 0x7c6f,  /* Codes 30-39 */
  0x6bcd, 0x94f2, 0x7c4f, 0x94f2, 0x5b6b, 0x9512, 0x6bed, 0x742e, 0x63ac, 0x8490,  /* Codes 40-49 */
  0xad92,  /* Codes 50-50 */
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

static const uint8_t g_lcdClockNormalLut[BITMAP_NLUTCODES] =
{
  0x87, 0x83, 0x65, 0x69, 0x7f, 0x71, 0x77, 0x53, 0x4f, 0x73, 0x1e, 0x16, 0x20, 0x36, 0x1a, 0x1c,  /* Codes 0-15 */
  0x42, 0x34, 0x6d, 0x12, 0x3e, 0x24, 0x2e, 0x28, 0x3a, 0x30, 0x57, 0x4a, 0x5b, 0x2c, 0x4e, 0x18,  /* Codes 16-31 */
  0x7b, 0x46, 0x22, 0x81, 0x63, 0x14, 0x56, 0x5f, 0x47, 0x79, 0x5d, 0x75, 0x32, 0x7d, 0x4b, 0x52,  /* Codes 32-47 */
  0x3c, 0x67, 0x90,  /* Codes 48-50 */
}

static const uint8_t g_lcdClockBrightLut[BITMAP_NLUTCODES] =
{
  0xa4, 0xa1, 0x8b, 0x8e, 0x9e, 0x94, 0x98, 0x7d, 0x7a, 0x95, 0x56, 0x50, 0x57, 0x67, 0x53, 0x54,  /* Codes 0-15 */
  0x70, 0x66, 0x91, 0x4d, 0x6d, 0x5a, 0x61, 0x5d, 0x6a, 0x63, 0x80, 0x76, 0x83, 0x60, 0x79, 0x51,  /* Codes 16-31 */
  0x9b, 0x73, 0x59, 0xa0, 0x89, 0x4e, 0x7f, 0x86, 0x74, 0x9a, 0x85, 0x97, 0x64, 0x9d, 0x77, 0x7c,  /* Codes 32-47 */
  0x6c, 0x8c, 0xab,  /* Codes 48-50 */
};

#  else /* CONFIG_NXWIDGETS_GREYSCALE */
// RGB8 (332) Colors

static const nxgl_mxpixel_t g_lcdClockNormalLut[BITMAP_NLUTCODES] =
{
  0x92, 0x72, 0x4d, 0x6d, 0x71, 0x6d, 0x6d, 0x49, 0x49, 0x6d, 0x04, 0x04, 0x04, 0x28, 0x04, 0x04,  /* Codes 0-15 */
  0x28, 0x28, 0x6d, 0x04, 0x28, 0x04, 0x24, 0x04, 0x28, 0x24, 0x4d, 0x29, 0x4d, 0x04, 0x49, 0x04,  /* Codes 16-31 */
  0x71, 0x28, 0x04, 0x71, 0x4d, 0x04, 0x4d, 0x4d, 0x29, 0x71, 0x4d, 0x6d, 0x24, 0x71, 0x49, 0x49,  /* Codes 32-47 */
  0x28, 0x4d, 0x91,  /* Codes 48-50 */
};

static const uint8_t g_lcdClockBrightLut[BITMAP_NLUTCODES] =
{
  0xb6, 0x96, 0x92, 0x92, 0x96, 0x92, 0x92, 0x71, 0x71, 0x92, 0x4d, 0x49, 0x4d, 0x4d, 0x49, 0x49,  /* Codes 0-15 */
  0x6d, 0x4d, 0x92, 0x49, 0x6d, 0x4d, 0x4d, 0x4d, 0x6d, 0x4d, 0x71, 0x71, 0x71, 0x4d, 0x71, 0x49,  /* Codes 16-31 */
  0x96, 0x6d, 0x4d, 0x96, 0x92, 0x49, 0x71, 0x92, 0x6d, 0x96, 0x92, 0x92, 0x4d, 0x96, 0x71, 0x71,  /* Codes 32-47 */
  0x6d, 0x92, 0xb6,  /* Codes 48-50 */
};

#  endif
#else
#  error Unsupported pixel format
#endif

static const struct SRlePaletteBitmapEntry g_lcdClockRleEntries[] =
{
  { 51,   0},                                                                                        // Row 0
  { 11,   0}, {  1,   1}, {  5,   2}, {  1,   3}, {  1,   4}, {  9,   0}, {  1,   1}, {  1,   5},    // Row 1
  {  5,   2}, {  1,   6}, { 15,   0},
  {  6,   0}, {  1,   7}, {  1,   8}, {  2,   0}, {  1,   9}, {  1,  10}, {  5,  11}, {  1,  12},    // Row 2
  {  2,  13}, {  8,   0}, {  1,  13}, {  1,  14}, {  4,  11}, {  1,  15}, {  1,  16}, {  1,   3},
  {  3,   0}, {  1,   7}, {  5,  10}, {  1,  17}, {  1,   2}, {  1,  18}, {  2,   0},
  {  5,   0}, {  1,   4}, {  1,  12}, {  1,  11}, {  3,   0}, {  1,  16}, {  1,  12}, {  4,  19},    // Row 3
  {  1,  20}, {  1,  21}, {  1,  19}, {  8,   0}, {  1,  16}, {  1,  15}, {  4,  19}, {  1,  16},
  {  1,  17}, {  1,  19}, {  2,   0}, {  1,   5}, {  1,  22}, {  1,  12}, {  4,  19}, {  1,   7},
  {  1,  23}, {  1,  10}, {  2,   0},
  {  5,   0}, {  1,  24}, {  1,  19}, {  1,  25}, {  3,   0}, {  1,   1}, {  1,   5}, {  4,  26},    // Row 4
  {  1,  27}, {  2,  19}, {  8,   0}, {  1,   1}, {  1,   3}, {  4,  26}, {  1,  28}, {  1,  11},
  {  1,  19}, {  2,   0}, {  1,  21}, {  1,  29}, {  1,   8}, {  4,  26}, {  1,  30}, {  1,  11},
  {  1,  10}, {  2,   0},
  {  5,   0}, {  1,  31}, {  1,  19}, {  1,  13}, {  9,   0}, {  2,  19}, {  1,  16}, { 14,   0},    // Row 5
  {  1,  16}, {  1,  19}, {  1,  23}, {  2,   0}, {  1,  31}, {  1,  19}, {  1,  27}, {  3,   0},
  {  1,   1}, {  1,  17}, {  1,  19}, {  1,  13}, {  2,   0},
  {  4,   0}, {  1,  32}, {  1,  11}, {  1,  19}, {  1,   8}, {  9,   0}, {  2,  19}, {  1,  33},    // Row 6
  { 14,   0}, {  1,  34}, {  1,  19}, {  1,  33}, {  1,   0}, {  1,  32}, {  1,  11}, {  1,  19},
  {  1,  28}, {  3,   0}, {  1,  35}, {  1,  23}, {  1,  19}, {  1,   7}, {  2,   0},
  {  4,   0}, {  1,  36}, {  1,  37}, {  1,  19}, {  1,  28}, {  9,   0}, {  2,  19}, {  1,   7},    // Row 7
  { 14,   0}, {  2,  19}, {  1,  33}, {  1,   0}, {  1,  36}, {  1,  37}, {  1,  19}, {  1,  28},
  {  3,   0}, {  1,   4}, {  2,  19}, {  1,   7}, {  2,   0},
  {  4,   0}, {  1,  36}, {  1,  37}, {  1,  19}, {  1,  28}, {  8,   0}, {  1,  38}, {  2,  19},    // Row 8
  {  1,   6}, { 13,   0}, {  1,  18}, {  2,  19}, {  1,  39}, {  1,   0}, {  1,  36}, {  1,  37},
  {  1,  19}, {  1,  28}, {  3,   0}, {  1,   4}, {  2,  19}, {  1,  18}, {  2,   0},
  {  4,   0}, {  1,  16}, {  1,  19}, {  1,  34}, {  1,   5}, {  8,   0}, {  1,  38}, {  2,  19},    // Row 9
  {  1,   6}, { 13,   0}, {  1,  38}, {  2,  19}, {  1,   6}, {  1,   0}, {  1,  16}, {  1,  19},
  {  1,  34}, {  1,   5}, {  3,   0}, {  1,  40}, {  2,  19}, {  3,   0},
  {  4,   0}, {  1,  30}, {  1,  19}, {  1,  34}, {  1,   5}, {  8,   0}, {  1,  38}, {  1,  19},    // Row 10
  {  1,  15}, {  1,  32}, { 13,   0}, {  1,  38}, {  1,  19}, {  1,  15}, {  1,  32}, {  1,   0},
  {  1,  16}, {  1,  19}, {  1,  23}, {  1,  41}, {  3,   0}, {  1,  40}, {  2,  19}, {  3,   0},
  {  5,   0}, {  1,  24}, {  1,  21}, {  1,   5}, {  1,   0}, {  1,   1}, {  1,  18}, {  1,  29},    // Row 11
  {  4,  19}, {  1,  23}, {  1,   7}, {  1,  17}, {  1,   4}, {  6,   0}, {  1,   1}, {  1,   4},
  {  1,  42}, {  4,  19}, {  1,  20}, {  1,  16}, {  1,  17}, {  1,   4}, {  1,  41}, {  1,  29},
  {  1,  22}, {  1,   7}, {  1,  14}, {  3,  19}, {  1,  33}, {  1,  17}, {  1,  29}, {  3,   0},
  {  5,   0}, {  1,  18}, {  1,  33}, {  1,   4}, {  1,   0}, {  1,   7}, {  1,  20}, {  5,  19},    // Row 12
  {  1,  14}, {  1,  16}, {  1,  43}, {  1,   1}, {  6,   0}, {  1,   4}, {  1,  16}, {  5,  19},
  {  1,  14}, {  1,  20}, {  1,  26}, {  1,   4}, {  1,   1}, {  1,   3}, {  1,  16}, {  6,  19},
  {  1,  27}, {  1,  26}, {  3,   0},
  {  3,   0}, {  1,   4}, {  1,  33}, {  1,  10}, {  1,  17}, {  2,   0}, {  1,  19}, {  1,  25},    // Row 13
  {  5,  20}, {  1,   3}, {  1,   4}, {  9,   0}, {  1,   6}, {  1,   7}, {  3,  20}, {  1,  27},
  {  1,  40}, {  1,  15}, {  1,  44}, {  1,   4}, {  2,   0}, {  1,   9}, {  6,  20}, {  1,  12},
  {  1,  16}, {  3,   0},
  {  3,   0}, {  1,   3}, {  2,  19}, {  1,  24}, {  1,   0}, {  1,  45}, {  2,  19}, {  1,   8},    // Row 14
  { 20,   0}, {  1,   3}, {  1,  14}, {  1,  19}, {  1,   8}, {  8,   0}, {  1,  28}, {  2,  19},
  {  1,  16}, {  3,   0},
  {  3,   0}, {  1,   3}, {  2,  19}, {  1,  28}, {  1,   0}, {  1,  46}, {  2,  19}, {  1,   3},    // Row 15
  { 20,   0}, {  1,   3}, {  1,  14}, {  1,  19}, {  1,   8}, {  8,   0}, {  1,  28}, {  2,  19},
  {  1,  39}, {  3,   0},
  {  3,   0}, {  1,  26}, {  2,  19}, {  1,  28}, {  1,   0}, {  1,  46}, {  2,  19}, { 21,   0},    // Row 16
  {  1,   7}, {  2,  19}, {  1,   8}, {  8,   0}, {  1,  28}, {  2,  19}, {  1,   3}, {  3,   0},
  {  3,   0}, {  1,  20}, {  2,  19}, {  1,   2}, {  1,   0}, {  1,  20}, {  2,  19}, { 21,   0},    // Row 17
  {  1,   8}, {  1,  19}, {  1,  14}, {  1,   3}, {  8,   0}, {  1,  17}, {  1,  19}, {  1,  11},
  {  1,  18}, {  3,   0},
  {  3,   0}, {  1,  20}, {  2,  19}, {  2,   0}, {  2,  19}, {  1,  23}, { 21,   0}, {  1,   8},    // Row 18
  {  1,  19}, {  1,  14}, {  1,   3}, {  8,   0}, {  1,  17}, {  1,  19}, {  1,  23}, {  1,   6},
  {  3,   0},
  {  3,   0}, {  1,  20}, {  2,  19}, {  2,   0}, {  1,  19}, {  1,  21}, {  1,  28}, { 21,   0},    // Row 19
  {  1,   8}, {  1,  19}, {  1,  14}, {  1,   3}, {  8,   0}, {  1,  17}, {  1,  19}, {  1,  23},
  {  1,   6}, {  3,   0},
  {  3,   0}, {  1,  27}, {  1,  19}, {  1,  12}, {  2,   0}, {  1,  47}, {  1,  17}, {  1,  23},    // Row 20
  {  3,  48}, {  1,  20}, {  1,  18}, {  9,   0}, {  1,  49}, {  1,  27}, {  4,  48}, {  1,  20},
  {  1,  33}, {  1,  11}, {  1,  34}, {  1,  32}, {  1,   0}, {  1,  18}, {  1,  30}, {  4,  48},
  {  1,  46}, {  1,  27}, {  1,  19}, {  1,  29}, {  1,  41}, {  3,   0},
  {  3,   0}, {  1,   5}, {  1,  12}, {  1,   3}, {  2,   0}, {  7,  19}, {  1,  33}, {  1,   1},    // Row 21
  {  8,   0}, {  1,  21}, {  6,  19}, {  1,  29}, {  1,  14}, {  1,  17}, {  2,   0}, {  1,  23},
  {  6,  19}, {  1,  16}, {  1,  19}, {  1,  33}, {  1,   1}, {  3,   0},
  {  4,   0}, {  1,   5}, {  1,   1}, {  2,   0}, {  1,   2}, {  1,  21}, {  5,  19}, {  1,  12},    // Row 22
  {  1,  43}, {  8,   0}, {  1,   1}, {  7,  19}, {  1,  20}, {  1,   1}, {  2,   0}, {  1,  26},
  {  1,  15}, {  6,  19}, {  1,  46}, {  1,   1}, {  4,   0},
  {  1,  50}, { 25,   0}, {  1,   4}, {  1,  36}, {  4,  42}, {  1,  49}, {  1,   1}, {  4,   0},    // Row 23
  {  1,  18}, {  6,  42}, {  6,   0},
 };

/********************************************************************************************
 * Public Bitmap Structure Definitions
 ********************************************************************************************/

const struct SRlePaletteBitmap NXWidgets::g_lcdClockBitmap =
{
  CONFIG_NXWIDGETS_BPP,  // bpp    - Bits per pixel
  CONFIG_NXWIDGETS_FMT,  // fmt    - Color format
  BITMAP_NLUTCODES,      // nlut   - Number of colors in the lLook-Up Table (LUT)
  BITMAP_NCOLUMNS,       // width  - Width in pixels
  BITMAP_NROWS,          // height - Height in rows
  {                      // lut    - Pointer to the beginning of the Look-Up Table (LUT)
    g_lcdClockNormalLut, //          Index 0: Unselected LUT
    g_lcdClockBrightLut, //          Index 1: Selected LUT
  },
  g_lcdClockRleEntries   // data   - Pointer to the beginning of the RLE data
};
