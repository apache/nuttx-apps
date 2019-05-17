/********************************************************************************************
 * apps/graphics/nxglyphs/src/glyph_stop42x42.cxx
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
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
#define BITMAP_NCOLUMNS  42
#define BITMAP_NLUTCODES 8

#define DARK_STOP_ICON   1

/********************************************************************************************
 * Private Bitmap Data
 ********************************************************************************************/

using namespace NXWidgets;

/* RGB24 (8-8-8) Colors */

#if CONFIG_NXWIDGETS_BPP == 24 ||  CONFIG_NXWIDGETS_BPP == 32
#  ifdef DARK_STOP_ICON

static const uint32_t g_stopNormalLut[BITMAP_NLUTCODES] =
{
  0xa5a5a5, 0xa50000, 0xbd0000, 0x6f1b1b, 0xbdbdbd, 0xa5bdbd, 0x8a0000, 0xbda5a5 /* Codes 0-7 */
};

static const uint32_t g_stopBrightLut[BITMAP_NLUTCODES] =
{
  0xdcdcdc, 0xdc0000, 0xfc0000, 0x942424, 0xfcfcfc, 0xdcfcfc, 0xb80000, 0xfcdcdc /* Codes 0-7 */
};

#  else /* DARK_STOP_ICON */

static const uint32_t g_stopNormalLut[BITMAP_NLUTCODES] =
{
  0xdcdcdc, 0xdc0000, 0xfc0000, 0x942424, 0xfcfcfc, 0xdcfcfc, 0xb80000, 0xfcdcdc /* Codes 0-7 */
};

static const uint32_t g_stopBrightLut[BITMAP_NLUTCODES] =
{
  0xffffff, 0xff0000, 0xff0000, 0xb92d2d, 0xffffff, 0xffffff, 0xe60000, 0xffffff /* Codes 0-7 */
};
#  endif /* DARK_STOP_ICON */

/* RGB16 (565) Colors */

#elif CONFIG_NXWIDGETS_BPP == 16
#  ifdef DARK_STOP_ICON

static const uint16_t g_stopNormalLut[BITMAP_NLUTCODES] =
{
  0xa534, 0xa000, 0xb800, 0x68c3, 0xbdf7, 0xa5f7, 0x8800, 0xbd34 /* Codes 0-7 */
};

static const uint16_t g_stopBrightLut[BITMAP_NLUTCODES] =
{
  0xdefb, 0xd800, 0xf800, 0x9124, 0xffff, 0xdfff, 0xb800, 0xfefb /* Codes 0-7 */
};

#  else /* DARK_STOP_ICON */

static const uint16_t g_stopNormalLut[BITMAP_NLUTCODES] =
{
  0xdefb, 0xd800, 0xf800, 0x9124, 0xffff, 0xdfff, 0xb800, 0xfefb /* Codes 0-7 */
};

static const uint16_t g_stopBrightLut[BITMAP_NLUTCODES] =
{
  0xffff, 0xf800, 0xf800, 0xb965, 0xffff, 0xffff, 0xe000, 0xffff /* Codes 0-7 */
};

#  endif /* DARK_STOP_ICON */

/* 8-bit color lookups.  NOTE:  This is really dumb!  The lookup index is 8-bits and it used
 * to lookup an 8-bit value.  There is no savings in that!  It would be better to just put
 * the 8-bit color/greyscale value in the run-length encoded image and save the cost of these
 * pointless lookups.  But these pointless lookups do make the logic compatible with the
 * 16- and 24-bit types.
 */

#elif CONFIG_NXWIDGETS_BPP == 8
#  ifdef CONFIG_NXWIDGETS_GREYSCALE

/* 8-bit Greyscale */

#    ifdef DARK_STOP_ICON

static const uint8_t g_stopNormalLut[BITMAP_NLUTCODES] =
{
  0xa5, 0x31, 0x38, 0x34, 0xbd, 0xb5, 0x29, 0xac /* Codes 0-7 */
};

static const uint8_t g_stopBrightLut[BITMAP_NLUTCODES] =
{
  0xdc, 0x41, 0x4b, 0x45, 0xfc, 0xf2, 0x37, 0xe5 /* Codes 0-7 */
};

#    else /* DARK_STOP_ICON */

static const uint8_t g_stopNormalLut[BITMAP_NLUTCODES] =
{
  0xdc, 0x41, 0x4b, 0x45, 0xfc, 0xf2, 0x37, 0xe5 /* Codes 0-7 */
};

static const uint8_t g_stopBrightLut[BITMAP_NLUTCODES] =
{
  0xff, 0x4c, 0x4c, 0x56, 0xff, 0xff, 0x44, 0xff /* Codes 0-7 */
};

#    endif /* DARK_STOP_ICON */

#  else /* CONFIG_NXWIDGETS_GREYSCALE */

/* RGB8 (332) Colors */

#    ifdef DARK_STOP_ICON

