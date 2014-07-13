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

#define BITMAP_NROWS     49
#define BITMAP_NCOLUMNS  47
#define BITMAP_NLUTCODES 22

#define DARK_CALCULATOR_ICON   1

/********************************************************************************************
 * Private Bitmap Data
 ********************************************************************************************/

using namespace NxWM;

/* RGB24 (8-8-8) Colors */

#if CONFIG_NXWIDGETS_BPP == 24 ||  CONFIG_NXWIDGETS_BPP == 32
#  ifdef DARK_CALCULATOR_ICON

static const uint32_t g_calculatorNormalLut[BITMAP_NLUTCODES] =
{
  0xaeaeae, 0x8a8a8a, 0xbdbdbd, 0x575757, 0x57698a, 0x57788a, 0x577b8a, 0x577b9c,  /* Codes 0-7 */
  0x364569, 0x4b6c96, 0x48699c, 0x486c99, 0x45699c, 0x45698a, 0x243669, 0x57699c,  /* Codes 8-15 */
  0x36577b, 0x9c9c9c, 0x7b7b7b, 0x454545, 0x696969, 0x000000                       /* Codes 16-21 */
};

static const uint32_t g_calculatorBrightlLut[BITMAP_NLUTCODES] =
{
  0xe8e8e8, 0xb8b8b8, 0xfcfcfc, 0x747474, 0x748cb8, 0x74a0b8, 0x74a4b8, 0x74a4d0,  /* Codes 0-7 */
  0x485c8c, 0x6490c8, 0x608cd0, 0x6090cc, 0x5c8cd0, 0x5c8cb8, 0x30488c, 0x748cd0,  /* Codes 8-15 */
  0x4874a4, 0xd0d0d0, 0xa4a4a4, 0x5c5c5c, 0x8c8c8c, 0x000000                       /* Codes 16-21 */
};

#  else /* DARK_CALCULATOR_ICON */

static const uint32_t g_calculatorNormalLut[BITMAP_NLUTCODES] =
{
  0xe8e8e8, 0xb8b8b8, 0xfcfcfc, 0x747474, 0x748cb8, 0x74a0b8, 0x74a4b8, 0x74a4d0,  /* Codes 0-7 */
  0x485c8c, 0x6490c8, 0x608cd0, 0x6090cc, 0x5c8cd0, 0x5c8cb8, 0x30488c, 0x748cd0,  /* Codes 8-15 */
  0x4874a4, 0xd0d0d0, 0xa4a4a4, 0x5c5c5c, 0x8c8c8c, 0x000000                       /* Codes 16-21 */
};

static const uint32_t g_calculatorBrightlLut[BITMAP_NLUTCODES] =
{
  0xffffff, 0xe6e6e6, 0xffffff, 0x919191, 0x91afe6, 0x91c8e6, 0x91cde6, 0x91cdff,  /* Codes 0-7 */
  0x5a73af, 0x7db4fa, 0x78afff, 0x78b4ff, 0x73afff, 0x73afe6, 0x3c5aaf, 0x91afff,  /* Codes 8-15 */
  0x5a91cd, 0xffffff, 0xcdcdcd, 0x737373, 0xafafaf, 0x000000                       /* Codes 16-21 */
};
#  endif /* DARK_CALCULATOR_ICON */

/* RGB16 (565) Colors */

#elif CONFIG_NXWIDGETS_BPP == 16
#  ifdef DARK_CALCULATOR_ICON

static const uint16_t g_calculatorNormalLut[BITMAP_NLUTCODES] =
{
  0xad75, 0x8c51, 0xbdf7, 0x52aa, 0x5351, 0x53d1, 0x53d1, 0x53d3, 0x322d, 0x4b72,  /* Codes 0-9 */
  0x4b53, 0x4b73, 0x4353, 0x4351, 0x21ad, 0x5353, 0x32af, 0x9cf3, 0x7bcf, 0x4228,  /* Codes 10-19 */
  0x6b4d, 0x0000                                                                   /* Codes 20-21 */
};

