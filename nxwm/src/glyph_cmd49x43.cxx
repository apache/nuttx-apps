/********************************************************************************************
 * NxWidgets/nxwm/src/glyph_cmd49x43.cxx
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

#define BITMAP_NROWS     43
#define BITMAP_NCOLUMNS  49
#define BITMAP_NLUTCODES 10

#define DARK_CMD_ICON    1

/********************************************************************************************
 * Private Bitmap Data
 ********************************************************************************************/

using namespace NxWM;

/* RGB24 (8-8-8) Colors */

#if CONFIG_NXWIDGETS_BPP == 24 ||  CONFIG_NXWIDGETS_BPP == 32
#  ifdef DARK_CMD_ICON

static const uint32_t g_cmdNormalLut[BITMAP_NLUTCODES] =
{
  0x969696, 0x545454, 0x2a2a2a, 0x818181, 0x001581, 0xab0000, 0x0015ab, 0xbd0000,  /* Codes 0-7 */
  0xbdbdbd, 0x3f3f3f                                                               /* Codes 8-9 */
};

static const uint32_t g_cmdBrightLut[BITMAP_NLUTCODES] =
{
  0xc8c8c8, 0x707070, 0x383838, 0xacacac, 0x001cac, 0xe40000, 0x001ce4, 0xfc0000,  /* Codes 0-7 */
  0xfcfcfc, 0x545454                                                               /* Codes 8-9 */
};

#  else /* DARK_CMD_ICON */

static const uint32_t g_cmdNormalLut[BITMAP_NLUTCODES] =
{
  0xc8c8c8, 0x707070, 0x383838, 0xacacac, 0x001cac, 0xe40000, 0x001ce4, 0xfc0000,  /* Codes 0-7 */
  0xfcfcfc, 0x545454                                                               /* Codes 8-9 */
};

static const uint32_t g_cmdBrightLut[BITMAP_NLUTCODES] =
{
  0xfafafa, 0x8c8c8c, 0x464646, 0xd7d7d7, 0x0023d7, 0xff0000, 0x0023ff, 0xff0000,  /* Codes 0-7 */
  0xffffff, 0x696969                                                               /* Codes 8-9 */
};
#  endif /* DARK_CMD_ICON */

/* RGB16 (565) Colors */

#elif CONFIG_NXWIDGETS_BPP == 16
#  ifdef DARK_CMD_ICON

static const uint16_t g_cmdNormalLut[BITMAP_NLUTCODES] =
{
  0x94b2, 0x52aa, 0x2945, 0x8410, 0x00b0, 0xa800, 0x00b5, 0xb800, 0xbdf7, 0x39e7   /* Codes 0-9 */
};

static const uint16_t g_cmdBrightLut[BITMAP_NLUTCODES] =
{
  0xce59, 0x738e, 0x39c7, 0xad75, 0x00f5, 0xe000, 0x00fc, 0xf800, 0xffff, 0x52aa   /* Codes 0-9 */
};

#  else /* DARK_CMD_ICON */

static const uint16_t g_cmdNormalLut[BITMAP_NLUTCODES] =
{
  0xce59, 0x738e, 0x39c7, 0xad75, 0x00f5, 0xe000, 0x00fc, 0xf800, 0xffff, 0x52aa   /* Codes 0-9 */
};

static const uint16_t g_cmdBrightLut[BITMAP_NLUTCODES] =
{
  0xffdf, 0x8c71, 0x4228, 0xd6ba, 0x011a, 0xf800, 0x011f, 0xf800, 0xffff, 0x6b4d   /* Codes 0-9 */
};

#  endif /* DARK_CMD_ICON */

/* 8-bit color lookups.  NOTE:  This is really dumb!  The lookup index is 8-bits and it used
 * to lookup an 8-bit value.  There is no savings in that!  It would be better to just put
 * the 8-bit color/greyscale value in the run-length encoded image and save the cost of these
 * pointless lookups.  But these pointless lookups do make the logic compatible with the
 * 16- and 24-bit types.
 */

