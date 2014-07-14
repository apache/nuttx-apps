/********************************************************************************************
 * NxWidgets/nxwm/src/glyph_calculator47x47.cxx
 *
 *   Copyright (C) 2012 Gregory Nutt. All rights reserved.
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

#define BITMAP_NROWS         49
#define BITMAP_NCOLUMNS      47
#define BITMAP_NLUTCODES     13

#define DARK_CALCULATOR_ICON 1

/********************************************************************************************
 * Private Bitmap Data
 ********************************************************************************************/

using namespace NxWM;

/* RGB24 (8-8-8) Colors */

#if CONFIG_NXWIDGETS_BPP == 24 ||  CONFIG_NXWIDGETS_BPP == 32
#  ifdef DARK_CALCULATOR_ICON

static const uint32_t g_calculatorNormalLut[BITMAP_NLUTCODES] =
{
  0xababab, 0x969696, 0xbdbdbd, 0x545454, 0x546c96, 0x548196, 0x2a3f6c, 0x3f6c96,  /* Codes 0-7 */
  0x2a2a6c, 0x2a5481, 0x818181, 0x3f3f3f, 0x6c6c6c                                 /* Codes 8-12 */
};

static const uint32_t g_calculatorBrightLut[BITMAP_NLUTCODES] =
{
  0xe4e4e4, 0xc8c8c8, 0xfcfcfc, 0x707070, 0x7090c8, 0x70acc8, 0x385490, 0x5490c8,  /* Codes 0-7 */
  0x383890, 0x3870ac, 0xacacac, 0x545454, 0x909090                                 /* Codes 8-12 */
};

#  else /* DARK_CALCULATOR_ICON */

static const uint32_t g_calculatorNormalLut[BITMAP_NLUTCODES] =
{
  0xe4e4e4, 0xc8c8c8, 0xfcfcfc, 0x707070, 0x7090c8, 0x70acc8, 0x385490, 0x5490c8,  /* Codes 0-7 */
  0x383890, 0x3870ac, 0xacacac, 0x545454, 0x909090                                 /* Codes 8-12 */
};

static const uint32_t g_calculatorBrightLut[BITMAP_NLUTCODES] =
{
  0xffffff, 0xfafafa, 0xffffff, 0x8c8c8c, 0x8cb4fa, 0x8cd7fa, 0x4669b4, 0x69b4fa,  /* Codes 0-7 */
  0x4646b4, 0x468cd7, 0xd7d7d7, 0x696969, 0xb4b4b4                                 /* Codes 8-12 */
};
#  endif /* DARK_CALCULATOR_ICON */

/* RGB16 (565) Colors */

#elif CONFIG_NXWIDGETS_BPP == 16
#  ifdef DARK_CALCULATOR_ICON

static const uint16_t g_calculatorNormalLut[BITMAP_NLUTCODES] =
{
  0xad55, 0x94b2, 0xbdf7, 0x52aa, 0x5372, 0x5412, 0x29ed, 0x3b72, 0x294d, 0x2ab0,  /* Codes 0-9 */
  0x8410, 0x39e7, 0x6b6d,                                                          /* Codes 10-12 */
};

static const uint16_t g_calculatorBrightLut[BITMAP_NLUTCODES] =
{
  0xe73c, 0xce59, 0xffff, 0x738e, 0x7499, 0x7579, 0x3ab2, 0x5499, 0x39d2, 0x3b95,  /* Codes 0-9 */
  0xad75, 0x52aa, 0x9492                                                           /* Codes 10-12 */
};

#  else /* DARK_CALCULATOR_ICON */

static const uint16_t g_calculatorNormalLut[BITMAP_NLUTCODES] =
{
  0xe73c, 0xce59, 0xffff, 0x738e, 0x7499, 0x7579, 0x3ab2, 0x5499, 0x39d2, 0x3b95,  /* Codes 0-9 */
  0xad75, 0x52aa, 0x9492                                                           /* Codes 10-12 */
};