static const uint16_t g_calculatorBrightlLut[BITMAP_NLUTCODES] =
{
  0xef5d, 0xbdd7, 0xffff, 0x73ae, 0x7477, 0x7517, 0x7537, 0x753a, 0x4af1, 0x6499,  /* Codes 0-9 */
  0x647a, 0x6499, 0x5c7a, 0x5c77, 0x3251, 0x747a, 0x4bb4, 0xd69a, 0xa534, 0x5aeb,  /* Codes 10-19 */
  0x8c71, 0x0000                                                                   /* Codes 20-21 */
};

#  else /* DARK_CALCULATOR_ICON */

static const uint16_t g_calculatorNormalLut[BITMAP_NLUTCODES] =
{
  0xef5d, 0xbdd7, 0xffff, 0x73ae, 0x7477, 0x7517, 0x7537, 0x753a, 0x4af1, 0x6499,  /* Codes 0-9 */
  0x647a, 0x6499, 0x5c7a, 0x5c77, 0x3251, 0x747a, 0x4bb4, 0xd69a, 0xa534, 0x5aeb,  /* Codes 10-19 */
  0x8c71, 0x0000                                                                   /* Codes 20-21 */
};

static const uint16_t g_calculatorBrightlLut[BITMAP_NLUTCODES] =
{
  0xffff, 0xe73c, 0xffff, 0x9492, 0x957c, 0x965c, 0x967c, 0x967f, 0x5b95, 0x7dbf,  /* Codes 0-9 */
  0x7d7f, 0x7dbf, 0x757f, 0x757c, 0x3ad5, 0x957f, 0x5c99, 0xffff, 0xce79, 0x738e,  /* Codes 10-19 */
  0xad75, 0x0000                                                                   /* Codes 20-21 */
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
  0xae, 0x8a, 0xbd, 0x57, 0x67, 0x70, 0x71, 0x73, 0x44, 0x66, 0x64, 0x66, 0x64, 0x61, 0x36, 0x69,  /* Codes 0-15 */
  0x51, 0x9c, 0x7b, 0x45, 0x69, 0x00                                                               /* Codes 16-21 */
};

static const uint8_t g_calculatorBrightlLut[BITMAP_NLUTCODES] =
{
  0xe8, 0xb8, 0xfc, 0x74, 0x89, 0x95, 0x97, 0x9a, 0x5b, 0x89, 0x86, 0x88, 0x85, 0x82, 0x48, 0x8c,  /* Codes 0-15 */
  0x6c, 0xd0, 0xa4, 0x5c, 0x8c, 0x00                                                               /* Codes 16-21 */
};

#    else /* DARK_CALCULATOR_ICON */

static const uint8_t g_calculatorNormalLut[BITMAP_NLUTCODES] =
{
  0xe8, 0xb8, 0xfc, 0x74, 0x89, 0x95, 0x97, 0x9a, 0x5b, 0x89, 0x86, 0x88, 0x85, 0x82, 0x48, 0x8c,  /* Codes 0-15 */
  0x6c, 0xd0, 0xa4, 0x5c, 0x8c, 0x00                                                               /* Codes 16-21 */
};

static const uint8_t g_calculatorBrightlLut[BITMAP_NLUTCODES] =
{
  0xff, 0xe6, 0xff, 0x91, 0xac, 0xba, 0xbd, 0xc0, 0x72, 0xab, 0xa7, 0xaa, 0xa6, 0xa3, 0x5a, 0xaf,  /* Codes 0-15 */
  0x87, 0xff, 0xcd, 0x73, 0xaf, 0x00                                                               /* Codes 16-21 */
};

#    endif /* DARK_CALCULATOR_ICON */

#  else /* CONFIG_NXWIDGETS_GREYSCALE */

/* RGB8 (332) Colors */

#    ifdef DARK_CALCULATOR_ICON

