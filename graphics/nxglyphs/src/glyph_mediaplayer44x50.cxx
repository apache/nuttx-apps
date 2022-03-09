/********************************************************************************************
 * apps/graphics/nxglyphs/src/glyph_mediaplayer44x50.cxx
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
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

#define BITMAP_NROWS          48
#define BITMAP_NCOLUMNS       44
#define BITMAP_NLUTCODES      5

#define DARK_MEDIAPLAYER_ICON 1

/********************************************************************************************
 * Private Bitmap Data
 ********************************************************************************************/

using namespace NXWidgets;

/* RGB24 (8-8-8) Colors */

#if CONFIG_NXWIDGETS_BPP == 24 ||  CONFIG_NXWIDGETS_BPP == 32
#  ifdef DARK_MEDIAPLAYER_ICON

static const uint32_t g_mediaplayerNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,        /* Code 0 */
  0x002199, 0xbdbdbd, 0x4e8199, 0x276099  /* Codes 1-4 */
};

static const uint32_t g_mediaplayerBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,        /* Code 0 */
  0x002ccc, 0xfcfcfc, 0x68accc, 0x3480cc  /* Codes 1-4 */
};

#  else /* DARK_MEDIAPLAYER_ICON */

static const uint32_t g_mediaplayerNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,        /* Code 0 */
  0x002ccc, 0xfcfcfc, 0x68accc, 0x3480cc  /* Codes 1-4 */
};

static const uint32_t g_mediaplayerBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,        /* Code 0 */
  0x0037ff, 0xffffff, 0x82d7ff, 0x41a0ff  /* Codes 1-4 */
};
#  endif /* DARK_MEDIAPLAYER_ICON */

/* RGB16 (565) Colors */

#elif CONFIG_NXWIDGETS_BPP == 16
#  ifdef DARK_MEDIAPLAYER_ICON

static const uint16_t g_mediaplayerNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,     /* Code 0 */
  0x0113, 0xbdf7, 0x4c13, 0x2313       /* Codes 1-4 */
};

static const uint16_t g_mediaplayerBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,     /* Code 0 */
  0x0179, 0xffff, 0x6d79, 0x3419       /* Codes 1-4 */
};

#  else /* DARK_MEDIAPLAYER_ICON */

static const uint16_t g_mediaplayerNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,     /* Code 0 */
  0x0179, 0xffff, 0x6d79, 0x3419       /* Codes 1-4 */
};

static const uint16_t g_mediaplayerBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,     /* Code 0 */
  0x01bf, 0xffff, 0x86bf, 0x451f       /* Codes 1-4 */
};

#  endif /* DARK_MEDIAPLAYER_ICON */

/* 8-bit color lookups.  NOTE:  This is really dumb!  The lookup index is 8-bits and it used
 * to lookup an 8-bit value.  There is no savings in that!  It would be better to just put
 * the 8-bit color/greyscale value in the run-length encoded image and save the cost of these
 * pointless lookups.  But these pointless lookups do make the logic compatible with the
 * 16- and 24-bit types.
 */

#elif CONFIG_NXWIDGETS_BPP == 8
#  ifdef CONFIG_NXWIDGETS_GREYSCALE

/* 8-bit Greyscale */

#    ifdef DARK_MEDIAPLAYER_ICON

static const uint8_t g_mediaplayerNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,      /* Code 0 */
  0x24, 0xbd, 0x74, 0x55                /* Codes 1-4 */
};

static const uint8_t g_mediaplayerBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,      /* Code 0 */
  0x31, 0xfc, 0x9b, 0x71                /* Codes 1-4 */
};

#    else /* DARK_MEDIAPLAYER_ICON */

static const uint8_t g_mediaplayerNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,      /* Code 0 */
  0x31, 0xfc, 0x9b, 0x71                /* Codes 1-4 */
};

static const uint8_t g_mediaplayerBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,      /* Code 0 */
  0x3d, 0xff, 0xc2, 0x8e                /* Codes 1-4 */
};

#    endif /* DARK_MEDIAPLAYER_ICON */

#  else /* CONFIG_NXWIDGETS_GREYSCALE */

/* RGB8 (332) Colors */

#    ifdef DARK_MEDIAPLAYER_ICON

static const nxgl_mxpixel_t g_mediaplayerNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,       /* Code 0 */
  0x06, 0xb6, 0x52, 0x2e,                /* Codes 1-4 */
};

static const nxgl_mxpixel_t g_mediaplayerBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,       /* Code 0 */
  0x07, 0xff, 0x77, 0x33,                /* Codes 1-4 */
};

#    else /* DARK_MEDIAPLAYER_ICON */

static const nxgl_mxpixel_t g_mediaplayerNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,       /* Code 0 */
  0x07, 0xff, 0x77, 0x33,                /* Codes 1-4 */
};

