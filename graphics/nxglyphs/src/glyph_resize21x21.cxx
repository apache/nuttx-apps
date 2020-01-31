/********************************************************************************************
 * apps/graphics/nxglyphs/src/glyph_resize21x21.cxx
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
#define BITMAP_NLUTCODES 6

/********************************************************************************************
 * Private Bitmap Data
 ********************************************************************************************/

using namespace NXWidgets;

#if CONFIG_NXWIDGETS_BPP == 24 ||  CONFIG_NXWIDGETS_BPP == 32
// RGB24 (8-8-8) Colors

static const uint32_t g_resizeNormalLut[BITMAP_NLUTCODES] =
{
  0x90f0cc, 0x00b400, 0x00d800, 0xfcfcfc, 0x00a400, 0x046414,  /* Codes 0-5 */
};

static const uint32_t g_resizeBrightLut[BITMAP_NLUTCODES] =
{
  0xabf3d8, 0x3fc63f, 0x3fe13f, 0xfcfcfc, 0x3fba3f, 0x428a4e,  /* Codes 0-5 */
};

#elif CONFIG_NXWIDGETS_BPP == 16
// RGB16 (565) Colors (four of the colors in this map are duplicates)

static const uint16_t g_resizeNormalLut[BITMAP_NLUTCODES] =
{
  0x9799, 0x05a0, 0x06c0, 0xffff, 0x0520, 0x0322,  /* Codes 0-5 */
};

static const uint16_t g_resizeBrightLut[BITMAP_NLUTCODES] =
{
  0xaf9b, 0x3e27, 0x3f07, 0xffff, 0x3dc7, 0x4449,  /* Codes 0-5 */
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
  0xcf, 0x69, 0x7e, 0xfc, 0x60, 0x3e,  /* Codes 0-5 */
}

static const uint8_t g_resizeBrightLut[BITMAP_NLUTCODES] =
{
  0xda, 0x8e, 0x9e, 0xfc, 0x87, 0x6d,  /* Codes 0-5 */
};

#  else /* CONFIG_NXWIDGETS_GREYSCALE */
// RGB8 (332) Colors

static const nxgl_mxpixel_t g_resizeNormalLut[BITMAP_NLUTCODES] =
{
  0x9f, 0x14, 0x18, 0xff, 0x14, 0x0c,  /* Codes 0-5 */
};

static const uint8_t g_resizeBrightLut[BITMAP_NLUTCODES] =
{
  0xbf, 0x59, 0x5d, 0xff, 0x55, 0x51,  /* Codes 0-5 */
};

#  endif
#else
#  error Unsupported pixel format
#endif

static const struct SRlePaletteBitmapEntry g_resizeRleEntries[] =
{
  { 21,   0},                                                                                        // Row 0
  {  1,   0}, {  4,   1}, {  7,   2}, {  9,   1},                                                    // Row 1
  {  1,   0}, {  3,   1}, {  9,   2}, {  8,   1},                                                    // Row 2
  {  1,   0}, {  2,   1}, { 10,   2}, {  8,   1},                                                    // Row 3
  {  1,   0}, {  1,   1}, {  2,   2}, { 14,   3}, {  3,   1},                                        // Row 4
  {  1,   0}, {  1,   1}, {  2,   2}, {  1,   3}, {  6,   2}, {  6,   1}, {  1,   3}, {  2,   1},    // Row 5
  {  1,   4},
  {  1,   0}, {  3,   2}, {  1,   3}, {  5,   2}, {  7,   1}, {  1,   3}, {  2,   1}, {  1,   4},    // Row 6
  {  1,   0}, {  3,   2}, {  1,   3}, {  4,   2}, {  8,   1}, {  1,   3}, {  2,   1}, {  1,   4},    // Row 7
  {  1,   0}, {  3,   2}, {  1,   3}, {  3,   2}, {  9,   1}, {  1,   3}, {  1,   1}, {  2,   4},    // Row 8
  {  1,   0}, {  3,   2}, {  1,   3}, {  1,   2}, { 11,   1}, {  1,   3}, {  1,   1}, {  2,   4},    // Row 9
  {  1,   0}, {  1,   1}, {  2,   2}, {  1,   3}, { 12,   1}, {  1,   3}, {  3,   4},                // Row 10
  {  1,   0}, {  3,   1}, {  1,   3}, { 12,   1}, {  1,   3}, {  3,   4},                            // Row 11
  {  1,   0}, {  3,   1}, {  1,   3}, { 11,   1}, {  1,   4}, {  1,   3}, {  3,   4},                // Row 12
  {  1,   0}, {  3,   1}, {  1,   3}, { 10,   1}, {  2,   4}, {  1,   3}, {  3,   4},                // Row 13
  {  1,   0}, {  3,   1}, {  1,   3}, {  9,   1}, {  3,   4}, {  1,   3}, {  3,   4},                // Row 14
  {  1,   0}, {  3,   1}, {  1,   3}, {  8,   1}, {  4,   4}, {  1,   3}, {  3,   4},                // Row 15
  {  1,   0}, {  3,   1}, {  1,   3}, {  6,   1}, {  6,   4}, {  1,   3}, {  3,   4},                // Row 16
  {  1,   0}, {  3,   1}, { 14,   3}, {  3,   4},                                                    // Row 17
  {  1,   0}, {  4,   1}, { 16,   4},                                                                // Row 18
  {  1,   0}, {  2,   1}, { 18,   4},                                                                // Row 19
  {  1,   0}, { 19,   4}, {  1,   5},                                                                // Row 20
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