static const nxgl_mxpixel_t g_calculatorNormalLut[BITMAP_NLUTCODES] =
{
  0xb6, 0x92, 0xb6, 0x49, 0x4e, 0x4e, 0x4e, 0x4e, 0x29, 0x4e, 0x4e, 0x4e, 0x4e, 0x4e, 0x25, 0x4e,  /* Codes 0-15 */
  0x29, 0x92, 0x6d, 0x49, 0x6d, 0x00                                                               /* Codes 16-21 */
};

static const nxgl_mxpixel_t g_calculatorBrightlLut[BITMAP_NLUTCODES] =
{
  0xff, 0xb6, 0xff, 0x6d, 0x72, 0x76, 0x76, 0x77, 0x4a, 0x73, 0x73, 0x73, 0x53, 0x52, 0x2a, 0x73,  /* Codes 0-15 */
  0x4e, 0xdb, 0xb6, 0x49, 0x92, 0x00                                                               /* Codes 16-21 */
};

#    else /* DARK_CALCULATOR_ICON */

static const nxgl_mxpixel_t g_calculatorNormalLut[BITMAP_NLUTCODES] =
{
  0xff, 0xb6, 0xff, 0x6d, 0x72, 0x76, 0x76, 0x77, 0x4a, 0x73, 0x73, 0x73, 0x53, 0x52, 0x2a, 0x73,  /* Codes 0-15 */
  0x4e, 0xdb, 0xb6, 0x49, 0x92, 0x00                                                               /* Codes 16-21 */
};

static const nxgl_mxpixel_t g_calculatorBrightlLut[BITMAP_NLUTCODES] =
{
  0xff, 0xff, 0xff, 0x92, 0x97, 0x9b, 0x9b, 0x9b, 0x4e, 0x77, 0x77, 0x77, 0x77, 0x77, 0x2a, 0x97,  /* Codes 0-15 */
  0x53, 0xff, 0xdb, 0x6d, 0xb6, 0x00                                                               /* Codes 16-21 */
};

#    endif /* DARK_CALCULATOR_ICON */
#  endif /* CONFIG_NXWIDGETS_GREYSCALE */
#else
# error Unsupported pixel format
#endif