static const nxgl_mxpixel_t g_mediaplayerBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,       /* Code 0 */
  0x07, 0xff, 0x9b, 0x57,                /* Codes 1-4 */
};

#    endif /* DARK_MEDIAPLAYER_ICON */
#  endif /* CONFIG_NXWIDGETS_GREYSCALE */
#else
# error Unsupported pixel format
#endif

static const struct SRlePaletteBitmapEntry g_mediaplayerRleEntries[] =
{
  {44, 0},                                                                 /* Row 0 */
  {44, 0},                                                                 /* Row 1 */
  {38, 0}, { 6, 1},                                                        /* Row 2 */
  {34, 0}, {10, 1},                                                        /* Row 3 */
  {30, 0}, {14, 1},                                                        /* Row 4 */
  {26, 0}, {18, 1},                                                        /* Row 5 */
  {22, 0}, {17, 1}, { 2, 2}, { 3, 1},                                      /* Row 6 */
  {22, 0}, {13, 1}, { 6, 2}, { 3, 1},                                      /* Row 7 */
  {18, 0}, {12, 1}, {11, 2}, { 3, 1},                                      /* Row 8 */
  {14, 0}, {12, 1}, {15, 2}, { 3, 1},                                      /* Row 9 */
  {14, 0}, { 9, 1}, {18, 2}, { 3, 1},                                      /* Row 10 */
  {14, 0}, { 4, 1}, {23, 2}, { 3, 1},                                      /* Row 11 */
  {14, 0}, { 3, 1}, {21, 2}, { 1, 1}, { 2, 2}, { 3, 1},                    /* Row 12 */
  {14, 0}, { 3, 1}, {17, 2}, { 5, 1}, { 2, 2}, { 3, 1},                    /* Row 13 */
  {14, 0}, { 3, 1}, {13, 2}, { 9, 1}, { 2, 2}, { 3, 1},                    /* Row 14 */
  {14, 0}, { 3, 1}, { 9, 2}, {13, 1}, { 2, 2}, { 3, 1},                    /* Row 15 */
  {14, 0}, { 3, 1}, { 5, 2}, {17, 1}, { 2, 2}, { 3, 1},                    /* Row 16 */
  {14, 0}, { 3, 1}, { 2, 2}, {13, 1}, { 4, 0}, { 3, 1}, { 2, 2}, { 3, 1},  /* Row 17 */
  {14, 0}, { 3, 1}, { 2, 2}, { 9, 1}, { 8, 0}, { 3, 1}, { 2, 2}, { 3, 1},  /* Row 18 */
  {14, 0}, { 3, 1}, { 2, 2}, { 6, 1}, {11, 0}, { 3, 1}, { 2, 2}, { 3, 1},  /* Row 19 */
  {14, 0}, { 3, 1}, { 2, 2}, { 3, 1}, {14, 0}, { 3, 1}, { 2, 2}, { 3, 1},  /* Row 20 */
  {14, 0}, { 3, 1}, { 2, 2}, { 3, 1}, {14, 0}, { 3, 1}, { 2, 2}, { 3, 1},  /* Row 21 */
  {14, 0}, { 3, 1}, { 2, 2}, { 3, 1}, {14, 0}, { 3, 1}, { 2, 2}, { 3, 1},  /* Row 22 */
  {14, 0}, { 3, 1}, { 2, 2}, { 3, 1}, {14, 0}, { 3, 1}, { 2, 2}, { 3, 1},  /* Row 23 */
  {14, 0}, { 3, 1}, { 2, 2}, { 3, 1}, {14, 0}, { 3, 1}, { 2, 2}, { 3, 1},  /* Row 24 */
  {14, 0}, { 3, 1}, { 2, 2}, { 3, 1}, {14, 0}, { 3, 1}, { 2, 2}, { 3, 1},  /* Row 25 */
  {14, 0}, { 3, 1}, { 2, 2}, { 3, 1}, {14, 0}, { 3, 1}, { 2, 2}, { 3, 1},  /* Row 26 */
  {14, 0}, { 3, 1}, { 2, 2}, { 3, 1}, { 9, 0}, { 1, 3}, { 2, 4}, { 1, 3},  /* Row 27 */
  { 1, 0}, { 3, 1}, { 2, 2}, { 3, 1},
  {14, 0}, { 3, 1}, { 2, 2}, { 3, 1}, { 6, 0}, { 2, 3}, { 9, 1}, { 2, 2},  /* Row 28 */
  { 3, 1},
  {14, 0}, { 3, 1}, { 2, 2}, { 3, 1}, { 5, 0}, { 1, 4}, {11, 1}, { 2, 2},  /* Row 29 */
  { 3, 1},
  {14, 0}, { 3, 1}, { 2, 2}, { 3, 1}, { 5, 0}, { 7, 1}, { 1, 4}, { 4, 1},  /* Row 30 */
  { 2, 2}, { 3, 1},
  { 9, 0}, { 1, 3}, { 2, 4}, { 1, 3}, { 1, 0}, { 3, 1}, { 2, 2}, { 3, 1},  /* Row 31 */
  { 4, 0}, { 3, 1}, { 1, 4}, { 1, 1}, { 5, 2}, { 2, 4}, { 1, 1}, { 2, 2},
  { 3, 1},
  { 6, 0}, { 2, 4}, { 9, 1}, { 2, 2}, { 3, 1}, { 3, 0}, { 1, 4}, { 2, 1},  /* Row 32 */
  { 2, 4}, {11, 2}, { 3, 1},
  { 5, 0}, { 1, 4}, {11, 1}, { 2, 2}, { 3, 1}, { 3, 0}, { 3, 1}, {13, 2},  /* Row 33 */
  { 3, 1},
  { 5, 0}, {12, 1}, { 2, 2}, { 3, 1}, { 3, 0}, { 3, 1}, {13, 2}, { 3, 1},  /* Row 34 */
  { 4, 0}, { 3, 1}, { 1, 4}, { 1, 1}, { 5, 2}, { 3, 1}, { 2, 2}, { 3, 1},  /* Row 35 */
  { 2, 0}, { 1, 3}, { 2, 1}, {14, 2}, { 3, 1},
  { 3, 0}, { 1, 4}, { 2, 1}, { 2, 3}, {11, 2}, { 3, 1}, { 2, 0}, { 1, 4},  /* Row 36 */
  { 2, 1}, {14, 2}, { 3, 1},
  { 3, 0}, { 3, 1}, {13, 2}, { 3, 1}, { 3, 0}, { 3, 1}, {13, 2}, { 3, 1},  /* Row 37 */
  { 2, 0}, { 1, 3}, { 2, 1}, {14, 2}, { 3, 1}, { 3, 0}, { 3, 1}, {13, 2},  /* Row 38 */
  { 3, 1},
  { 2, 0}, { 1, 3}, { 2, 1}, {14, 2}, { 3, 1}, { 3, 0}, { 3, 1}, {12, 2},  /* Row 39 */
  { 1, 4}, { 3, 1},
  { 2, 0}, { 1, 4}, { 2, 1}, {14, 2}, { 3, 1}, { 4, 0}, { 4, 1}, {10, 2},  /* Row 40 */
  { 3, 1}, { 1, 3},
  { 3, 0}, { 3, 1}, {13, 2}, { 3, 1}, { 4, 0}, { 1, 3}, { 5, 1}, { 1, 4},  /* Row 41 */
  { 5, 2}, { 1, 4}, { 4, 1}, { 1, 0},
  { 3, 0}, { 3, 1}, {13, 2}, { 3, 1}, { 5, 0}, { 1, 3}, {14, 1}, { 2, 0},  /* Row 42 */
  { 3, 0}, { 3, 1}, {12, 2}, { 1, 4}, { 2, 1}, { 1, 3}, { 6, 0}, {12, 1},  /* Row 43 */
  { 1, 4}, { 3, 0},
  { 4, 0}, { 4, 1}, {10, 2}, { 3, 1}, { 1, 4}, { 9, 0}, { 1, 4}, { 7, 1},  /* Row 44 */
  { 1, 3}, { 4, 0},
  { 4, 0}, { 1, 3}, { 5, 1}, { 1, 4}, { 5, 2}, { 1, 4}, { 4, 1}, {23, 0},  /* Row 45 */
  { 5, 0}, { 1, 3}, {13, 1}, {25, 0},                                      /* Row 46 */
  { 7, 0}, {11, 1}, {26, 0},                                               /* Row 47 */
  { 9, 0}, { 1, 3}, { 4, 1}, { 2, 4}, {28, 0},                             /* Row 48 */
  {44, 0}                                                                  /* Row 49 */
};

/********************************************************************************************
 * Public Bitmap Structure Definitions
 ********************************************************************************************/

const struct SRlePaletteBitmap NXWidgets::g_mediaplayerBitmap =
{
  CONFIG_NXWIDGETS_BPP,     // bpp    - Bits per pixel
  CONFIG_NXWIDGETS_FMT,     // fmt    - Color format
  BITMAP_NLUTCODES,         // nlut   - Number of colors in the lLook-Up Table (LUT)
  BITMAP_NCOLUMNS,          // width  - Width in pixels
  BITMAP_NROWS,             // height - Height in rows
  {                         // lut    - Pointer to the beginning of the Look-Up Table (LUT)
    g_mediaplayerNormalLut, //          Index 0: Unselected LUT
    g_mediaplayerBrightLut, //          Index 1: Selected LUT
  },
  g_mediaplayerRleEntries   // data   - Pointer to the beginning of the RLE data
};
