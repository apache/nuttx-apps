/********************************************************************************************
 * NxWidgets/nxwm/src/mediagrip60x30.cxx
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

#define BITMAP_NROWS         30
#define BITMAP_NCOLUMNS      60
#define BITMAP_NLUTCODES     4

#define MEDIA_GRIP_DARK_ICON 1

/********************************************************************************************
 * Private Bitmap Data
 ********************************************************************************************/

using namespace NxWM;

/* RGB24 (8-8-8) Colors */

#if CONFIG_NXWIDGETS_BPP == 24 ||  CONFIG_NXWIDGETS_BPP == 32
#  ifdef MEDIA_GRIP_DARK_ICON

static const uint32_t g_gripNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR, /* Code 0 */
  0x3663ab, 0x002199, 0xbdbdbd         /* Codes 0-3 */
};

static const uint32_t g_gripBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR, /* Code 0 */
  0x4884e4, 0x002ccc, 0xfcfcfc         /* Codes 0-3 */
};

#  else /* MEDIA_GRIP_DARK_ICON */

static const uint32_t g_gripNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR, /* Code 0 */
  0x4884e4, 0x002ccc, 0xfcfcfc         /* Codes 0-3 */
};

static const uint32_t g_gripBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR, /* Code 0 */
  0x5aa5ff, 0x0037ff, 0xffffff         /* Codes 0-3 */
};
#  endif /* MEDIA_GRIP_DARK_ICON */

/* RGB16 (565) Colors */

#elif CONFIG_NXWIDGETS_BPP == 16
#  ifdef MEDIA_GRIP_DARK_ICON

static const uint16_t g_gripNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR, /* Code 0 */
   0x3315, 0x0113, 0xbdf7              /* Codes 0-3 */
};

static const uint16_t g_gripBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR, /* Code 0 */
   0x4c3c, 0x0179, 0xffff              /* Codes 0-3 */
};

#  else /* MEDIA_GRIP_DARK_ICON */

static const uint16_t g_gripNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR, /* Code 0 */
   0x4c3c, 0x0179, 0xffff              /* Codes 0-3 */
};

static const uint16_t g_gripBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR, /* Code 0 */
   0x5d3f, 0x01bf, 0xffff              /* Codes 0-3 */
};

#  endif /* MEDIA_GRIP_DARK_ICON */

/* 8-bit color lookups.  NOTE:  This is really dumb!  The lookup index is 8-bits and it used
 * to lookup an 8-bit value.  There is no savings in that!  It would be better to just put
 * the 8-bit color/greyscale value in the run-length encoded image and save the cost of these
 * pointless lookups.  But these pointless lookups do make the logic compatible with the
 * 16- and 24-bit types.
 */

#elif CONFIG_NXWIDGETS_BPP == 8
#  ifdef CONFIG_NXWIDGETS_GREYSCALE

/* 8-bit Greyscale */

#    ifdef MEDIA_GRIP_DARK_ICON

static const uint8_t g_gripNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR, /* Code 0 */
  0x5d, 0x24, 0xbd                      /* Codes 0-3 */
};

static const uint8_t g_gripBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR, /* Code 0 */
  0x7d, 0x31, 0xfc                     /* Codes 0-3 */
};

#    else /* MEDIA_GRIP_DARK_ICON */

static const uint8_t g_gripNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR, /* Code 0 */
  0x7d, 0x31, 0xfc                     /* Codes 0-3 */
};

static const uint8_t g_gripBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR, /* Code 0 */
  0x98, 0x3d, 0xff                     /* Codes 0-3 */
};

#    endif /* MEDIA_GRIP_DARK_ICON */

#  else /* CONFIG_NXWIDGETS_GREYSCALE */

/* RGB8 (332) Colors */

#    ifdef MEDIA_GRIP_DARK_ICON

static const nxgl_mxpixel_t g_gripNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR, /* Code 0 */
  0x2e, 0x06, 0xb6                     /* Codes 0-3 */
};

static const nxgl_mxpixel_t g_gripBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR, /* Code 0 */
  0x53, 0x07, 0xff                     /* Codes 0-3 */
};

#    else /* MEDIA_GRIP_DARK_ICON */

static const nxgl_mxpixel_t g_gripNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR, /* Code 0 */
  0x53, 0x07, 0xff                     /* Codes 0-3 */
};

