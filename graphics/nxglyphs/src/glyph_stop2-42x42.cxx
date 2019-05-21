/********************************************************************************************
 * apps/graphics/nxglyphs/src/glyph_xxxxxx.cxx
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
#define BITMAP_NLUTCODES 3

/********************************************************************************************
 * Private Bitmap Data
 ********************************************************************************************/

using namespace NXWidgets;

#if CONFIG_NXWIDGETS_BPP == 24 ||  CONFIG_NXWIDGETS_BPP == 32
// RGB24 (8-8-8) Colors

static const uint32_t g_stop2NormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR, 0xfcfcfc, 0xd8fcfc,  /* Codes 0-2 */
};

static const uint32_t g_stop2BrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR, 0xfcfcfc, 0xe1fcfc,  /* Codes 0-2 */
};

#elif CONFIG_NXWIDGETS_BPP == 16
// RGB16 (565) Colors (four of the colors in this map are duplicates)

static const uint16_t g_stop2NormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR, 0xffff, 0xdfff,  /* Codes 0-2 */
};

static const uint16_t g_stop2BrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR, 0xffff, 0xe7ff,  /* Codes 0-2 */
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

static const uint8_t g_stop2NormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR, 0xfc, 0xf1,  /* Codes 0-2 */
}

static const uint8_t g_stop2BrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR, 0xfc, 0xf3,  /* Codes 0-2 */
};

#  else /* CONFIG_NXWIDGETS_GREYSCALE */
// RGB8 (332) Colors

static const nxgl_mxpixel_t g_stop2NormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR, 0xff, 0xdf,  /* Codes 0-2 */
};

static const uint8_t g_stop2BrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR, 0xff, 0xff,  /* Codes 0-2 */
};

#  endif
#else
#  error Unsupported pixel format
#endif

static const struct SRlePaletteBitmapEntry g_stop2RleEntries[] =
{
  { 42,   0},                                                                                        // Row 0
  { 42,   0},                                                                                        // Row 1
  { 42,   0},                                                                                        // Row 2
  { 42,   0},                                                                                        // Row 3
  { 42,   0},                                                                                        // Row 4
  { 42,   0},                                                                                        // Row 5
  {  5,   0}, {  5,   1}, {  2,   2}, { 18,   0}, {  6,   1}, {  1,   2}, {  5,   0},                // Row 6
  {  5,   0}, {  6,   1}, {  2,   2}, { 16,   0}, {  7,   1}, {  1,   2}, {  5,   0},                // Row 7
  {  5,   0}, {  7,   1}, {  2,   2}, { 14,   0}, {  7,   1}, {  2,   2}, {  5,   0},                // Row 8
  {  6,   0}, {  8,   1}, {  1,   2}, { 12,   0}, {  7,   1}, {  2,   2}, {  6,   0},                // Row 9
  {  7,   0}, {  7,   1}, {  2,   2}, { 10,   0}, {  7,   1}, {  2,   2}, {  7,   0},                // Row 10
  {  8,   0}, {  7,   1}, {  2,   2}, {  8,   0}, {  7,   1}, {  2,   2}, {  8,   0},                // Row 11
  {  9,   0}, {  7,   1}, {  2,   2}, {  6,   0}, {  7,   1}, {  2,   2}, {  9,   0},                // Row 12
  { 10,   0}, {  7,   1}, {  2,   2}, {  4,   0}, {  7,   1}, {  2,   2}, { 10,   0},                // Row 13
  { 11,   0}, {  7,   1}, {  2,   2}, {  2,   0}, {  7,   1}, {  2,   2}, { 11,   0},                // Row 14
  { 12,   0}, {  7,   1}, {  2,   2}, {  7,   1}, {  2,   2}, { 12,   0},                            // Row 15
  { 13,   0}, { 14,   1}, {  2,   2}, { 13,   0},                                                    // Row 16
  { 14,   0}, { 12,   1}, {  2,   2}, { 14,   0},                                                    // Row 17
  { 15,   0}, { 10,   1}, {  2,   2}, { 15,   0},                                                    // Row 18
  { 16,   0}, {  8,   1}, {  2,   2}, { 16,   0},                                                    // Row 19
  { 17,   0}, {  6,   1}, {  2,   2}, { 17,   0},                                                    // Row 20
  { 17,   0}, {  6,   1}, {  2,   2}, { 17,   0},                                                    // Row 21
  { 16,   0}, {  8,   1}, {  2,   2}, { 16,   0},                                                    // Row 22
  { 15,   0}, { 10,   1}, {  2,   2}, { 15,   0},                                                    // Row 23
  { 14,   0}, { 12,   1}, {  2,   2}, { 14,   0},                                                    // Row 24
  { 13,   0}, { 14,   1}, {  2,   2}, { 13,   0},                                                    // Row 25
  { 12,   0}, {  7,   1}, {  2,   2}, {  7,   1}, {  2,   2}, { 12,   0},                            // Row 26
  { 11,   0}, {  7,   1}, {  2,   2}, {  2,   0}, {  7,   1}, {  2,   2}, { 11,   0},                // Row 27
  { 10,   0}, {  7,   1}, {  2,   2}, {  4,   0}, {  7,   1}, {  2,   2}, { 10,   0},                // Row 28
  {  9,   0}, {  7,   1}, {  2,   2}, {  6,   0}, {  7,   1}, {  2,   2}, {  9,   0},                // Row 29
  {  8,   0}, {  7,   1}, {  2,   2}, {  8,   0}, {  7,   1}, {  2,   2}, {  8,   0},                // Row 30
  {  7,   0}, {  7,   1}, {  2,   2}, { 10,   0}, {  7,   1}, {  2,   2}, {  7,   0},                // Row 31
  {  6,   0}, {  7,   1}, {  2,   2}, { 12,   0}, {  7,   1}, {  2,   2}, {  6,   0},                // Row 32
  {  5,   0}, {  7,   1}, {  2,   2}, { 14,   0}, {  7,   1}, {  2,   2}, {  5,   0},                // Row 33
  {  5,   0}, {  6,   1}, {  2,   2}, { 16,   0}, {  7,   1}, {  1,   2}, {  5,   0},                // Row 34
  {  5,   0}, {  7,   2}, { 18,   0}, {  7,   2}, {  5,   0},                                        // Row 35
  { 42,   0},                                                                                        // Row 36
  { 42,   0},                                                                                        // Row 37
  { 42,   0},                                                                                        // Row 38
  { 42,   0},                                                                                        // Row 39
  { 42,   0},                                                                                        // Row 40
  { 42,   0},                                                                                        // Row 41
 };

/********************************************************************************************
 * Public Bitmap Structure Definitions
 ********************************************************************************************/

const struct SRlePaletteBitmap NXWidgets::g_stop2Bitmap =
{
  CONFIG_NXWIDGETS_BPP,  // bpp    - Bits per pixel
  CONFIG_NXWIDGETS_FMT,  // fmt    - Color format
  BITMAP_NLUTCODES,      // nlut   - Number of colors in the lLook-Up Table (LUT)
  BITMAP_NCOLUMNS,       // width  - Width in pixels
  BITMAP_NROWS,          // height - Height in rows
  {                      // lut    - Pointer to the beginning of the Look-Up Table (LUT)
    g_stop2NormalLut,    //          Index 0: Unselected LUT
    g_stop2BrightLut,    //          Index 1: Selected LUT
  },
  g_stop2RleEntries      // data   - Pointer to the beginning of the RLE data
};
