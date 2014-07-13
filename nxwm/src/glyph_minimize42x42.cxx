/********************************************************************************************
 * NxWidgets/nxwm/src/glyph_minimize42x42.cxx
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

#include "crlepalettebitmap.hxx"

#include "nxwmconfig.hxx"
#include "nxwmglyphs.hxx"

/********************************************************************************************
 * Pre-Processor Definitions
 ********************************************************************************************/

#define BITMAP_NROWS       42
#define BITMAP_NCOLUMNS    42
#define BITMAP_NLUTCODES   8

#define DARK_MINIMIZE_ICON 1

/********************************************************************************************
 * Private Bitmap Data
 ********************************************************************************************/

using namespace NxWM;

/* RGB24 (8-8-8) Colors */

#if CONFIG_NXWIDGETS_BPP == 24 ||  CONFIG_NXWIDGETS_BPP == 32
#  ifdef DARK_MINIMIZE_ICON

static const uint32_t g_minimizeNormalLut[BITMAP_NLUTCODES] =
{
  0x1b368a, 0x3651a5, 0x001b8a, 0x001ba5, 0x1b1b6f, 0x00008a, 0xbdbdbd, 0xa5bdbd  /* Codes 0-7 */
};

static const uint32_t g_minimizeBrightLut[BITMAP_NLUTCODES] =
{
  0x2448b8, 0x486cdc, 0x0024b8, 0x0024dc, 0x242494, 0x0000b8, 0xfcfcfc, 0xdcfcfc  /* Codes 0-7 */
};

#  else /* DARK_MINIMIZE_ICON */

static const uint32_t g_minimizeNormalLut[BITMAP_NLUTCODES] =
{
  0x2448b8, 0x486cdc, 0x0024b8, 0x0024dc, 0x242494, 0x0000b8, 0xfcfcfc, 0xdcfcfc  /* Codes 0-7 */
};

static const uint32_t g_minimizeBrightLut[BITMAP_NLUTCODES] =
{
  0x2d5ae6, 0x5a87ff, 0x002de6, 0x002dff, 0x2d2db9, 0x0000e6, 0xffffff, 0xffffff  /* Codes 0-7 */
};
#  endif /* DARK_MINIMIZE_ICON */

/* RGB16 (565) Colors */

#elif CONFIG_NXWIDGETS_BPP == 16
#  ifdef DARK_MINIMIZE_ICON

static const uint16_t g_minimizeNormalLut[BITMAP_NLUTCODES] =
{
  0x19b1, 0x3294, 0x00d1, 0x00d4, 0x18cd, 0x0011, 0xbdf7, 0xa5f7  /* Codes 0-7 */
};

static const uint16_t g_minimizeBrightLut[BITMAP_NLUTCODES] =
{
  0x2257, 0x4b7b, 0x0137, 0x013b, 0x2132, 0x0017, 0xffff, 0xdfff  /* Codes 0-7 */
};

#  else /* DARK_MINIMIZE_ICON */

static const uint16_t g_minimizeNormalLut[BITMAP_NLUTCODES] =
{
  0x2257, 0x4b7b, 0x0137, 0x013b, 0x2132, 0x0017, 0xffff, 0xdfff  /* Codes 0-7 */
};

static const uint16_t g_minimizeBrightLut[BITMAP_NLUTCODES] =
{
  0x2adc, 0x5c3f, 0x017c, 0x017f, 0x2977, 0x001c, 0xffff, 0xffff  /* Codes 0-7 */
};

#  endif /* DARK_MINIMIZE_ICON */

/* 8-bit color lookups.  NOTE:  This is really dumb!  The lookup index is 8-bits and it used
 * to lookup an 8-bit value.  There is no savings in that!  It would be better to just put
 * the 8-bit color/greyscale value in the run-length encoded image and save the cost of these
 * pointless lookups.  But these pointless lookups do make the logic compatible with the
 * 16- and 24-bit types.
 */

#elif CONFIG_NXWIDGETS_BPP == 8
#  ifdef CONFIG_NXWIDGETS_GREYSCALE

/* 8-bit Greyscale */

#    ifdef DARK_MINIMIZE_ICON

