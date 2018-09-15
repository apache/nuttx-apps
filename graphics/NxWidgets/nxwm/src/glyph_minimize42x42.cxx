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
#define BITMAP_NLUTCODES   7

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
  0x6f99b4, 0x001b8a, 0x001ba5, 0x060f4b, 0x00007b, 0xbdbdbd, 0xa5bdbd  /* Codes 0-6 */
};

static const uint32_t g_minimizeBrightLut[BITMAP_NLUTCODES] =
{
  0x94ccf0, 0x0024b8, 0x0024dc, 0x081464, 0x0000a4, 0xfcfcfc, 0xdcfcfc  /* Codes 0-6 */
};

#  else /* DARK_MINIMIZE_ICON */

static const uint32_t g_minimizeNormalLut[BITMAP_NLUTCODES] =
{
  0x94ccf0, 0x0024b8, 0x0024dc, 0x081464, 0x0000a4, 0xfcfcfc, 0xdcfcfc  /* Codes 0-6 */
};

static const uint32_t g_minimizeBrightLut[BITMAP_NLUTCODES] =
{
  0xb9ffff, 0x002de6, 0x002dff, 0x0a197d, 0x0000cd, 0xffffff, 0xffffff  /* Codes 0-6 */
};
#  endif /* DARK_MINIMIZE_ICON */

/* RGB16 (565) Colors */

#elif CONFIG_NXWIDGETS_BPP == 16
#  ifdef DARK_MINIMIZE_ICON

static const uint16_t g_minimizeNormalLut[BITMAP_NLUTCODES] =
{
  0x6cd6, 0x00d1, 0x00d4, 0x0069, 0x000f, 0xbdf7, 0xa5f7  /* Codes 0-6 */
};

static const uint16_t g_minimizeBrightLut[BITMAP_NLUTCODES] =
{
  0x967e, 0x0137, 0x013b, 0x08ac, 0x0014, 0xffff, 0xdfff  /* Codes 0-6 */
};

#  else /* DARK_MINIMIZE_ICON */

static const uint16_t g_minimizeNormalLut[BITMAP_NLUTCODES] =
{
  0x967e, 0x0137, 0x013b, 0x08ac, 0x0014, 0xffff, 0xdfff  /* Codes 0-6 */
};

static const uint16_t g_minimizeBrightLut[BITMAP_NLUTCODES] =
{
  0xbfff, 0x017c, 0x017f, 0x08cf, 0x0019, 0xffff, 0xffff  /* Codes 0-6 */
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
  0x8f, 0x1f, 0x22, 0x13, 0x0e, 0xbd, 0xb5  /* Codes 0-6 */
};

static const uint8_t g_minimizeBrightLut[BITMAP_NLUTCODES] =
{
  0xbf, 0x2a, 0x2e, 0x19, 0x12, 0xfc, 0xf2  /* Codes 0-6 */
};

#    else /* DARK_MINIMIZE_ICON */

static const uint8_t g_minimizeNormalLut[BITMAP_NLUTCODES] =
{
  0xbf, 0x2a, 0x2e, 0x19, 0x12, 0xfc, 0xf2  /* Codes 0-6 */
};

static const uint8_t g_minimizeBrightLut[BITMAP_NLUTCODES] =
{
  0xea, 0x34, 0x37, 0x1f, 0x17, 0xff, 0xff  /* Codes 0-6 */
};

#    endif /* DARK_MINIMIZE_ICON */

#  else /* CONFIG_NXWIDGETS_GREYSCALE */

/* RGB8 (332) Colors */

#    ifdef DARK_MINIMIZE_ICON

static const nxgl_mxpixel_t g_minimizeNormalLut[BITMAP_NLUTCODES] =
{
  0x72, 0x02, 0x02, 0x01, 0x01, 0xb6, 0xb6  /* Codes 0-6 */
};

static const nxgl_mxpixel_t g_minimizeBrightLut[BITMAP_NLUTCODES] =
{
  0x9b, 0x06, 0x07, 0x01, 0x02, 0xff, 0xdf  /* Codes 0-6 */
};

#    else /* DARK_MINIMIZE_ICON */

static const nxgl_mxpixel_t g_minimizeNormalLut[BITMAP_NLUTCODES] =
{
  0x9b, 0x06, 0x07, 0x01, 0x02, 0xff, 0xdf  /* Codes 0-6 */
};

static const nxgl_mxpixel_t g_minimizeBrightLut[BITMAP_NLUTCODES] =
{
  0xbf, 0x07, 0x07, 0x01, 0x03, 0xff, 0xff  /* Codes 0-6 */
};

#    endif /* DARK_MINIMIZE_ICON */
#  endif /* CONFIG_NXWIDGETS_GREYSCALE */
#else
# error Unsupported pixel format
#endif

