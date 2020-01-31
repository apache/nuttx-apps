/********************************************************************************************
 * apps/graphics/nxglyphs/src/glyph_nxicon42x42.cxx
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

#define BITMAP_NROWS     42
#define BITMAP_NCOLUMNS  42
#define BITMAP_NLUTCODES 52

/********************************************************************************************
 * Private Bitmap Data
 ********************************************************************************************/

using namespace NXWidgets;

#if CONFIG_NXWIDGETS_BPP == 24 ||  CONFIG_NXWIDGETS_BPP == 32
// RGB24 (8-8-8) Colors

static const uint32_t g_nxiconNormalLut[BITMAP_NLUTCODES] =
{
  0xfcfcfc, 0x3830e0, 0x3c34e0, 0x3828e0, 0xe0f0f8, 0xdcd8f8, 0x8488e8, 0xd4ecf0,  /* Codes 0-7 */
  0x2c20e0, 0x3830d0, 0xcce0f0, 0xd4e8f4, 0x5894bc, 0xc8e0f0, 0x181074, 0x2c2894,  /* Codes 8-15 */
  0xf4f8f8, 0xe4f0f4, 0xe4f0f8, 0x282480, 0x3c34ec, 0xc8e0ec, 0x90bcd4, 0x848cf0,  /* Codes 16-23 */
  0x4c8cb4, 0xe0ecf4, 0x302cb0, 0x7c84b8, 0xd8ecf8, 0x282478, 0x3028e4, 0x100870,  /* Codes 24-31 */
  0x08006c, 0x3c34e8, 0x3028e0, 0x20187c, 0xdcf4f0, 0xf0f4f8, 0x5ca4e8, 0x58a0e4,  /* Codes 32-39 */
  0xd8ecf4, 0xc4dcec, 0xdcf4fc, 0xd0e8f4, 0xd0d0f8, 0x94bcd4, 0x5080b0, 0xd4e4f4,  /* Codes 40-47 */
  0x303c8c, 0x1c1870, 0x3830cc, 0x382ce0,  /* Codes 48-51 */
};

static const uint32_t g_nxiconBrightLut[BITMAP_NLUTCODES] =
{
  0xfcfcfc, 0x6963e7, 0x6c66e7, 0x695de7, 0xe7f3f9, 0xe4e1f9, 0xa2a5ed, 0xdef0f3,  /* Codes 0-7 */
  0x6057e7, 0x6963db, 0xd8e7f3, 0xdeedf6, 0x81aecc, 0xd5e7f3, 0x514b96, 0x605dae,  /* Codes 8-15 */
  0xf6f9f9, 0xeaf3f6, 0xeaf3f9, 0x5d5a9f, 0x6c66f0, 0xd5e7f0, 0xabccde, 0xa2a8f3,  /* Codes 16-23 */
  0x78a8c6, 0xe7f0f6, 0x6360c3, 0x9ca2c9, 0xe1f0f9, 0x5d5a99, 0x635dea, 0x4b4593,  /* Codes 24-31 */
  0x453f90, 0x6c66ed, 0x635de7, 0x57519c, 0xe4f6f3, 0xf3f6f9, 0x84baed, 0x81b7ea,  /* Codes 32-39 */
  0xe1f0f6, 0xd2e4f0, 0xe4f6fc, 0xdbedf6, 0xdbdbf9, 0xaeccde, 0x7b9fc3, 0xdeeaf6,  /* Codes 40-47 */
  0x636ca8, 0x545193, 0x6963d8, 0x6960e7,  /* Codes 48-51 */
};

#elif CONFIG_NXWIDGETS_BPP == 16
// RGB16 (565) Colors (four of the colors in this map are duplicates)