static const uint8_t g_minimizeNormalLut[BITMAP_NLUTCODES] =
{
  0x37, 0x52, 0x1f, 0x22, 0x24, 0x0f, 0xbd, 0xb5  /* Codes 0-7 */
};

static const uint8_t g_minimizeBrightLut[BITMAP_NLUTCODES] =
{
  0x4a, 0x6e, 0x2a, 0x2e, 0x30, 0x14, 0xfc, 0xf2  /* Codes 0-7 */
};

#    else /* DARK_MINIMIZE_ICON */

static const uint8_t g_minimizeNormalLut[BITMAP_NLUTCODES] =
{
  0x4a, 0x6e, 0x2a, 0x2e, 0x30, 0x14, 0xfc, 0xf2  /* Codes 0-7 */
};

static const uint8_t g_minimizeBrightLut[BITMAP_NLUTCODES] =
{
  0x5c, 0x87, 0x34, 0x37, 0x3c, 0x1a, 0xff, 0xff  /* Codes 0-7 */
};

#    endif /* DARK_MINIMIZE_ICON */

#  else /* CONFIG_NXWIDGETS_GREYSCALE */

/* RGB8 (332) Colors */

#    ifdef DARK_MINIMIZE_ICON

static const nxgl_mxpixel_t g_minimizeNormalLut[BITMAP_NLUTCODES] =
{
  0x06, 0x2a, 0x02, 0x02, 0x01, 0x02, 0xb6, 0xb6  /* Codes 0-7 */
};

static const nxgl_mxpixel_t g_minimizeBrightLut[BITMAP_NLUTCODES] =
{
  0x2a, 0x4f, 0x06, 0x07, 0x26, 0x02, 0xff, 0xdf  /* Codes 0-7 */
};

#    else /* DARK_MINIMIZE_ICON */

static const nxgl_mxpixel_t g_minimizeNormalLut[BITMAP_NLUTCODES] =
{
  0x2a, 0x4f, 0x06, 0x07, 0x26, 0x02, 0xff, 0xdf  /* Codes 0-7 */
};

static const nxgl_mxpixel_t g_minimizeBrightLut[BITMAP_NLUTCODES] =
{
  0x2b, 0x53, 0x07, 0x07, 0x26, 0x03, 0xff, 0xff  /* Codes 0-7 */
};

#    endif /* DARK_MINIMIZE_ICON */
#  endif /* CONFIG_NXWIDGETS_GREYSCALE */
#else
# error Unsupported pixel format
#endif