static const nxgl_mxpixel_t g_stopNormalLut[BITMAP_NLUTCODES] =
{
  0xb6, 0xa0, 0xa0, 0x60, 0xb6, 0xb6, 0x80, 0xb6 /* Codes 0-7 */
};

static const nxgl_mxpixel_t g_stopBrightLut[BITMAP_NLUTCODES] =
{
  0xdb, 0xc0, 0xe0, 0x84, 0xff, 0xdf, 0xa0, 0xfb /* Codes 0-7 */
};

#    else /* DARK_STOP_ICON */

static const nxgl_mxpixel_t g_stopNormalLut[BITMAP_NLUTCODES] =
{
  0xdb, 0xc0, 0xe0, 0x84, 0xff, 0xdf, 0xa0, 0xfb /* Codes 0-7 */
};

static const nxgl_mxpixel_t g_stopBrightLut[BITMAP_NLUTCODES] =
{
  0xff, 0xe0, 0xe0, 0xa4, 0xff, 0xff, 0xe0, 0xff /* Codes 0-7 */
};

#    endif /* DARK_STOP_ICON */
#  endif /* CONFIG_NXWIDGETS_GREYSCALE */
#else
# error Unsupported pixel format
#endif

static const struct SRlePaletteBitmapEntry g_stopRleEntries[] =
{
  {42,  0}, /* Row 0 */
  { 1,  0}, {13,  1}, { 7,  2}, {20,  1}, { 1,  0},                               /* Row 1 */
  { 1,  0}, { 4,  1}, {18,  2}, {18,  1}, { 1,  3},                               /* Row 2 */
  { 1,  0}, { 3,  1}, {22,  2}, {15,  1}, { 1,  3},                               /* Row 3 */
  { 1,  0}, { 2,  1}, {24,  2}, {14,  1}, { 1,  3},                               /* Row 4 */
  { 1,  0}, { 1,  1}, {26,  2}, {13,  1}, { 1,  3},                               /* Row 5 */
  { 1,  0}, { 1,  1}, { 3,  2}, { 5,  4}, { 2,  5}, {16,  2}, { 2,  1}, { 6,  4}, /* Row 6 */
  { 1,  5}, { 4,  1}, { 1,  3},
  { 1,  0}, { 1,  1}, { 3,  2}, { 6,  4}, { 2,  5}, {15,  2}, { 1,  1}, { 7,  4}, /* Row 7 */
  { 1,  5}, { 4,  1}, { 1,  3},
  { 1,  0}, { 1,  1}, { 3,  2}, { 7,  4}, { 2,  5}, {13,  2}, { 1,  1}, { 7,  4}, /* Row 8 */
  { 2,  5}, { 4,  1}, { 1,  3},
  { 1,  0}, { 1,  1}, { 4,  2}, { 8,  4}, { 1,  5}, {11,  2}, { 1,  1}, { 7,  4}, /* Row 9 */
  { 2,  5}, { 5,  1}, { 1,  3},
  { 1,  0}, { 1,  1}, { 5,  2}, { 7,  4}, { 2,  5}, {10,  2}, { 7,  4}, { 2,  5}, /* Row 10 */
  { 5,  1}, { 1,  6}, { 1,  3},
  { 1,  0}, { 1,  1}, { 6,  2}, { 7,  4}, { 2,  5}, { 8,  2}, { 7,  4}, { 2,  5}, /* Row 11 */
  { 6,  1}, { 1,  6}, { 1,  3},
  { 1,  0}, { 8,  2}, { 7,  4}, { 2,  5}, { 6,  2}, { 7,  4}, { 2,  5}, { 7,  1}, /* Row 12 */
  { 1,  6}, { 1,  3},
  { 1,  0}, { 9,  2}, { 7,  4}, { 2,  5}, { 4,  2}, { 7,  4}, { 2,  5}, { 8,  1}, /* Row 13 */
  { 1,  6}, { 1,  3},
  { 1,  0}, {10,  2}, { 7,  4}, { 2,  5}, { 2,  2}, { 7,  4}, { 2,  5}, { 8,  1}, /* Row 14 */
  { 2,  6}, { 1,  3},
  { 1,  0}, {11,  2}, { 7,  4}, { 2,  5}, { 7,  4}, { 2,  5}, { 9,  1}, { 2,  6}, /* Row 15 */
  { 1,  3},
  { 1,  0}, {12,  2}, {14,  4}, { 2,  5}, {10,  1}, { 2,  6}, { 1,  3},           /* Row 16 */
  { 1,  0}, {13,  2}, {12,  4}, { 2,  5}, {10,  1}, { 3,  6}, { 1,  3},           /* Row 17 */
  { 1,  0}, {14,  2}, {10,  4}, { 2,  5}, {11,  1}, { 3,  6}, { 1,  3},           /* Row 18 */
  { 1,  0}, {15,  2}, { 8,  4}, { 2,  5}, {11,  1}, { 4,  6}, { 1,  3},           /* Row 19 */
  { 1,  0}, {15,  2}, { 1,  1}, { 6,  4}, { 2,  5}, {12,  1}, { 4,  6}, { 1,  3}, /* Row 20 */
  { 1,  0}, { 1,  1}, {13,  2}, { 2,  1}, { 6,  4}, { 2,  5}, {11,  1}, { 5,  6}, /* Row 21 */
  { 1,  3},
  { 1,  0}, { 2,  1}, {11,  2}, { 2,  1}, { 8,  4}, { 2,  5}, {10,  1}, { 5,  6}, /* Row 22 */
  { 1,  3},
  { 1,  0}, { 3,  1}, { 8,  2}, { 3,  1}, {10,  4}, { 2,  5}, { 8,  1}, { 6,  6}, /* Row 23 */
  { 1,  3},
  { 1,  0}, { 5,  1}, { 4,  2}, { 4,  1}, {12,  4}, { 2,  5}, { 6,  1}, { 7,  6}, /* Row 24 */
  { 1,  3},
  { 1,  0}, {12,  1}, {14,  4}, { 2,  5}, { 4,  1}, { 8,  6}, { 1,  3},           /* Row 25 */
  { 1,  0}, {11,  1}, { 7,  4}, { 2,  5}, { 7,  4}, { 2,  5}, { 2,  1}, { 9,  6}, /* Row 26 */
  { 1,  3},
  { 1,  0}, {10,  1}, { 7,  4}, { 2,  5}, { 2,  1}, { 7,  4}, { 2,  5}, {10,  6}, /* Row 27 */
  { 1,  3},
  { 1,  0}, { 9,  1}, { 7,  4}, { 2,  5}, { 4,  1}, { 7,  4}, { 2,  5}, { 9,  6}, /* Row 28 */
  { 1,  3},
  { 1,  0}, { 8,  1}, { 7,  4}, { 2,  5}, { 6,  1}, { 7,  4}, { 2,  5}, { 8,  6}, /* Row 29 */
  { 1,  3},
  { 1,  0}, { 7,  1}, { 7,  4}, { 2,  5}, { 8,  1}, { 7,  4}, { 2,  5}, { 7,  6}, /* Row 30 */
  { 1,  3},
  { 1,  0}, { 6,  1}, { 7,  4}, { 2,  5}, { 9,  1}, { 1,  6}, { 7,  4}, { 2,  5}, /* Row 31 */
  { 6,  6}, { 1,  3},
  { 1,  0}, { 5,  1}, { 7,  4}, { 2,  5}, { 9,  1}, { 3,  6}, { 7,  4}, { 2,  5}, /* Row 32 */
  { 5,  6}, { 1,  3},
  { 1,  0}, { 4,  1}, { 7,  4}, { 2,  5}, { 9,  1}, { 5,  6}, { 7,  4}, { 2,  5}, /* Row 33 */
  { 4,  6}, { 1,  3},
  { 1,  0}, { 4,  1}, { 6,  4}, { 2,  5}, { 8,  1}, { 8,  6}, { 7,  4}, { 1,  5}, /* Row 34 */
  { 4,  6}, { 1,  3},
  { 1,  0}, { 4,  1}, { 7,  5}, { 7,  1}, {11,  6}, { 7,  5}, { 4,  6}, { 1,  3}, /* Row 35 */
  { 1,  0}, {16,  1}, {24,  6}, { 1,  3},                                         /* Row 36 */
  { 1,  0}, {13,  1}, {27,  6}, { 1,  3},                                         /* Row 37 */
  { 1,  0}, { 8,  1}, {32,  6}, { 1,  3},                                         /* Row 38 */
  { 1,  0}, { 5,  1}, {35,  6}, { 1,  3},                                         /* Row 39 */
  { 1,  0}, { 2,  1}, {38,  6}, { 1,  3},                                         /* Row 40 */
  { 1,  0}, { 1,  7}, {40,  3}                                                    /* Row 41 */
};

/********************************************************************************************
 * Public Bitmap Structure Definitions
 ********************************************************************************************/

const struct SRlePaletteBitmap NXWidgets::g_stopBitmap =
{
  CONFIG_NXWIDGETS_BPP,  // bpp    - Bits per pixel
  CONFIG_NXWIDGETS_FMT,  // fmt    - Color format
  BITMAP_NLUTCODES,      // nlut   - Number of colors in the lLook-Up Table (LUT)
  BITMAP_NCOLUMNS,       // width  - Width in pixels
  BITMAP_NROWS,          // height - Height in rows
  {                      // lut    - Pointer to the beginning of the Look-Up Table (LUT)
    g_stopNormalLut,     //          Index 0: Unselected LUT
    g_stopBrightLut,     //          Index 1: Selected LUT
  },
  g_stopRleEntries       // data   - Pointer to the beginning of the RLE data
};
