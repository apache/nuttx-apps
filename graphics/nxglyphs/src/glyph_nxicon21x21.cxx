/********************************************************************************************
 * apps/graphics/nxglyphs/src/glyph_nxicon21x21.cxx
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

#define BITMAP_NROWS     21
#define BITMAP_NCOLUMNS  21
#define BITMAP_NLUTCODES 31

/********************************************************************************************
 * Private Bitmap Data
 ********************************************************************************************/

using namespace NXWidgets;

#if CONFIG_NXWIDGETS_BPP == 24 ||  CONFIG_NXWIDGETS_BPP == 32
// RGB24 (8-8-8) Colors

static const uint32_t g_nxiconNormalLut[BITMAP_NLUTCODES] =
{
  0xfcfcfc, 0x3830e0, 0x3c34e0, 0xcce0f0, 0x2c20e0, 0xc8e0f0, 0x181074, 0xd4e8f4,  /* Codes 0-7 */
  0x5894bc, 0xd4ecf0, 0x282480, 0x3c34ec, 0x848cf0, 0x4c8cb4, 0x7c84b8, 0x8488e8,  /* Codes 8-15 */
  0x3028e4, 0x100870, 0x3830d0, 0x3c34e8, 0x3028e0, 0x302cb0, 0x20187c, 0xd8ecf8,  /* Codes 16-23 */
  0x58a0e4, 0xd8ecf4, 0xe4f0f8, 0xd0d0f8, 0xc8e0ec, 0x94bcd4, 0xd4e4f4,  /* Codes 24-30 */
};

static const uint32_t g_nxiconBrightLut[BITMAP_NLUTCODES] =
{
  0xfcfcfc, 0x6963e7, 0x6c66e7, 0xd8e7f3, 0x6057e7, 0xd5e7f3, 0x514b96, 0xdeedf6,  /* Codes 0-7 */
  0x81aecc, 0xdef0f3, 0x5d5a9f, 0x6c66f0, 0xa2a8f3, 0x78a8c6, 0x9ca2c9, 0xa2a5ed,  /* Codes 8-15 */
  0x635dea, 0x4b4593, 0x6963db, 0x6c66ed, 0x635de7, 0x6360c3, 0x57519c, 0xe1f0f9,  /* Codes 16-23 */
  0x81b7ea, 0xe1f0f6, 0xeaf3f9, 0xdbdbf9, 0xd5e7f0, 0xaeccde, 0xdeeaf6,  /* Codes 24-30 */
};

#elif CONFIG_NXWIDGETS_BPP == 16
// RGB16 (565) Colors (four of the colors in this map are duplicates)

static const uint16_t g_nxiconNormalLut[BITMAP_NLUTCODES] =
{
  0xffff, 0x399c, 0x39bc, 0xcf1e, 0x291c, 0xcf1e, 0x188e, 0xd75e, 0x5cb7, 0xd77e,  /* Codes 0-9 */
  0x2930, 0x39bd, 0x847e, 0x4c76, 0x7c37, 0x845d, 0x315c, 0x104e, 0x399a, 0x39bd,  /* Codes 10-19 */
  0x315c, 0x3176, 0x20cf, 0xdf7f, 0x5d1c, 0xdf7e, 0xe79f, 0xd69f, 0xcf1d, 0x95fa,  /* Codes 20-29 */
  0xd73e,  /* Codes 30-30 */
};

static const uint16_t g_nxiconBrightLut[BITMAP_NLUTCODES] =
{
  0xffff, 0x6b1c, 0x6b3c, 0xdf3e, 0x62bc, 0xd73e, 0x5252, 0xdf7e, 0x8579, 0xdf9e,  /* Codes 0-9 */
  0x5ad3, 0x6b3e, 0xa55e, 0x7d58, 0x9d19, 0xa53d, 0x62fd, 0x4a32, 0x6b1b, 0x6b3d,  /* Codes 10-19 */
  0x62fc, 0x6318, 0x5293, 0xe79f, 0x85bd, 0xe79e, 0xef9f, 0xdedf, 0xd73e, 0xae7b,  /* Codes 20-29 */
  0xdf5e,  /* Codes 30-30 */
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
  0xfc, 0x46, 0x4a, 0xdb, 0x39, 0xda, 0x1d, 0xe3, 0x86, 0xe5, 0x2f, 0x4b, 0x95, 0x7d, 0x87, 0x91,  /* Codes 0-15 */
  0x3f, 0x16, 0x44, 0x4a, 0x3f, 0x3c, 0x25, 0xe7, 0x92, 0xe6, 0xed, 0xd4, 0xda, 0xb2, 0xe1,  /* Codes 16-30 */
}