static const struct NXWidgets::SRlePaletteBitmapEntry g_minimizeRleEntries[] =
{
  { 2,  0}, {40,  1},                                         /* Row 0 */
  { 2,  0}, {40,  1},                                         /* Row 1 */
  { 2,  1}, {10,  2}, {10,  3}, {16,  2}, { 2,  4}, { 2,  0}, /* Row 2 */
  { 2,  1}, {10,  2}, {10,  3}, {16,  2}, { 2,  4}, { 2,  0}, /* Row 3 */
  { 2,  1}, { 6,  2}, {16,  3}, {16,  2}, { 2,  4},           /* Row 4 */
  { 2,  1}, { 6,  2}, {16,  3}, {16,  2}, { 2,  4},           /* Row 5 */
  { 2,  1}, { 4,  2}, {20,  3}, {14,  2}, { 2,  4},           /* Row 6 */
  { 2,  1}, { 4,  2}, {20,  3}, {14,  2}, { 2,  4},           /* Row 7 */
  { 2,  1}, { 2,  2}, {20,  3}, {16,  2}, { 2,  4},           /* Row 8 */
  { 2,  1}, { 2,  2}, {20,  3}, {16,  2}, { 2,  4},           /* Row 9 */
  { 2,  1}, { 2,  2}, {18,  3}, {18,  2}, { 2,  4},           /* Row 10 */
  { 2,  1}, { 2,  2}, {18,  3}, {18,  2}, { 2,  4},           /* Row 11 */
  { 2,  1}, {18,  3}, {18,  2}, { 2,  5}, { 2,  4},           /* Row 12 */
  { 2,  1}, {18,  3}, {18,  2}, { 2,  5}, { 2,  4},           /* Row 13 */
  { 2,  1}, {16,  3}, {20,  2}, { 2,  5}, { 2,  4},           /* Row 14 */
  { 2,  1}, {16,  3}, {20,  2}, { 2,  5}, { 2,  4},           /* Row 15 */
  { 2,  1}, {14,  3}, {22,  2}, { 2,  5}, { 2,  4},           /* Row 16 */
  { 2,  1}, {14,  3}, {22,  2}, { 2,  5}, { 2,  4},           /* Row 17 */
  { 2,  1}, { 2,  2}, { 8,  3}, {24,  2}, { 4,  5}, { 2,  4}, /* Row 18 */
  { 2,  1}, { 2,  2}, { 8,  3}, {24,  2}, { 4,  5}, { 2,  4}, /* Row 19 */
  { 2,  1}, { 4,  2}, { 2,  3}, {28,  2}, { 4,  5}, { 2,  4}, /* Row 20 */
  { 2,  1}, { 4,  2}, { 2,  3}, {28,  2}, { 4,  5}, { 2,  4}, /* Row 21 */
  { 2,  1}, {32,  2}, { 6,  5}, { 2,  4},                     /* Row 22 */
  { 2,  1}, {32,  2}, { 6,  5}, { 2,  4},                     /* Row 23 */
  { 2,  1}, {30,  2}, { 8,  5}, { 2,  4},                     /* Row 24 */
  { 2,  1}, {30,  2}, { 8,  5}, { 2,  4},                     /* Row 25 */
  { 2,  1}, {26,  2}, {12,  5}, { 2,  4},                     /* Row 26 */
  { 2,  1}, {26,  2}, {12,  5}, { 2,  4},                     /* Row 27 */
  { 2,  1}, {22,  2}, {16,  5}, { 2,  4},                     /* Row 28 */
  { 2,  1}, {22,  2}, {16,  5}, { 2,  4},                     /* Row 29 */
  { 2,  1}, {16,  2}, {22,  5}, { 2,  4},                     /* Row 30 */
  { 2,  1}, {16,  2}, {22,  5}, { 2,  4},                     /* Row 31 */
  { 2,  1}, { 8,  2}, {30,  5}, { 2,  4},                     /* Row 32 */
  { 2,  1}, { 8,  2}, {30,  5}, { 2,  4},                     /* Row 33 */
  { 2,  1}, { 4,  2}, {28,  6}, { 2,  7}, { 4,  5}, { 2,  4}, /* Row 34 */
  { 2,  1}, { 4,  2}, {28,  6}, { 2,  7}, { 4,  5}, { 2,  4}, /* Row 35 */
  { 2,  1}, { 2,  2}, { 2,  5}, {30,  7}, { 4,  5}, { 2,  4}, /* Row 36 */
  { 2,  1}, { 2,  2}, { 2,  5}, {30,  7}, { 4,  5}, { 2,  4}, /* Row 37 */
  { 2,  1}, {38,  5}, { 2,  0},                               /* Row 38 */
  { 2,  1}, {38,  5}, { 2,  0},                               /* Row 39 */
  { 2,  1}, { 2,  0}, {34,  4}, { 2,  0}, { 2,  1},           /* Row 40 */
  { 2,  1}, { 2,  0}, {34,  4}, { 2,  0}, { 2,  1}            /* Row 41 */
};

/********************************************************************************************
 * Public Bitmap Structure Definitions
 ********************************************************************************************/

const struct NXWidgets::SRlePaletteBitmap NxWM::g_minimizeBitmap =
{
  CONFIG_NXWIDGETS_BPP,  // bpp    - Bits per pixel
  CONFIG_NXWIDGETS_FMT,  // fmt    - Color format
  BITMAP_NLUTCODES,      // nlut   - Number of colors in the lLook-Up Table (LUT)
  BITMAP_NCOLUMNS,       // width  - Width in pixels
  BITMAP_NROWS,          // height - Height in rows
  {                      // lut    - Pointer to the beginning of the Look-Up Table (LUT)
    g_minimizeNormalLut, //          Index 0: Unselected LUT
    g_minimizeBrightLut, //          Index 1: Selected LUT
  },
  g_minimizeRleEntries  // data   - Pointer to the beginning of the RLE data
};