static const uint16_t g_calculatorBrightLut[BITMAP_NLUTCODES] =
{
  0xffff, 0xffdf, 0xffff, 0x8c71, 0x8dbf, 0x8ebf, 0x4356, 0x6dbf, 0x4236, 0x447a,  /* Codes 0-9 */
  0xd6ba, 0x6b4d, 0xb5b6                                                           /* Codes 10-12 */
};

#  endif /* DARK_CALCULATOR_ICON */

/* 8-bit color lookups.  NOTE:  This is really dumb!  The lookup index is 8-bits and it used
 * to lookup an 8-bit value.  There is no savings in that!  It would be better to just put
 * the 8-bit color/greyscale value in the run-length encoded image and save the cost of these
 * pointless lookups.  But these pointless lookups do make the logic compatible with the
 * 16- and 24-bit types.
 */

#elif CONFIG_NXWIDGETS_BPP == 8
#  ifdef CONFIG_NXWIDGETS_GREYSCALE

/* 8-bit Greyscale */

#    ifdef DARK_CALCULATOR_ICON

static const uint8_t g_calculatorNormalLut[BITMAP_NLUTCODES] =
{
  0xab, 0x96, 0xbd, 0x54, 0x69, 0x75, 0x3d, 0x63, 0x31, 0x4c, 0x81, 0x3f, 0x6c  /* Codes 0-12 */
};

static const uint8_t g_calculatorBrightLut[BITMAP_NLUTCODES] =
{
  0xe4, 0xc8, 0xfc, 0x70, 0x8c, 0x9d, 0x52, 0x84, 0x42, 0x66, 0xac, 0x54, 0x90  /* Codes 0-12 */
};

#    else /* DARK_CALCULATOR_ICON */

static const uint8_t g_calculatorNormalLut[BITMAP_NLUTCODES] =
{
  0xe4, 0xc8, 0xfc, 0x70, 0x8c, 0x9d, 0x52, 0x84, 0x42, 0x66, 0xac, 0x54, 0x90  /* Codes 0-12 */
};

static const uint8_t g_calculatorBrightLut[BITMAP_NLUTCODES] =
{
  0xff, 0xfa, 0xff, 0x8c, 0xb0, 0xc4, 0x67, 0xa5, 0x52, 0x7f, 0xd7, 0x69, 0xb4  /* Codes 0-12 */
};

#    endif /* DARK_CALCULATOR_ICON */

#  else /* CONFIG_NXWIDGETS_GREYSCALE */

/* RGB8 (332) Colors */

#    ifdef DARK_CALCULATOR_ICON

static const nxgl_mxpixel_t g_calculatorNormalLut[BITMAP_NLUTCODES] =
{
  0xb6, 0x92, 0xb6, 0x49, 0x4e, 0x52, 0x25, 0x2e, 0x25, 0x2a, 0x92, 0x24, 0x6d  /* Codes 0-12 */
};

static const nxgl_mxpixel_t g_calculatorBrightLut[BITMAP_NLUTCODES] =
{
  0xff, 0xdb, 0xff, 0x6d, 0x73, 0x77, 0x2a, 0x53, 0x26, 0x2e, 0xb6, 0x49, 0x92  /* Codes 0-12 */
};

#    else /* DARK_CALCULATOR_ICON */

static const nxgl_mxpixel_t g_calculatorNormalLut[BITMAP_NLUTCODES] =
{
  0xff, 0xdb, 0xff, 0x6d, 0x73, 0x77, 0x2a, 0x53, 0x26, 0x2e, 0xb6, 0x49, 0x92  /* Codes 0-12 */
};

static const nxgl_mxpixel_t g_calculatorBrightLut[BITMAP_NLUTCODES] =
{
  0xff, 0xff, 0xff, 0x92, 0x97, 0x9b, 0x4e, 0x77, 0x4a, 0x53, 0xdb, 0x6d, 0xb6  /* Codes 0-12 */
};