static const nxgl_mxpixel_t g_gripBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR, /* Code 0 */
  0x57, 0x07, 0xff                     /* Codes 0-3 */
};

#    endif /* MEDIA_GRIP_DARK_ICON */
#  endif /* CONFIG_NXWIDGETS_GREYSCALE */
#else
# error Unsupported pixel format
#endif

static const struct NXWidgets::SRlePaletteBitmapEntry g_gripRleEntries[] =
{
  { 1, 0}, { 1, 1}, {56, 2}, { 1, 1}, { 1, 0},                             /* Row 0 */
  { 1, 1}, {58, 2}, { 1, 1},                                               /* Row 1 */
  {60, 2},                                                                 /* Row 2 */
  {60, 2},                                                                 /* Row 3 */
  {60, 2},                                                                 /* Row 4 */
  {60, 2},                                                                 /* Row 5 */
  {42, 2}, { 1, 1}, { 4, 3}, { 1, 1}, {12, 2},                             /* Row 6 */
  {42, 2}, { 6, 3}, {12, 2},                                               /* Row 7 */
  {42, 2}, { 6, 3}, {12, 2},                                               /* Row 8 */
  {42, 2}, { 6, 3}, {12, 2},                                               /* Row 9 */
  {42, 2}, { 6, 3}, {12, 2},                                               /* Row 10 */
  {42, 2}, { 6, 3}, {12, 2},                                               /* Row 11 */
  { 6, 2}, { 1, 1}, {16, 3}, { 1, 1}, {12, 2}, { 1, 1}, {16, 3}, { 1, 1},  /* Row 12 */
  { 6, 2},
  { 6, 2}, {18, 3}, {12, 2}, {18, 3}, { 6, 2},                             /* Row 13 */
  { 6, 2}, {18, 3}, {12, 2}, {18, 3}, { 6, 2},                             /* Row 14 */
  { 6, 2}, {18, 3}, {12, 2}, {18, 3}, { 6, 2},                             /* Row 15 */
  { 6, 2}, {18, 3}, {12, 2}, {18, 3}, { 6, 2},                             /* Row 16 */
  { 6, 2}, { 1, 1}, {16, 3}, { 1, 1}, {12, 2}, { 1, 1}, {16, 3}, { 1, 1},  /* Row 17 */
  { 6, 2},
  {42, 2}, { 6, 3}, {12, 2},                                               /* Row 18 */
  {42, 2}, { 6, 3}, {12, 2},                                               /* Row 19 */
  {42, 2}, { 6, 3}, {12, 2},                                               /* Row 20 */
  {42, 2}, { 6, 3}, {12, 2},                                               /* Row 21 */
  {42, 2}, { 6, 3}, {12, 2},                                               /* Row 22 */
  {42, 2}, { 1, 1}, { 4, 3}, { 1, 1}, {12, 2},                             /* Row 23 */
  {60, 2},                                                                 /* Row 24 */
  {60, 2},                                                                 /* Row 25 */
  {60, 2},                                                                 /* Row 26 */
  {60, 2},                                                                 /* Row 27 */
  { 1, 1}, {58, 2}, { 1, 1},                                               /* Row 28 */
  { 1, 0}, { 1, 1}, {56, 2}, { 1, 1}, { 1, 0}                              /* Row 29 */
};

/********************************************************************************************
 * Public Bitmap Structure Definitions
 ********************************************************************************************/

const struct NXWidgets::SRlePaletteBitmap NxWM::g_mplayerVolBitmap =
{
  CONFIG_NXWIDGETS_BPP,  // bpp    - Bits per pixel
  CONFIG_NXWIDGETS_FMT,  // fmt    - Color format
  BITMAP_NLUTCODES,      // nlut   - Number of colors in the lLook-Up Table (LUT)
  BITMAP_NCOLUMNS,       // width  - Width in pixels
  BITMAP_NROWS,          // height - Height in rows
  {                      // lut    - Pointer to the beginning of the Look-Up Table (LUT)
    g_gripNormalLut,     //          Index 0: Unselected LUT
    g_gripBrightLut,     //          Index 1: Selected LUT
  },
  g_gripRleEntries       // data   - Pointer to the beginning of the RLE data
};