static const struct NXWidgets::SRlePaletteBitmapEntry g_minimizeRleEntries[] =
{
  {42, 0},                                               /* Row 0 */
  { 1, 0}, {11, 1}, {10, 2}, {19, 1}, { 1, 0},           /* Row 1 */
  { 1, 0}, { 9, 1}, {14, 2}, {17, 1}, { 1, 3},           /* Row 2 */
  { 1, 0}, { 7, 1}, {17, 2}, {16, 1}, { 1, 3},           /* Row 3 */
  { 1, 0}, { 6, 1}, {18, 2}, {16, 1}, { 1, 3},           /* Row 4 */
  { 1, 0}, { 5, 1}, {19, 2}, {16, 1}, { 1, 3},           /* Row 5 */
  { 1, 0}, { 4, 1}, {20, 2}, {16, 1}, { 1, 3},           /* Row 6 */
  { 1, 0}, { 3, 1}, {20, 2}, {17, 1}, { 1, 3},           /* Row 7 */
  { 1, 0}, { 3, 1}, {20, 2}, {17, 1}, { 1, 3},           /* Row 8 */
  { 1, 0}, { 2, 1}, {20, 2}, {17, 1}, { 1, 4}, { 1, 3},  /* Row 9 */
  { 1, 0}, { 2, 1}, {19, 2}, {18, 1}, { 1, 4}, { 1, 3},  /* Row 10 */
  { 1, 0}, { 1, 1}, {19, 2}, {19, 1}, { 1, 4}, { 1, 3},  /* Row 11 */
  { 1, 0}, { 1, 1}, {18, 2}, {20, 1}, { 1, 4}, { 1, 3},  /* Row 12 */
  { 1, 0}, {18, 2}, {20, 1}, { 2, 4}, { 1, 3},           /* Row 13 */
  { 1, 0}, {17, 2}, {21, 1}, { 2, 4}, { 1, 3},           /* Row 14 */
  { 1, 0}, {15, 2}, {22, 1}, { 3, 4}, { 1, 3},           /* Row 15 */
  { 1, 0}, {14, 2}, {23, 1}, { 3, 4}, { 1, 3},           /* Row 16 */
  { 1, 0}, {12, 2}, {24, 1}, { 4, 4}, { 1, 3},           /* Row 17 */
  { 1, 0}, {10, 2}, {26, 1}, { 4, 4}, { 1, 3},           /* Row 18 */
  { 1, 0}, { 1, 1}, { 8, 2}, {26, 1}, { 5, 4}, { 1, 3},  /* Row 19 */
  { 1, 0}, { 2, 1}, { 5, 2}, {27, 1}, { 6, 4}, { 1, 3},  /* Row 20 */
  { 1, 0}, { 4, 1}, { 1, 2}, {29, 1}, { 6, 4}, { 1, 3},  /* Row 21 */
  { 1, 0}, {33, 1}, { 7, 4}, { 1, 3},                    /* Row 22 */
  { 1, 0}, {32, 1}, { 8, 4}, { 1, 3},                    /* Row 23 */
  { 1, 0}, {31, 1}, { 9, 4}, { 1, 3},                    /* Row 24 */
  { 1, 0}, {30, 1}, {10, 4}, { 1, 3},                    /* Row 25 */
  { 1, 0}, {29, 1}, {11, 4}, { 1, 3},                    /* Row 26 */
  { 1, 0}, {28, 1}, {12, 4}, { 1, 3},                    /* Row 27 */
  { 1, 0}, {27, 1}, {13, 4}, { 1, 3},                    /* Row 28 */
  { 1, 0}, {26, 1}, {14, 4}, { 1, 3},                    /* Row 29 */
  { 1, 0}, {24, 1}, {16, 4}, { 1, 3},                    /* Row 30 */
  { 1, 0}, {22, 1}, {18, 4}, { 1, 3},                    /* Row 31 */
  { 1, 0}, { 6, 1}, {28, 5}, { 6, 4}, { 1, 3},           /* Row 32 */
  { 1, 0}, { 5, 1}, {29, 5}, { 1, 6}, { 5, 4}, { 1, 3},  /* Row 33 */
  { 1, 0}, { 5, 1}, {29, 5}, { 1, 6}, { 5, 4}, { 1, 3},  /* Row 34 */
  { 1, 0}, { 5, 1}, {29, 5}, { 1, 6}, { 5, 4}, { 1, 3},  /* Row 35 */
  { 1, 0}, { 5, 1}, {29, 5}, { 1, 6}, { 5, 4}, { 1, 3},  /* Row 36 */
  { 1, 0}, { 6, 1}, {28, 6}, { 6, 4}, { 1, 3},           /* Row 37 */
  { 1, 0}, { 4, 1}, {36, 4}, { 1, 3},                    /* Row 38 */
  { 1, 0}, { 2, 1}, {38, 4}, { 1, 3},                    /* Row 39 */
  { 1, 0}, {40, 4}, { 1, 3},                             /* Row 40 */
  { 2, 0}, {40, 3}                                       /* Row 41 */
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
  g_minimizeRleEntries   // data   - Pointer to the beginning of the RLE data
};