static const uint8_t g_nxiconBrightLut[BITMAP_NLUTCODES] =
{
  0xfc, 0x73, 0x76, 0xe3, 0x6a, 0xe2, 0x55, 0xe9, 0xa3, 0xea, 0x62, 0x77, 0xae, 0x9d, 0xa4, 0xac,  /* Codes 0-15 */
  0x6e, 0x4f, 0x72, 0x77, 0x6e, 0x6c, 0x5b, 0xec, 0xac, 0xec, 0xf0, 0xde, 0xe2, 0xc5, 0xe7,  /* Codes 16-30 */
};

#  else /* CONFIG_NXWIDGETS_GREYSCALE */
// RGB8 (332) Colors

static const nxgl_mxpixel_t g_nxiconNormalLut[BITMAP_NLUTCODES] =
{
  0xff, 0x27, 0x27, 0xdf, 0x27, 0xdf, 0x01, 0xdf, 0x53, 0xdf, 0x26, 0x27, 0x93, 0x52, 0x72, 0x93,  /* Codes 0-15 */
  0x27, 0x01, 0x27, 0x27, 0x27, 0x26, 0x22, 0xdf, 0x57, 0xdf, 0xff, 0xdb, 0xdf, 0x97, 0xdf,  /* Codes 16-30 */
};

static const uint8_t g_nxiconBrightLut[BITMAP_NLUTCODES] =
{
  0xff, 0x6f, 0x6f, 0xdf, 0x6b, 0xdf, 0x4a, 0xff, 0x97, 0xff, 0x4a, 0x6f, 0xb7, 0x77, 0x97, 0xb7,  /* Codes 0-15 */
  0x6b, 0x4a, 0x6f, 0x6f, 0x6b, 0x6f, 0x4a, 0xff, 0x97, 0xff, 0xff, 0xdb, 0xdf, 0xbb, 0xff,  /* Codes 16-30 */
};

#  endif
#else
#  error Unsupported pixel format
#endif