#    endif /* DARK_CALCULATOR_ICON */
#  endif /* CONFIG_NXWIDGETS_GREYSCALE */
#else
# error Unsupported pixel format
#endif

static const struct NXWidgets::SRlePaletteBitmapEntry g_calculatorRleEntries[] =
{
  {46,  0}, { 1,  1},                                                             /* Row 0 */
  { 1,  0}, {45,  2}, { 1,  1},                                                   /* Row 1 */
  { 1,  0}, { 1,  2}, {43,  1}, { 1,  2}, { 1,  1},                               /* Row 2 */
  { 1,  0}, { 1,  2}, { 1,  1}, {41,  3}, { 1,  0}, { 1,  2}, { 1,  1},           /* Row 3 */
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1,  4}, {36,  5}, { 1,  4}, { 1,  6}, /* Row 4 */
  { 1,  3}, { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1,  5}, {37,  7}, { 1,  8}, { 1,  3}, /* Row 5 */
  { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1,  5}, { 1,  7}, {35,  4}, { 1,  7}, /* Row 6 */
  { 1,  8}, { 1,  3}, { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1,  5}, { 1,  7}, {35,  4}, { 1,  7}, /* Row 7 */
  { 1,  8}, { 1,  3}, { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1,  5}, { 1,  7}, {35,  4}, { 1,  7}, /* Row 8 */
  { 1,  8}, { 1,  3}, { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1,  5}, { 1,  7}, {35,  4}, { 1,  7}, /* Row 9 */
  { 1,  8}, { 1,  3}, { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1,  5}, { 1,  7}, {35,  4}, { 1,  7}, /* Row 10 */
  { 1,  8}, { 1,  3}, { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1,  5}, { 1,  7}, {35,  4}, { 1,  7}, /* Row 11 */
  { 1,  8}, { 1,  3}, { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1,  5}, { 1,  7}, {35,  4}, { 1,  7}, /* Row 12 */
  { 1,  8}, { 1,  3}, { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1,  4}, {36,  7}, { 1,  9}, { 1,  8}, /* Row 13 */
  { 1,  3}, { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1,  6}, {38,  8}, { 1,  3}, { 1,  0}, /* Row 14 */
  { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, {41,  3}, { 1,  0}, { 1,  2}, { 1,  1},           /* Row 15 */
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 7,  1}, { 1, 10}, { 2,  3}, { 7,  1}, /* Row 16 */
  { 1, 10}, { 2,  3}, { 7,  1}, { 1, 10}, { 2,  3}, { 7,  1}, { 1, 10}, { 2,  3},
  { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1,  1}, { 1,  2}, { 5,  0}, { 1,  1}, /* Row 17 */
  { 1, 11}, { 1,  3}, { 1,  1}, { 1,  2}, { 5,  0}, { 1,  1}, { 2,  3}, { 1,  1},
  { 1,  2}, { 5,  0}, { 1,  1}, { 2,  3}, { 1,  1}, { 1,  2}, { 5,  0}, { 1,  1},
  { 1, 11}, { 1,  3}, { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1,  1}, { 1,  0}, { 1,  1}, { 4, 10}, /* Row 18 */
  { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1,  1}, { 4, 10}, { 1,  1},
  { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1,  1}, { 4, 10}, { 1,  1}, { 1, 11},
  { 1,  3}, { 1,  1}, { 1,  0}, { 1,  1}, { 4, 10}, { 1,  1}, { 1, 11}, { 1,  3},
  { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 3,  3}, /* Row 19 */
  { 1, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 3,  3},
  { 1, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 3,  3},
  { 1, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 3,  3},
  { 1, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1,  1}, { 1,  0}, { 1,  1}, { 2,  3}, /* Row 20 */
  { 2, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 2,  3},
  { 2, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 2,  3},
  { 2, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 2,  3},
  { 2, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1,  1}, { 1,  0}, { 1,  1}, { 1,  3}, /* Row 21 */
  { 3, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 1,  3},
  { 3, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 1,  3},
  { 3, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 1,  3},
  { 3, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1,  1}, { 1,  0}, { 1,  1}, { 3, 12}, /* Row 22 */
  { 2,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 3, 12}, { 2,  1},
  { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 3, 12}, { 2,  1}, { 1, 11},
  { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 3, 12}, { 2,  1}, { 1, 11}, { 1,  3},
  { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1, 10}, { 6,  1}, { 1,  3}, { 1, 11}, /* Row 23 */
  { 1,  3}, { 1, 10}, { 6,  1}, { 1,  3}, { 1, 11}, { 1,  3}, { 1, 10}, { 6,  1},
  { 1,  3}, { 1, 11}, { 1,  3}, { 1, 10}, { 6,  1}, { 1,  3}, { 1, 11}, { 1,  3},
  { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 2,  3}, { 8, 11}, { 2,  3}, { 8, 11}, { 2,  3}, /* Row 24 */
  { 8, 11}, { 2,  3}, { 8, 11}, { 1,  3}, { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, {41,  3}, { 1,  0}, { 1,  2}, { 1,  1},           /* Row 25 */
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 7,  1}, { 1, 10}, { 2,  3}, { 7,  1}, /* Row 26 */
  { 1, 10}, { 2,  3}, { 7,  1}, { 1, 10}, { 2,  3}, { 7,  1}, { 1, 10}, { 2,  3},
  { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1,  1}, { 1,  2}, { 5,  0}, { 1,  1}, /* Row 27 */
  { 1, 11}, { 1,  3}, { 1,  1}, { 1,  2}, { 5,  0}, { 1,  1}, { 1, 11}, { 1,  3},
  { 1,  1}, { 1,  2}, { 5,  0}, { 1,  1}, { 2,  3}, { 1,  1}, { 1,  2}, { 5,  0},
  { 1,  1}, { 1, 11}, { 1,  3}, { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1,  1}, { 1,  0}, { 1,  1}, { 4, 10}, /* Row 28 */
  { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1,  1}, { 4, 10}, { 1,  1},
  { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1,  1}, { 4, 10}, { 1,  1}, { 1, 11},
  { 1,  3}, { 1,  1}, { 1,  0}, { 1,  1}, { 4, 10}, { 1,  1}, { 1, 11}, { 1,  3},
  { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 3,  3}, /* Row 29 */
  { 1, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 3,  3},
  { 1, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 3,  3},
  { 1, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 3,  3},
  { 1, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 2,  3}, /* Row 30 */
  { 2, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 2,  3},
  { 2, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 2,  3},
  { 2, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 2,  3},
  { 2, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 1,  3}, /* Row 31 */
  { 3, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 1,  3},
  { 3, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 1,  3},
  { 3, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 1,  3},
  { 3, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 3, 12}, /* Row 32 */
  { 2,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 3, 12}, { 2,  1},
  { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 3, 12}, { 2,  1}, { 1, 11},
  { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 3, 12}, { 2,  1}, { 1, 11}, { 1,  3},
  { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1, 10}, { 6,  1}, { 1,  3}, { 1, 11}, /* Row 33 */
  { 1,  3}, { 1, 10}, { 6,  1}, { 1,  3}, { 1, 11}, { 1,  3}, { 1, 10}, { 6,  1},
  { 1,  3}, { 1, 11}, { 1,  3}, { 1, 10}, { 6,  1}, { 1,  3}, { 1, 11}, { 1,  3},
  { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 2,  3}, { 8, 11}, { 2,  3}, { 8, 11}, { 3,  3}, /* Row 34 */
  { 7, 11}, { 2,  3}, { 8, 11}, { 1,  3}, { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, {41,  3}, { 1,  0}, { 1,  2}, { 1,  1},           /* Row 35 */
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 7,  1}, { 1, 10}, { 2,  3}, { 7,  1}, /* Row 36 */
  { 1, 10}, { 2,  3}, { 7,  1}, { 1, 10}, { 2,  3}, { 7,  1}, { 1, 10}, { 2,  3},
  { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1,  1}, { 1,  2}, { 5,  0}, { 1,  1}, /* Row 37 */
  { 1, 11}, { 1,  3}, { 1,  1}, { 1,  2}, { 5,  0}, { 1,  1}, { 1, 11}, { 1,  3},
  { 1,  1}, { 1,  2}, { 5,  0}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  2},
  { 5,  0}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1,  1}, { 1,  0}, { 1,  1}, { 4, 10}, /* Row 38 */
  { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1,  1}, { 4, 10}, { 1,  1},
  { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1,  1}, { 4, 10}, { 1,  1}, { 1, 11},
  { 1,  3}, { 1,  1}, { 1,  0}, { 1,  1}, { 4, 10}, { 1,  1}, { 1, 11}, { 1,  3},
  { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 3,  3}, /* Row 39 */
  { 1, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 3,  3},
  { 1, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 3,  3},
  { 1, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 3,  3},
  { 1, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 2,  3}, /* Row 40 */
  { 2, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 2,  3},
  { 2, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 2,  3},
  { 2, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 2,  3},
  { 2, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 1,  3}, /* Row 41 */
  { 3, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 1,  3},
  { 3, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 1,  3},
  { 3, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 1,  3},
  { 3, 12}, { 1,  1}, { 1, 11}, { 1,  3}, { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 3, 12}, /* Row 42 */
  { 2,  1}, { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 3, 12}, { 2,  1},
  { 1, 11}, { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 3, 12}, { 2,  1}, { 1, 11},
  { 1,  3}, { 1,  1}, { 1,  0}, { 1, 10}, { 3, 12}, { 2,  1}, { 1, 11}, { 1,  3},
  { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 1,  3}, { 1, 10}, { 6,  1}, { 1,  3}, { 1, 11}, /* Row 43 */
  { 1,  3}, { 1, 10}, { 6,  1}, { 1,  3}, { 1, 11}, { 1,  3}, { 1, 10}, { 6,  1},
  { 1,  3}, { 1, 11}, { 1,  3}, { 1, 10}, { 6,  1}, { 1,  3}, { 1, 11}, { 1,  3},
  { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, { 2,  3}, { 8, 11}, { 2,  3}, { 8, 11}, { 2,  3}, /* Row 44 */
  { 8, 11}, { 2,  3}, { 8, 11}, { 1,  3}, { 1,  0}, { 1,  2}, { 1,  1},
  { 1,  0}, { 1,  2}, { 1,  1}, {41,  3}, { 1,  0}, { 1,  2}, { 1,  1},           /* Row 45 */
  { 1,  0}, { 1,  2}, {43,  0}, { 1,  2}, { 1,  1},                               /* Row 46 */
  { 1,  0}, {45,  2}, { 1,  1},                                                   /* Row 47 */
  {47,  1}                                                                        /* Row 48 */
};

/********************************************************************************************
 * Public Bitmap Structure Definitions
 ********************************************************************************************/

const struct NXWidgets::SRlePaletteBitmap NxWM::g_calculatorBitmap =
{
  CONFIG_NXWIDGETS_BPP,     // bpp    - Bits per pixel
  CONFIG_NXWIDGETS_FMT,     // fmt    - Color format
  BITMAP_NLUTCODES,         // nlut   - Number of colors in the lLook-Up Table (LUT)
  BITMAP_NCOLUMNS,          // width  - Width in pixels
  BITMAP_NROWS,             // height - Height in rows
  {                         // lut    - Pointer to the beginning of the Look-Up Table (LUT)
    g_calculatorNormalLut,  //          Index 0: Unselected LUT
    g_calculatorBrightLut,  //          Index 1: Selected LUT
  },
  g_calculatorRleEntries    // data   - Pointer to the beginning of the RLE data
};