static const uint16_t g_nxiconNormalLut[BITMAP_NLUTCODES] =
{
  0xffff, 0x399c, 0x39bc, 0x395c, 0xe79f, 0xdedf, 0x845d, 0xd77e, 0x291c, 0x399a,  /* Codes 0-9 */
  0xcf1e, 0xd75e, 0x5cb7, 0xcf1e, 0x188e, 0x2952, 0xf7df, 0xe79e, 0xe79f, 0x2930,  /* Codes 10-19 */
  0x39bd, 0xcf1d, 0x95fa, 0x847e, 0x4c76, 0xe77e, 0x3176, 0x7c37, 0xdf7f, 0x292f,  /* Codes 20-29 */
  0x315c, 0x104e, 0x080d, 0x39bd, 0x315c, 0x20cf, 0xdfbe, 0xf7bf, 0x5d3d, 0x5d1c,  /* Codes 30-39 */
  0xdf7e, 0xc6fd, 0xdfbf, 0xd75e, 0xd69f, 0x95fa, 0x5416, 0xd73e, 0x31f1, 0x18ce,  /* Codes 40-49 */
  0x3999, 0x397c,  /* Codes 50-51 */
};

static const uint16_t g_nxiconBrightLut[BITMAP_NLUTCODES] =
{
  0xffff, 0x6b1c, 0x6b3c, 0x6afc, 0xe79f, 0xe71f, 0xa53d, 0xdf9e, 0x62bc, 0x6b1b,  /* Codes 0-9 */
  0xdf3e, 0xdf7e, 0x8579, 0xd73e, 0x5252, 0x62f5, 0xf7df, 0xef9e, 0xef9f, 0x5ad3,  /* Codes 10-19 */
  0x6b3e, 0xd73e, 0xae7b, 0xa55e, 0x7d58, 0xe79e, 0x6318, 0x9d19, 0xe79f, 0x5ad3,  /* Codes 20-29 */
  0x62fd, 0x4a32, 0x41f2, 0x6b3d, 0x62fc, 0x5293, 0xe7be, 0xf7bf, 0x85dd, 0x85bd,  /* Codes 30-39 */
  0xe79e, 0xd73e, 0xe7bf, 0xdf7e, 0xdedf, 0xae7b, 0x7cf8, 0xdf5e, 0x6375, 0x5292,  /* Codes 40-49 */
  0x6b1b, 0x6b1c,  /* Codes 50-51 */
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

static const uint8_t g_nxiconNormalLut[BITMAP_NLUTCODES] =
{
  0xfc, 0x46, 0x4a, 0x41, 0xec, 0xdc, 0x91, 0xe5, 0x39, 0x44, 0xdb, 0xe3, 0x86, 0xda, 0x1d, 0x35,  /* Codes 0-15 */
  0xf6, 0xec, 0xed, 0x2f, 0x4b, 0xda, 0xb1, 0x95, 0x7d, 0xe9, 0x3c, 0x87, 0xe7, 0x2e, 0x3f, 0x16,  /* Codes 16-31 */
  0x0e, 0x4a, 0x3f, 0x25, 0xec, 0xf3, 0x96, 0x92, 0xe6, 0xd6, 0xed, 0xe2, 0xd4, 0xb2, 0x77, 0xe1,  /* Codes 32-47 */
  0x41, 0x23, 0x44, 0x44,  /* Codes 48-51 */
}

static const uint8_t g_nxiconBrightLut[BITMAP_NLUTCODES] =
{
  0xfc, 0x73, 0x76, 0x70, 0xf0, 0xe4, 0xac, 0xea, 0x6a, 0x72, 0xe3, 0xe9, 0xa3, 0xe2, 0x55, 0x67,  /* Codes 0-15 */
  0xf8, 0xf0, 0xf0, 0x62, 0x77, 0xe2, 0xc4, 0xae, 0x9d, 0xed, 0x6c, 0xa4, 0xec, 0x62, 0x6e, 0x4f,  /* Codes 16-31 */
  0x4a, 0x77, 0x6e, 0x5b, 0xf0, 0xf5, 0xaf, 0xac, 0xec, 0xdf, 0xf1, 0xe8, 0xde, 0xc5, 0x98, 0xe7,  /* Codes 32-47 */
  0x70, 0x59, 0x72, 0x72,  /* Codes 48-51 */
};

#  else /* CONFIG_NXWIDGETS_GREYSCALE */
// RGB8 (332) Colors

static const nxgl_mxpixel_t g_nxiconNormalLut[BITMAP_NLUTCODES] =
{
  0xff, 0x27, 0x27, 0x27, 0xff, 0xdb, 0x93, 0xdf, 0x27, 0x27, 0xdf, 0xdf, 0x53, 0xdf, 0x01, 0x26,  /* Codes 0-15 */
  0xff, 0xff, 0xff, 0x26, 0x27, 0xdf, 0x97, 0x93, 0x52, 0xff, 0x26, 0x72, 0xdf, 0x25, 0x27, 0x01,  /* Codes 16-31 */
  0x01, 0x27, 0x27, 0x22, 0xdf, 0xff, 0x57, 0x57, 0xdf, 0xdb, 0xdf, 0xdf, 0xdb, 0x97, 0x52, 0xdf,  /* Codes 32-47 */
  0x26, 0x01, 0x27, 0x27,  /* Codes 48-51 */
};

static const uint8_t g_nxiconBrightLut[BITMAP_NLUTCODES] =
{
  0xff, 0x6f, 0x6f, 0x6b, 0xff, 0xff, 0xb7, 0xff, 0x6b, 0x6f, 0xdf, 0xff, 0x97, 0xdf, 0x4a, 0x6a,  /* Codes 0-15 */
  0xff, 0xff, 0xff, 0x4a, 0x6f, 0xdf, 0xbb, 0xb7, 0x77, 0xff, 0x6f, 0x97, 0xff, 0x4a, 0x6b, 0x4a,  /* Codes 16-31 */
  0x4a, 0x6f, 0x6b, 0x4a, 0xff, 0xff, 0x97, 0x97, 0xff, 0xdf, 0xff, 0xdf, 0xdb, 0xbb, 0x77, 0xff,  /* Codes 32-47 */
  0x6e, 0x4a, 0x6f, 0x6f,  /* Codes 48-51 */
};

#  endif
#else
#  error Unsupported pixel format
#endif

static const struct SRlePaletteBitmapEntry g_nxiconRleEntries[] =
{
  { 20,   0}, {  1,   1}, {  1,   2}, { 20,   0},                                                    // Row 0
  { 19,   0}, {  1,   1}, {  1,   3}, {  1,   1}, {  1,   2}, { 19,   0},                            // Row 1
  { 18,   0}, {  1,   1}, {  1,   3}, {  2,   2}, {  1,   1}, {  1,   2}, { 18,   0},                // Row 2
  { 17,   0}, {  1,   1}, {  1,   3}, {  4,   2}, {  1,   1}, {  1,   2}, { 17,   0},                // Row 3
  { 15,   0}, {  1,   4}, {  1,   1}, {  1,   3}, {  6,   2}, {  1,   1}, {  1,   2}, {  1,   5},    // Row 4
  { 15,   0},
  { 15,   0}, {  1,   1}, {  3,   2}, {  1,   6}, {  1,   7}, {  1,   8}, {  3,   2}, {  1,   9},    // Row 5
  {  1,   2}, {  6,   0}, {  3,  10}, {  6,   0},
  {  5,   0}, {  1,  11}, {  3,  10}, {  1,  12}, {  4,   0}, {  1,   1}, {  1,   3}, {  2,   2},    // Row 6
  {  1,   8}, {  1,  10}, {  1,  13}, {  1,  10}, {  1,  14}, {  1,  15}, {  2,   2}, {  1,   1},
  {  1,   2}, {  5,   0}, {  1,  10}, {  1,   0}, {  1,  10}, {  1,  11}, {  5,   0},
  {  5,   0}, {  1,  10}, {  1,   0}, {  1,  16}, {  1,  10}, {  2,  12}, {  2,   0}, {  1,   1},    // Row 7
  {  1,   3}, {  3,   2}, {  1,   7}, {  1,  17}, {  1,   0}, {  1,  18}, {  1,  10}, {  1,  19},
  {  1,  20}, {  2,   2}, {  1,   1}, {  1,   2}, {  3,   0}, {  1,  10}, {  1,  18}, {  1,   0},
  {  1,  21}, {  1,  10}, {  1,  12}, {  4,   0},
  {  5,   0}, {  1,  10}, {  2,   0}, {  2,  10}, {  1,  12}, {  1,   0}, {  1,   1}, {  1,   3},    // Row 8
  {  4,   2}, {  1,   7}, {  3,   0}, {  1,  10}, {  1,  14}, {  1,  19}, {  3,   2}, {  1,   1},
  {  1,   2}, {  2,   0}, {  1,  10}, {  2,   0}, {  1,  10}, {  2,  12}, {  4,   0},
  {  5,   0}, {  1,  10}, {  3,   0}, {  1,  10}, {  1,  22}, {  1,  19}, {  1,   3}, {  5,   2},    // Row 9
  {  1,   7}, {  3,   0}, {  1,  18}, {  1,  10}, {  1,  19}, {  1,  20}, {  3,   2}, {  1,   1},
  {  1,   2}, {  1,  10}, {  1,  18}, {  1,   0}, {  1,  21}, {  1,  10}, {  2,  12}, {  4,   0},
  {  5,   0}, {  1,  10}, {  3,   0}, {  2,  10}, {  1,  19}, {  1,  20}, {  5,   2}, {  1,   7},    // Row 10
  {  4,   0}, {  1,  10}, {  2,  19}, {  4,   2}, {  1,  23}, {  1,  10}, {  2,   0}, {  1,  10},
  {  1,  24}, {  1,  12}, {  5,   0},
  {  5,   0}, {  1,  10}, {  3,   0}, {  1,  18}, {  1,  10}, {  1,  14}, {  1,  19}, {  5,   2},    // Row 11
  {  1,   7}, {  2,   0}, {  1,  25}, {  1,   0}, {  1,  21}, {  1,  10}, {  1,  19}, {  1,  26},
  {  3,   2}, {  1,  10}, {  1,  21}, {  1,   0}, {  1,  18}, {  1,  10}, {  2,  12}, {  5,   0},
  {  5,   0}, {  1,  10}, {  4,   0}, {  2,  10}, {  1,  19}, {  1,  20}, {  4,   2}, {  1,   7},    // Row 12
  {  2,   0}, {  1,  13}, {  2,   0}, {  1,  10}, {  2,  19}, {  2,   2}, {  1,   6}, {  1,  10},
  {  2,   0}, {  1,  10}, {  1,  24}, {  1,  12}, {  6,   0},
  {  5,   0}, {  1,  10}, {  2,   0}, {  1,  10}, {  2,   0}, {  1,  10}, {  1,  14}, {  1,  19},    // Row 13
  {  4,   2}, {  1,   7}, {  2,   0}, {  1,  13}, {  1,  18}, {  1,   0}, {  1,  21}, {  1,  10},
  {  1,  19}, {  1,  26}, {  1,   2}, {  1,  10}, {  2,   0}, {  1,  18}, {  1,  10}, {  1,  19},
  {  1,  12}, {  6,   0},
  {  5,   0}, {  1,  10}, {  2,   0}, {  1,  10}, {  2,   0}, {  2,  10}, {  1,  19}, {  1,  20},    // Row 14
  {  3,   2}, {  1,   7}, {  2,   0}, {  1,  13}, {  1,  10}, {  2,   0}, {  1,  10}, {  1,  27},
  {  1,  19}, {  1,   6}, {  1,  10}, {  2,   0}, {  1,  10}, {  1,  14}, {  1,  19}, {  1,   1},
  {  6,   0},
  {  4,   0}, {  1,  28}, {  1,  10}, {  2,   0}, {  2,  10}, {  2,   0}, {  1,  10}, {  1,  14},    // Row 15
  {  1,  19}, {  3,   2}, {  1,   7}, {  2,   0}, {  1,  13}, {  1,  10}, {  1,  18}, {  1,   0},
  {  1,  21}, {  1,  10}, {  1,  19}, {  1,  10}, {  2,   0}, {  1,  18}, {  1,  10}, {  1,  19},
  {  1,  29}, {  1,   1}, {  1,   2}, {  5,   0},
  {  4,   0}, {  1,  30}, {  1,  10}, {  2,   0}, {  2,  10}, {  2,   0}, {  2,  10}, {  1,  19},    // Row 16
  {  1,   1}, {  2,   2}, {  1,   7}, {  2,   0}, {  1,  13}, {  1,  31}, {  1,  10}, {  2,   0},
  {  1,  10}, {  1,  32}, {  1,  10}, {  2,   0}, {  1,  10}, {  1,  27}, {  1,  19}, {  1,   1},
  {  1,   2}, {  1,   9}, {  1,   2}, {  4,   0},
  {  3,   0}, {  1,  33}, {  1,  34}, {  1,  10}, {  2,   0}, {  3,  10}, {  2,   0}, {  1,  10},    // Row 17
  {  2,  19}, {  2,   2}, {  1,   7}, {  2,   0}, {  1,  13}, {  1,  35}, {  1,  10}, {  2,   0},
  {  1,  18}, {  1,  10}, {  1,  18}, {  1,   0}, {  1,  21}, {  1,  10}, {  2,  19}, {  3,   2},
  {  1,   1}, {  1,   2}, {  3,   0},
  {  2,   0}, {  1,  33}, {  1,   2}, {  1,  34}, {  1,  10}, {  2,   0}, {  1,  10}, {  1,  19},    // Row 18
  {  1,  10}, {  2,   0}, {  2,  10}, {  1,  19}, {  1,  26}, {  1,   2}, {  1,   7}, {  2,   0},
  {  1,  13}, {  1,  35}, {  1,  14}, {  1,  10}, {  2,   0}, {  1,  10}, {  2,   0}, {  1,  10},
  {  1,  27}, {  1,  19}, {  1,   1}, {  4,   2}, {  1,   1}, {  1,   2}, {  2,   0},
  {  1,   0}, {  1,  33}, {  2,   2}, {  1,  34}, {  1,  10}, {  2,   0}, {  1,  10}, {  1,  19},    // Row 19
  {  1,  27}, {  1,  10}, {  2,   0}, {  1,  10}, {  2,  19}, {  1,   2}, {  1,   7}, {  2,   0},
  {  1,  13}, {  1,  35}, {  1,  19}, {  1,  10}, {  4,   0}, {  1,  21}, {  1,  10}, {  2,  19},
  {  6,   2}, {  1,   1}, {  1,   2}, {  1,   0},
  {  1,  33}, {  3,   2}, {  1,  34}, {  1,  10}, {  2,   0}, {  1,  10}, {  2,  19}, {  1,  10},    // Row 20
  {  2,   0}, {  2,  10}, {  1,  19}, {  1,  26}, {  1,   7}, {  2,   0}, {  1,  13}, {  1,  35},
  {  1,  19}, {  1,   8}, {  1,  10}, {  3,   0}, {  1,  10}, {  2,  19}, {  1,   1}, {  7,   2},
  {  1,   1}, {  1,   2},
  {  1,   2}, {  1,   1}, {  2,   2}, {  1,  34}, {  1,  10}, {  2,   0}, {  1,  10}, {  2,  19},    // Row 21
  {  1,  36}, {  1,  10}, {  1,   0}, {  1,  18}, {  1,  10}, {  1,  14}, {  1,  19}, {  1,   7},
  {  2,   0}, {  1,  13}, {  1,  35}, {  1,  19}, {  1,   8}, {  1,  10}, {  2,   0}, {  1,  37},
  {  1,  10}, {  2,  19}, {  9,   2}, {  1,  38},
  {  1,   0}, {  1,   2}, {  1,   1}, {  1,   2}, {  1,  34}, {  1,  10}, {  2,   0}, {  1,  10},    // Row 22
  {  2,  19}, {  1,   2}, {  1,  10}, {  2,   0}, {  2,  10}, {  1,  19}, {  1,  28}, {  2,   0},
  {  1,  13}, {  1,  35}, {  1,  19}, {  1,  10}, {  1,  18}, {  3,   0}, {  1,  10}, {  1,  14},
  {  1,  19}, {  1,   1}, {  7,   2}, {  1,  39}, {  1,   0},
  {  2,   0}, {  1,   2}, {  1,   1}, {  1,  34}, {  1,  10}, {  2,   0}, {  1,  10}, {  2,  19},    // Row 23
  {  1,   2}, {  1,  36}, {  1,  10}, {  1,   0}, {  1,  18}, {  1,  10}, {  1,  14}, {  1,  40},
  {  2,   0}, {  1,  13}, {  1,  35}, {  1,  14}, {  1,  10}, {  2,   0}, {  1,  41}, {  1,   0},
  {  1,  18}, {  1,  10}, {  2,  19}, {  6,   2}, {  1,  39}, {  2,   0},
  {  3,   0}, {  1,   2}, {  1,  34}, {  1,  10}, {  2,   0}, {  1,  10}, {  2,  19}, {  2,   2},    // Row 24
  {  1,  10}, {  2,   0}, {  2,  10}, {  1,  40}, {  2,   0}, {  1,  13}, {  1,  35}, {  1,  10},
  {  1,  18}, {  2,   0}, {  1,  10}, {  2,   0}, {  1,  10}, {  1,  42}, {  1,  19}, {  1,   1},
  {  4,   2}, {  1,  39}, {  3,   0},
  {  4,   0}, {  1,  34}, {  1,  10}, {  2,   0}, {  1,  10}, {  2,  19}, {  2,   2}, {  1,  36},    // Row 25
  {  1,  10}, {  2,   0}, {  2,  10}, {  2,   0}, {  1,  13}, {  1,  31}, {  1,  10}, {  2,   0},
  {  1,  10}, {  1,  43}, {  1,  21}, {  1,   0}, {  1,  18}, {  1,  10}, {  2,  19}, {  2,   2},
  {  1,   1}, {  1,  39}, {  4,   0},
  {  4,   0}, {  1,  44}, {  1,  10}, {  2,   0}, {  1,  10}, {  2,  19}, {  3,   2}, {  1,  10},    // Row 26
  {  2,   0}, {  2,  10}, {  2,   0}, {  1,  13}, {  1,  10}, {  1,  21}, {  2,   0}, {  1,  10},
  {  1,  19}, {  1,  10}, {  2,   0}, {  1,  10}, {  1,  27}, {  1,  19}, {  2,   2}, {  1,  39},
  {  5,   0},
  {  5,   0}, {  1,  10}, {  2,   0}, {  1,  10}, {  2,  19}, {  3,   2}, {  1,  10}, {  1,  21},    // Row 27
  {  2,   0}, {  1,  10}, {  2,   0}, {  1,  13}, {  1,  10}, {  2,   0}, {  2,  10}, {  1,  19},
  {  1,  42}, {  1,  18}, {  2,   0}, {  1,  10}, {  1,  19}, {  1,  29}, {  1,  39}, {  6,   0},
  {  5,   0}, {  1,  10}, {  2,   0}, {  1,  10}, {  2,  19}, {  4,   2}, {  1,  10}, {  2,   0},    // Row 28
  {  1,  10}, {  2,   0}, {  1,  13}, {  1,  21}, {  2,   0}, {  1,  10}, {  2,  19}, {  1,   2},
  {  1,  10}, {  2,   0}, {  1,  10}, {  1,  27}, {  1,  19}, {  7,   0},
  {  5,   0}, {  1,  10}, {  2,   0}, {  1,  10}, {  2,  19}, {  4,   2}, {  2,  10}, {  4,   0},    // Row 29
  {  1,  13}, {  2,   0}, {  1,  10}, {  1,  27}, {  1,  19}, {  1,  29}, {  1,   2}, {  1,  36},
  {  1,  18}, {  2,   0}, {  1,  10}, {  2,  12}, {  6,   0},
  {  5,   0}, {  1,  10}, {  2,   0}, {  1,  10}, {  2,  19}, {  5,   2}, {  1,  10}, {  4,   0},    // Row 30
  {  1,  41}, {  2,   0}, {  1,  10}, {  2,  19}, {  3,   2}, {  1,  10}, {  2,   0}, {  1,  10},
  {  1,  45}, {  1,  12}, {  6,   0},
  {  5,   0}, {  1,  10}, {  2,   0}, {  1,  10}, {  1,  46}, {  1,  19}, {  1,   1}, {  4,   2},    // Row 31
  {  1,  36}, {  1,  10}, {  5,   0}, {  1,  10}, {  1,  42}, {  2,  19}, {  3,   2}, {  1,  10},
  {  1,  21}, {  1,   0}, {  1,  18}, {  1,  10}, {  2,  12}, {  5,   0},
  {  5,   0}, {  1,  10}, {  2,   0}, {  1,  10}, {  2,  12}, {  1,   2}, {  1,   1}, {  4,   2},    // Row 32
  {  1,  10}, {  5,   0}, {  1,  10}, {  2,  19}, {  1,   1}, {  2,   2}, {  1,   9}, {  1,   2},
  {  1,  10}, {  2,   0}, {  1,  10}, {  1,  24}, {  1,  12}, {  5,   0},
  {  5,   0}, {  1,  10}, {  2,   0}, {  1,  10}, {  2,  12}, {  1,   0}, {  1,   2}, {  1,   1},    // Row 33
  {  3,   2}, {  1,  36}, {  1,  10}, {  3,   0}, {  1,  10}, {  1,  42}, {  1,  19}, {  1,  26},
  {  2,   2}, {  1,   9}, {  1,   2}, {  1,   0}, {  1,  18}, {  1,  21}, {  1,   0}, {  1,  18},
  {  1,  10}, {  2,  12}, {  4,   0},
  {  5,   0}, {  1,  10}, {  2,   0}, {  1,  10}, {  2,  12}, {  2,   0}, {  1,   2}, {  1,   1},    // Row 34
  {  3,   2}, {  1,  10}, {  3,   0}, {  1,  10}, {  2,  19}, {  2,   2}, {  1,   9}, {  1,   2},
  {  3,   0}, {  1,  10}, {  2,   0}, {  1,  10}, {  2,  12}, {  4,   0},
  {  5,   0}, {  1,  10}, {  2,   0}, {  1,  10}, {  2,  12}, {  3,   0}, {  1,   2}, {  1,   1},    // Row 35
  {  2,   2}, {  1,  36}, {  1,  10}, {  1,   0}, {  1,  10}, {  1,  42}, {  1,  19}, {  1,  26},
  {  1,   2}, {  1,   9}, {  1,   2}, {  4,   0}, {  1,  18}, {  1,  13}, {  1,  18}, {  1,  10},
  {  3,  12}, {  3,   0},
  {  5,   0}, {  1,  10}, {  2,   0}, {  1,  47}, {  2,  12}, {  4,   0}, {  1,   2}, {  1,   9},    // Row 36
  {  2,   2}, {  3,  10}, {  2,  19}, {  1,   2}, {  1,  48}, {  1,   2}, {  6,   0}, {  3,  10},
  {  2,  12}, {  4,   0},
  {  7,   0}, {  4,  12}, {  4,   0}, {  1,   5}, {  1,   2}, {  1,   1}, {  2,   2}, {  1,  49},    // Row 37
  {  2,  19}, {  1,  50}, {  1,   9}, {  1,   2}, {  8,   0}, {  4,  12}, {  4,   0},
  {  7,   0}, {  4,  12}, {  6,   0}, {  1,   2}, {  1,   1}, {  4,   2}, {  1,   9}, {  1,   2},    // Row 38
  { 17,   0},
  { 18,   0}, {  1,   2}, {  1,   1}, {  2,   2}, {  1,   9}, {  1,   2}, { 18,   0},                // Row 39
  { 19,   0}, {  1,   2}, {  1,   1}, {  1,   9}, {  1,   2}, { 19,   0},                            // Row 40
  { 20,   0}, {  1,  51}, {  1,   2}, { 20,   0},                                                    // Row 41
 };

/********************************************************************************************
 * Public Bitmap Structure Definitions
 ********************************************************************************************/

const struct SRlePaletteBitmap NXWidgets::g_nxiconBitmap =
{
  CONFIG_NXWIDGETS_BPP,  // bpp    - Bits per pixel
  CONFIG_NXWIDGETS_FMT,  // fmt    - Color format
  BITMAP_NLUTCODES,      // nlut   - Number of colors in the lLook-Up Table (LUT)
  BITMAP_NCOLUMNS,       // width  - Width in pixels
  BITMAP_NROWS,          // height - Height in rows
  {                      // lut    - Pointer to the beginning of the Look-Up Table (LUT)
    g_nxiconNormalLut,   //          Index 0: Unselected LUT
    g_nxiconBrightLut,   //          Index 1: Selected LUT
  },
  g_nxiconRleEntries     // data   - Pointer to the beginning of the RLE data
};