static const struct SRlePaletteBitmapEntry g_nxiconRleEntries[] =
{
  { 10,   0}, {  1,   1}, { 10,   0},                                                                // Row 0
  {  9,   0}, {  1,   1}, {  1,   2}, {  1,   1}, {  9,   0},                                        // Row 1
  {  8,   0}, {  1,   1}, {  3,   2}, {  1,   1}, {  8,   0},                                        // Row 2
  {  3,   0}, {  2,   3}, {  2,   0}, {  1,   1}, {  1,   2}, {  1,   4}, {  1,   5}, {  1,   6},    // Row 3
  {  1,   2}, {  1,   1}, {  4,   0}, {  1,   7}, {  2,   0},
  {  4,   0}, {  1,   3}, {  1,   8}, {  1,   1}, {  2,   2}, {  1,   9}, {  1,   0}, {  1,   3},    // Row 4
  {  1,  10}, {  1,   2}, {  1,   1}, {  1,   0}, {  1,   3}, {  1,   0}, {  1,   8}, {  2,   0},
  {  5,   0}, {  1,   3}, {  1,  11}, {  2,   2}, {  1,   9}, {  2,   0}, {  1,  10}, {  2,   2},    // Row 5
  {  1,  12}, {  1,   0}, {  1,   3}, {  1,   8}, {  2,   0},
  {  5,   0}, {  1,   3}, {  1,  10}, {  2,   2}, {  1,   9}, {  2,   0}, {  1,   3}, {  1,  10},    // Row 6
  {  1,   2}, {  1,   3}, {  1,   0}, {  1,  13}, {  3,   0},
  {  4,   0}, {  1,   3}, {  1,   0}, {  1,   3}, {  1,  11}, {  1,   2}, {  1,   9}, {  1,   0},    // Row 7
  {  1,   3}, {  1,   0}, {  1,  14}, {  1,  15}, {  1,   0}, {  1,   3}, {  1,  10}, {  3,   0},
  {  2,   0}, {  1,  16}, {  1,   0}, {  1,   3}, {  1,   0}, {  1,   3}, {  1,  10}, {  1,   2},    // Row 8
  {  1,   9}, {  1,   0}, {  1,  17}, {  1,   0}, {  2,   3}, {  1,   0}, {  1,  14}, {  1,   1},
  {  1,  18}, {  2,   0},
  {  1,   0}, {  1,  19}, {  1,  20}, {  1,   0}, {  2,   3}, {  1,   0}, {  1,   3}, {  1,  21},    // Row 9
  {  1,   9}, {  1,   0}, {  1,  22}, {  1,   3}, {  2,   0}, {  1,   3}, {  1,  10}, {  2,   2},
  {  1,   1}, {  1,   0},
  {  1,  19}, {  1,   2}, {  1,  20}, {  1,   0}, {  1,   3}, {  1,  10}, {  1,   0}, {  1,   3},    // Row 10
  {  1,  10}, {  1,   9}, {  1,   0}, {  1,  22}, {  1,   4}, {  2,   0}, {  1,  10}, {  1,   1},
  {  3,   2}, {  1,   1},
  {  1,   0}, {  1,   1}, {  1,  20}, {  1,   0}, {  1,   3}, {  1,  10}, {  1,   3}, {  1,   0},    // Row 11
  {  1,   3}, {  1,  23}, {  1,   0}, {  1,  22}, {  1,   3}, {  2,   0}, {  1,   6}, {  1,   1},
  {  3,   2}, {  1,  24},
  {  2,   0}, {  1,  20}, {  1,   0}, {  1,   3}, {  1,  10}, {  1,   2}, {  1,   0}, {  1,   3},    // Row 12
  {  1,  25}, {  1,   0}, {  1,  22}, {  1,  26}, {  2,   0}, {  1,   3}, {  1,  10}, {  2,   2},
  {  1,  24}, {  1,   0},
  {  2,   0}, {  1,  27}, {  1,   0}, {  1,   3}, {  1,  10}, {  1,   2}, {  1,   3}, {  1,   0},    // Row 13
  {  1,   3}, {  1,   0}, {  1,   3}, {  1,   0}, {  2,   3}, {  1,   0}, {  1,  14}, {  1,   2},
  {  1,  24}, {  2,   0},
  {  4,   0}, {  1,   3}, {  1,  10}, {  2,   2}, {  1,   0}, {  1,   3}, {  1,   0}, {  1,  28},    // Row 14
  {  1,   0}, {  1,  10}, {  1,   2}, {  1,   0}, {  1,   3}, {  1,  10}, {  3,   0},
  {  4,   0}, {  1,   3}, {  1,  10}, {  2,   2}, {  1,   3}, {  3,   0}, {  1,   3}, {  1,  10},    // Row 15
  {  1,   2}, {  1,   3}, {  1,   0}, {  1,  29}, {  3,   0},
  {  4,   0}, {  1,   3}, {  1,   8}, {  1,   1}, {  2,   2}, {  3,   0}, {  1,  10}, {  1,   1},    // Row 16
  {  1,   2}, {  1,  24}, {  1,   0}, {  1,   3}, {  1,   8}, {  2,   0},
  {  4,   0}, {  1,   3}, {  1,   8}, {  1,   0}, {  1,   1}, {  1,   2}, {  1,   3}, {  1,   0},    // Row 17
  {  1,   3}, {  1,  10}, {  1,   2}, {  1,  24}, {  1,   0}, {  1,   3}, {  1,   0}, {  1,   8},
  {  2,   0},
  {  4,   0}, {  1,  30}, {  1,   8}, {  2,   0}, {  1,  18}, {  1,   2}, {  1,   3}, {  1,  10},    // Row 18
  {  1,   2}, {  1,  24}, {  3,   0}, {  1,   3}, {  1,   8}, {  2,   0},
  {  4,   0}, {  2,   8}, {  3,   0}, {  1,   1}, {  2,   2}, {  1,  24}, {  8,   0},                // Row 19
  { 10,   0}, {  1,   1}, {  1,  24}, {  9,   0},                                                    // Row 20
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