#elif CONFIG_NXWIDGETS_BPP == 8
#  ifdef CONFIG_NXWIDGETS_GREYSCALE

/* 8-bit Greyscale */

#    ifdef DARK_CMD_ICON

static const uint8_t g_cmdNormalLut[BITMAP_NLUTCODES] =
{
  0x96, 0x54, 0x2a, 0x81, 0x1b, 0x33, 0x1f, 0x38, 0xbd, 0x3f  /* Codes 0-9 */
};

static const uint8_t g_cmdBrightLut[BITMAP_NLUTCODES] =
{
  0xc8, 0x70, 0x38, 0xac, 0x24, 0x44, 0x2a, 0x4b, 0xfc, 0x54  /* Codes 0-9 */
};

#    else /* DARK_CMD_ICON */

static const uint8_t g_cmdNormalLut[BITMAP_NLUTCODES] =
{
  0xc8, 0x70, 0x38, 0xac, 0x24, 0x44, 0x2a, 0x4b, 0xfc, 0x54  /* Codes 0-9 */
};

static const uint8_t g_cmdBrightLut[BITMAP_NLUTCODES] =
{
  0xfa, 0x8c, 0x46, 0xd7, 0x2d, 0x4c, 0x31, 0x4c, 0xff, 0x69  /* Codes 0-9 */
};

#    endif /* DARK_CMD_ICON */

#  else /* CONFIG_NXWIDGETS_GREYSCALE */

/* RGB8 (332) Colors */

#    ifdef DARK_CMD_ICON

static const nxgl_mxpixel_t g_cmdNormalLut[BITMAP_NLUTCODES] =
{
  0x92, 0x49, 0x24, 0x92, 0x02, 0xa0, 0x02, 0xa0, 0xb6, 0x24  /* Codes 0-9 */
};

static const nxgl_mxpixel_t g_cmdBrightLut[BITMAP_NLUTCODES] =
{
  0xdb, 0x6d, 0x24, 0xb6, 0x02, 0xe0, 0x03, 0xe0, 0xff, 0x49  /* Codes 0-9 */
};

#    else /* DARK_CMD_ICON */

static const nxgl_mxpixel_t g_cmdNormalLut[BITMAP_NLUTCODES] =
{
  0xdb, 0x6d, 0x24, 0xb6, 0x02, 0xe0, 0x03, 0xe0, 0xff, 0x49  /* Codes 0-9 */
};

static const nxgl_mxpixel_t g_cmdBrightLut[BITMAP_NLUTCODES] =
{
  0xff, 0x92, 0x49, 0xdb, 0x07, 0xe0, 0x07, 0xe0, 0xff, 0x6d  /* Codes 0-9 */
};

#    endif /* DARK_CMD_ICON */
#  endif /* CONFIG_NXWIDGETS_GREYSCALE */
#else
# error Unsupported pixel format
#endif