static const struct NXWidgets::SRlePaletteBitmapEntry g_calculatorRleEntries[] =
{
  { 46,   0},  {  1,   1},  {  1,   0},                                                                   /* Row 0 */
  { 45,   2},  {  1,   1},  {  1,   0},  {  1,   2},                                                      /* Row 1 */
  { 43,   1},  {  1,   2},  {  1,   1},  {  1,   0},  {  1,   2},  {  1,   1},                            /* Row 2 */
  { 41,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   3},  /* Row 3 */
  {  1,   4},  {  1,   5},  { 34,   6},  {  1,   7},  {  1,   4},  {  1,   8},  {  1,   3},  {  1,   0},  /* Row 4 */
  {  1,   2},  {  1,   1},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   3},  {  1,   6},
  {  1,   9},  {  1,  10},  {  2,  11},  { 32,  12},  {  1,  13},  {  1,  14},  {  1,   3},  {  1,   0},  /* Row 5 */
  {  1,   2},  {  1,   1},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   3},  {  1,   6},  {  1,  12},
  { 35,  15},  {  1,  13},  {  1,  14},  {  1,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  /* Row 6 */
  {  1,   2},  {  1,   1},  {  1,   3},  {  1,   6},  {  1,  12},  {  1,  15},
  { 34,  15},  {  1,  13},  {  1,  14},  {  1,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  /* Row 7 */
  {  1,   2},  {  1,   1},  {  1,   3},  {  1,   6},  {  1,  12},  {  2,  15},
  { 33,  15},  {  1,  13},  {  1,  14},  {  1,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  /* Row 8 */
  {  1,   2},  {  1,   1},  {  1,   3},  {  1,   6},  {  1,  12},  {  3,  15},
  { 32,  15},  {  1,  13},  {  1,  14},  {  1,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  /* Row 9 */
  {  1,   2},  {  1,   1},  {  1,   3},  {  1,   6},  {  1,  12},  {  4,  15},
  { 31,  15},  {  1,  13},  {  1,  14},  {  1,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  /* Row 10 */
  {  1,   2},  {  1,   1},  {  1,   3},  {  1,   6},  {  1,  12},  {  5,  15},
  { 30,  15},  {  1,  13},  {  1,  14},  {  1,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  /* Row 11 */
  {  1,   2},  {  1,   1},  {  1,   3},  {  1,   6},  {  1,  12},  {  6,  15},
  { 29,  15},  {  1,  13},  {  1,  14},  {  1,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  /* Row 12 */
  {  1,   2},  {  1,   1},  {  1,   3},  {  1,   4},  {  8,  13},
  { 28,  13},  {  1,  16},  {  1,  14},  {  1,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  /* Row 13 */
  {  1,   2},  {  1,   1},  {  1,   3},  {  1,   8},  {  9,  14},
  { 29,  14},  {  1,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  {  1,   2},  {  1,   1},  /* Row 14 */
  { 12,   3},
  { 29,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   3},  /* Row 15 */
  {  1,   1},  {  6,  17},  {  1,  18},  {  2,   3},  {  1,   1}, {  1,  17},
  {  5,  17},  {  1,  18},  {  2,   3},  {  1,   1},  {  6,  17},  {  1,  18},  {  2,   3},  {  1,   1},  /* Row 16 */
  {  6,  17},  {  1,  18},  {  2,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  {  1,   2},
  {  1,   1},  {  1,   3},  {  1,  17},  {  1,   2},  {  5,   0},  {  1,   1},  {  1,  19},  {  1,   3},
  {  1,  17},  {  1,   2},  {  1,   0},
  {  4,   0},  {  1,   1},  {  2,   3},  {  1,  17},  {  1,   2},  {  5,   0},  {  1,   1},  {  2,   3},  /* Row 17 */
  {  1,  17},  {  1,   2},  {  5,   0},  {  1,   1},  {  1,  19},  {  1,   3},  {  1,   0},  {  1,   2},
  {  1,   1},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,   1},
  {  4,  18},  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,   1},  {  1,  18},
  {  3,  18},  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,   1},  {  4,  18},  /* Row 18 */
  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,   1},  {  4,  18},  {  1,   1},
  {  1,  19},  {  1,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  {  1,   2},  {  1,   1},
  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  3,   3},  {  1,  20},  {  1,   1},  {  1,  19},
  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18}, {  2,   3},
  {  1,   3},  {  1,  20},  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  /* Row 19 */
  {  3,   3},  {  1,  20},  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},
  {  3,   3},  {  1,  20},  {  1,   1},  {  1,  19},  {  1,   3},  {  1,   0},  {  1,   2},  {  1,   1},
  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,   1},  {  2,   3},
  {  2,  20},  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  2,   3},
  {  1,  20},
  {  1,  20},  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  2,   3},  /* Row 20 */
  {  2,  20},  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  2,   3},
  {  2,  20},  {  1,   1},  {  1,  19},  {  1,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},
  {  1,   2},  {  1,   1},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,   1},  {  1,   3},  {  3,  20},
  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  1,   3},  {  3,  20},
  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  1,   3},  {  3,  20},  /* Row 21 */
  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  1,   3},  {  3,  20},
  {  1,   1},  {  1,  19},  {  1,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  {  1,   2},
  {  1,   1},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,   1},  {  3,  20},  {  2,   1},  {  1,  19},
  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  3,  20},  {  2,   1},
  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  3,  20},  {  2,   1},  {  1,  19},  /* Row 22 */
  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  3,  20},  {  2,   1},  {  1,  19},  {  1,   3},
  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   3},  {  1,  18},
  {  6,   1},  {  1,   3},  {  1,  19},  {  1,   3},  {  1,  18},  {  6,   1},  {  1,   3},  {  1,  19},
  {  1,   3},  {  1,  18},  {  6,   1},  {  1,   3},  {  1,  19},  {  1,   3},  {  1,  18},  {  6,   1},  /* Row 23 */
  {  1,   3},  {  1,  19},  {  1,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  {  1,   2},
  {  1,   1},  {  2,   3},  {  8,  19},  {  2,   3},  {  8,  19},  {  1,   3},
  {  1,   3},  {  8,  19},  {  2,   3},  {  8,  19},  {  1,   3},  {  1,   0},  {  1,   2},  {  1,   1},  /* Row 24 */
  {  1,   0},  {  1,   2},  {  1,   1},  { 22,   3},
  { 19,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   3},  /* Row 25 */
  {  1,   1},  {  6,  17},  {  1,  18},  {  2,   3},  {  1,   1},  {  6,  17},  {  1,  18},  {  2,   3},
  {  1,   1},  {  1,  17},
  {  5,  17},  {  1,  18},  {  2,   3},  {  1,   1},  {  6,  17},  {  1,  18},  {  2,   3},  {  1,   0},  /* Row 26 */
  {  1,   2},  {  1,   1},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   3},  {  1,  17},  {  1,   2},
  {  5,   0},  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   2},  {  5,   0},  {  1,   1},
  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   2},  {  1,   0},
  {  4,   0},  {  1,   1},  {  2,   3},  {  1,  17},  {  1,   2},  {  5,   0},  {  1,   1},  {  1,  19},  /* Row 27 */
  {  1,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   3},
  {  1,  17},  {  1,   0},  {  1,   1},  {  4,  18},  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},
  {  1,   0},  {  1,   1},  {  4,  18},  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},
  {  1,   1},  {  1,  18},
  {  3,  18},  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,   1},  {  4,  18},  /* Row 28 */
  {  1,   1},  {  1,  19},  {  1,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  {  1,   2},
  {  1,   1},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  3,   3},  {  1,  20},  {  1,   1},
  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  3,   3},  {  1,  20},  {  1,   1},
  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  2,   3},
  {  1,   3},  {  1,  20},  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  /* Row 29 */
  {  3,   3},  {  1,  20},  {  1,   1},  {  1,  19},  {  1,   3},  {  1,   0},  {  1,   2},  {  1,   1},
  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  2,   3},
  {  2,  20},  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  2,   3},
  {  2,  20},  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  2,   3},
  {  1,  20},
  {  1,  20},  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  2,   3},  /* Row 30 */
  {  2,  20},  {  1,   1},  {  1,  19},  {  1,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},
  {  1,   2},  {  1,   1},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  1,   3},  {  3,  20},
  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  1,   3},  {  3,  20},
  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  1,   3},  {  3,  20},
  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  1,   3},  {  3,  20},  /* Row 31 */
  {  1,   1},  {  1,  19},  {  1,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  {  1,   2},
  {  1,   1},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  3,  20},  {  2,   1},  {  1,  19},
  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  3,  20},  {  2,   1},  {  1,  19},  {  1,   3},
  {  1,  17},  {  1,   0},  {  1,  18},  {  3,  20},  {  2,   1},
  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  3,  20},  {  2,   1},  {  1,  19},  /* Row 32 */
  {  1,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   3},
  {  1,  18},  {  6,   1},  {  1,   3},  {  1,  19},  {  1,   3},  {  1,  18},  {  6,   1},  {  1,   3},
  {  1,  19},  {  1,   3},  {  1,  18},  {  6,   1},  {  1,   3},  {  1,  19},
  {  1,   3},  {  1,  18},  {  6,   1},  {  1,   3},  {  1,  19},  {  1,   3},  {  1,   0},  {  1,   2},  /* Row 33 */
  {  1,   1},  {  1,   0},  {  1,   2},  {  1,   1},  {  2,   3},  {  8,  19},  {  2,   3},  {  8,  19},
  {  3,   3},  {  7,  19},  {  1,   3},
  {  1,   3},  {  8,  19},  {  1,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  {  1,   2},  /* Row 34 */
  {  1,   1},  { 32,   3},
  {  9,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   3},  /* Row 35 */
  {  1,   1},  {  6,  17},  {  1,  18},  {  2,   3},  {  1,   1},  {  6,  17},  {  1,  18},  {  2,   3},
  {  1,   1},  {  6,  17},  {  1,  18},  {  2,   3},  {  1,   1},  {  1,  17},
  {  5,  17},  {  1,  18},  {  2,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  {  1,   2},  /* Row 36 */
  {  1,   1},  {  1,   3},  {  1,  17},  {  1,   2},  {  5,   0},  {  1,   1},  {  1,  19},  {  1,   3},
  {  1,  17},  {  1,   2},  {  5,   0},  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   2},
  {  5,   0},  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   2},  {  1,   0},
  {  4,   0},  {  1,   1},  {  1,  19},  {  1,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  /* Row 37 */
  {  1,   2},  {  1,   1},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,   1},  {  4,  18},  {  1,   1},
  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,   1},  {  4,  18},  {  1,   1},  {  1,  19},
  {  1,   3},  {  1,  17},  {  1,   0},  {  1,   1},  {  4,  18},  {  1,   1},  {  1,  19},  {  1,   3},
  {  1,  17},  {  1,   0},  {  1,   1},  {  1,  18},
  {  3,  18},  {  1,   1},  {  1,  19},  {  1,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  /* Row 38 */
  {  1,   2},  {  1,   1},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  3,   3},  {  1,  20},
  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  3,   3},  {  1,  20},
  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  3,   3},  {  1,  20},
  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  2,   3},
  {  1,   3},  {  1,  20},  {  1,   1},  {  1,  19},  {  1,   3},  {  1,   0},  {  1,   2},  {  1,   1},  /* Row 39 */
  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  2,   3},
  {  2,  20},  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  2,   3},
  {  2,  20},  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  2,   3},
  {  2,  20},  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  2,   3},
  {  1,  20},
  {  1,  20},  {  1,   1},  {  1,  19},  {  1,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  /* Row 40 */
  {  1,   2},  {  1,   1},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  1,   3},  {  3,  20},
  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  1,   3},  {  3,  20},
  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  1,   3},  {  3,  20},
  {  1,   1},  {  1,  19},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  1,   3},  {  3,  20},
  {  1,   1},  {  1,  19},  {  1,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  {  1,   2},  /* Row 41 */
  {  1,   1},  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  3,  20},  {  2,   1},  {  1,  19},
  {  1,   3},  {  1,  17},  {  1,   0},  {  1,  18},  {  3,  20},  {  2,   1},  {  1,  19},  {  1,   3},
  {  1,  17},  {  1,   0},  {  1,  18},  {  3,  20},  {  2,   1},  {  1,  19},  {  1,   3},  {  1,  17},
  {  1,   0},  {  1,  18},  {  3,  20},  {  2,   1},
  {  1,  19},  {  1,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  {  1,   2},  {  1,   1},  /* Row 42 */
  {  1,   3},  {  1,  18},  {  6,   1},  {  1,   3},  {  1,  19},  {  1,   3},  {  1,  18},  {  6,   1},
  {  1,   3},  {  1,  19},  {  1,   3},  {  1,  18},  {  6,   1},  {  1,   3},  {  1,  19},  {  1,   3},
  {  1,  18},  {  6,   1},  {  1,   3},  {  1,  19},
  {  1,   3},  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  {  1,   2},  {  1,   1},  {  2,   3},  /* Row 43 */
  {  8,  19},  {  2,   3},  {  8,  19},  {  2,   3},  {  8,  19},  {  2,   3},  {  8,  19},  {  1,   3},
  {  1,   0},  {  1,   2},  {  1,   1},  {  1,   0},  {  1,   2},  {  1,   1},  { 41,   3},  {  1,   0},  /* Row 44 */
  {  1,   2},  {  1,   1},  {  1,   0},  {  1,   2},  { 43,   0},  {  1,   2},                            /* Row 45 */
  {  1,   1},  {  1,   0},  { 45,   2},  {  1,   1},                                                      /* Row 46 */
  { 47,   1},  {  1,  21}                                                                                 /* Row 47 */
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
    g_calculatorBrightlLut, //          Index 1: Selected LUT
  },
  g_calculatorRleEntries    // data   - Pointer to the beginning of the RLE data
};