static const struct NXWidgets::SRlePaletteBitmapEntry g_cmdRleEntries[] =
{
  {47,  0}, { 1,  1}, { 1,  2},                                                   /* Row 0 */
  { 1,  0}, {46,  3}, { 1,  1}, { 1,  2},                                         /* Row 1 */
  { 1,  0}, {46,  3}, { 1,  1}, { 1,  2},                                         /* Row 2 */
  { 1,  0}, {31,  3}, { 6,  4}, { 2,  3}, { 6,  5}, { 1,  3}, { 1,  1}, { 1,  2}, /* Row 3 */
  { 1,  0}, {31,  3}, { 1,  4}, { 3,  6}, { 2,  4}, { 2,  3}, { 1,  5}, { 3,  7}, /* Row 4 */
  { 2,  5}, { 1,  3}, { 1,  1}, { 1,  2},
  { 1,  0}, {31,  3}, { 1,  4}, { 2,  6}, { 3,  4}, { 2,  3}, { 1,  5}, { 2,  7}, /* Row 5 */
  { 3,  5}, { 1,  3}, { 1,  1}, { 1,  2},
  { 1,  0}, {31,  3}, { 1,  4}, { 1,  6}, { 4,  4}, { 2,  3}, { 1,  5}, { 1,  7}, /* Row 6 */
  { 4,  5}, { 1,  3}, { 1,  1}, { 1,  2},
  { 1,  0}, {31,  3}, { 6,  4}, { 2,  3}, { 6,  5}, { 1,  3}, { 1,  1}, { 1,  2}, /* Row 7 */
  { 1,  0}, {31,  3}, { 6,  4}, { 2,  3}, { 6,  5}, { 1,  3}, { 1,  1}, { 1,  2}, /* Row 8 */
  { 1,  0}, {46,  3}, { 1,  1}, { 1,  2},                                         /* Row 9 */
  { 1,  0}, {46,  3}, { 1,  1}, { 1,  2},                                         /* Row 10 */
  { 1,  0}, { 1,  3}, {44,  1}, { 1,  3}, { 1,  1}, { 1,  2},                     /* Row 11 */
  { 1,  0}, { 1,  3}, { 1,  1}, {43,  2}, { 1,  3}, { 1,  1}, { 1,  2},           /* Row 12 */
  { 1,  0}, { 1,  3}, { 1,  1}, { 1,  2}, {42,  1}, { 1,  3}, { 1,  1}, { 1,  2}, /* Row 13 */
  { 1,  0}, { 1,  3}, { 1,  1}, { 1,  2}, { 1,  1}, { 1,  8}, {40,  1}, { 1,  3}, /* Row 14 */
  { 1,  1}, { 1,  2},
  { 1,  0}, { 1,  3}, { 1,  1}, { 1,  2}, { 2,  1}, { 1,  8}, {39,  1}, { 1,  3}, /* Row 15 */
  { 1,  1}, { 1,  2},
  { 1,  0}, { 1,  3}, { 1,  1}, { 1,  2}, { 3,  1}, { 1,  8}, {38,  1}, { 1,  3}, /* Row 16 */
  { 1,  1}, { 1,  2},
  { 1,  0}, { 1,  3}, { 1,  1}, { 1,  2}, { 4,  1}, { 1,  8}, {37,  1}, { 1,  3}, /* Row 17 */
  { 1,  1}, { 1,  2},
  { 1,  0}, { 1,  3}, { 1,  1}, { 1,  2}, { 5,  1}, { 1,  8}, {36,  1}, { 1,  3}, /* Row 18 */
  { 1,  1}, { 1,  2},
  { 1,  0}, { 1,  3}, { 1,  1}, { 1,  2}, { 4,  1}, { 1,  8}, { 1,  9}, {36,  1}, /* Row 19 */
  { 1,  3}, { 1,  1}, { 1,  2},
  { 1,  0}, { 1,  3}, { 1,  1}, { 1,  2}, { 3,  1}, { 1,  8}, { 1,  9}, {37,  1}, /* Row 20 */
  { 1,  3}, { 1,  1}, { 1,  2},
  { 1,  0}, { 1,  3}, { 1,  1}, { 1,  2}, { 2,  1}, { 1,  8}, { 1,  9}, { 4,  1}, /* Row 21 */
  { 9,  8}, {25,  1}, { 1,  3}, { 1,  1}, { 1,  2},
  { 1,  0}, { 1,  3}, { 1,  1}, { 1,  2}, { 1,  1}, { 1,  8}, { 1,  9}, { 6,  1}, /* Row 22 */
  { 9,  9}, {24,  1}, { 1,  3}, { 1,  1}, { 1,  2},
  { 1,  0}, { 1,  3}, { 1,  1}, { 1,  2}, { 1,  1}, { 1,  9}, {40,  1}, { 1,  3}, /* Row 23 */
  { 1,  1}, { 1,  2},
  { 1,  0}, { 1,  3}, { 1,  1}, { 1,  2}, {42,  1}, { 1,  3}, { 1,  1}, { 1,  2}, /* Row 24 */
  { 1,  0}, { 1,  3}, { 1,  1}, { 1,  2}, {42,  1}, { 1,  3}, { 1,  1}, { 1,  2}, /* Row 25 */
  { 1,  0}, { 1,  3}, { 1,  1}, { 1,  2}, {42,  1}, { 1,  3}, { 1,  1}, { 1,  2}, /* Row 26 */
  { 1,  0}, { 1,  3}, { 1,  1}, { 1,  2}, {42,  1}, { 1,  3}, { 1,  1}, { 1,  2}, /* Row 27 */
  { 1,  0}, { 1,  3}, { 1,  1}, { 1,  2}, {42,  1}, { 1,  3}, { 1,  1}, { 1,  2}, /* Row 28 */
  { 1,  0}, { 1,  3}, { 1,  1}, { 1,  2}, {42,  1}, { 1,  3}, { 1,  1}, { 1,  2}, /* Row 29 */
  { 1,  0}, { 1,  3}, { 1,  1}, { 1,  2}, {42,  1}, { 1,  3}, { 1,  1}, { 1,  2}, /* Row 30 */
  { 1,  0}, { 1,  3}, { 1,  1}, { 1,  2}, {42,  1}, { 1,  3}, { 1,  1}, { 1,  2}, /* Row 31 */
  { 1,  0}, { 1,  3}, { 1,  1}, { 1,  2}, {42,  1}, { 1,  3}, { 1,  1}, { 1,  2}, /* Row 32 */
  { 1,  0}, { 1,  3}, { 1,  1}, { 1,  2}, {42,  1}, { 1,  3}, { 1,  1}, { 1,  2}, /* Row 33 */
  { 1,  0}, { 1,  3}, { 1,  1}, { 1,  2}, {42,  1}, { 1,  3}, { 1,  1}, { 1,  2}, /* Row 34 */
  { 1,  0}, { 1,  3}, { 1,  1}, { 1,  2}, {42,  1}, { 1,  3}, { 1,  1}, { 1,  2}, /* Row 35 */
  { 1,  0}, { 1,  3}, { 1,  1}, { 1,  2}, {42,  1}, { 1,  3}, { 1,  1}, { 1,  2}, /* Row 36 */
  { 1,  0}, { 1,  3}, { 1,  1}, { 1,  2}, {42,  1}, { 1,  3}, { 1,  1}, { 1,  2}, /* Row 37 */
  { 1,  0}, { 1,  3}, { 1,  1}, { 1,  2}, {42,  1}, { 1,  3}, { 1,  1}, { 1,  2}, /* Row 38 */
  { 1,  0}, { 1,  3}, {44,  1}, { 1,  3}, { 1,  1}, { 1,  2},                     /* Row 39 */
  { 1,  0}, {46,  3}, { 1,  1}, { 1,  2},                                         /* Row 40 */
  {48,  1}, { 1,  2},                                                             /* Row 41 */
  {49,  2}                                                                        /* Row 42 */
};

/********************************************************************************************
 * Public Bitmap Structure Definitions
 ********************************************************************************************/

const struct NXWidgets::SRlePaletteBitmap NxWM::g_cmdBitmap =
{
  CONFIG_NXWIDGETS_BPP, // bpp    - Bits per pixel
  CONFIG_NXWIDGETS_FMT, // fmt    - Color format
  BITMAP_NLUTCODES,     // nlut   - Number of colors in the lLook-Up Table (LUT)
  BITMAP_NCOLUMNS,      // width  - Width in pixels
  BITMAP_NROWS,         // height - Height in rows
  {                     // lut    - Pointer to the beginning of the Look-Up Table (LUT)
    g_cmdNormalLut,     //          Index 0: Unselected LUT
    g_cmdBrightLut,     //          Index 1: Selected LUT
  },
  g_cmdRleEntries       // data   - Pointer to the beginning of the RLE data
};
