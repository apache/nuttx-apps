/********************************************************************************************
 * NxWidgets/nxwm/src/glyph_mediaplayer43x41.cxx
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

/* Automatically NuttX bitmap file. */
/* Generated from play_music.png by bitmap_converter.py. */

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

#define FORWARD_BITMAP_NROWS    41
#define FORWARD_BITMAP_NCOLUMNS 43

#define PLAY_BITMAP_NROWS       41
#define PLAY_BITMAP_NCOLUMNS    21

#define REWIND_BITMAPNROWS      41
#define REWIND_BITMAPNCOLUMNS   43

#define PAUSE_BITMAP_NROWS      41
#define PAUSE_BITMAP_NCOLUMNS   21

#define BITMAP_NLUTCODES        8

/********************************************************************************************
 * Private Data
 ********************************************************************************************/

using namespace NxWM;

/* RGB Colors */

static const NXWidgets::nxwidget_pixel_t g_normalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR,                                   /* Code 0 */
  MKRGB(189,189,189)                                                     /* Code 1 */
};

static const NXWidgets::nxwidget_pixel_t g_brightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR,                                   /* Code 0 */
  MKRGB(252,252,252)                                                     /* Code 1 */
};

/* Bitmap definition for the "Forward" button */

static const struct NXWidgets::SRlePaletteBitmapEntry g_forwardRleEntries[] =
{
  { 1, 1}, {21, 0}, { 1, 1}, {20, 0},  /* Row 0 */
  { 2, 1}, {20, 0}, { 2, 1}, {19, 0},  /* Row 1 */
  { 3, 1}, {19, 0}, { 3, 1}, {18, 0},  /* Row 2 */
  { 4, 1}, {18, 0}, { 4, 1}, {17, 0},  /* Row 3 */
  { 5, 1}, {17, 0}, { 5, 1}, {16, 0},  /* Row 4 */
  { 6, 1}, {16, 0}, { 6, 1}, {15, 0},  /* Row 5 */
  { 7, 1}, {15, 0}, { 7, 1}, {14, 0},  /* Row 6 */
  { 8, 1}, {14, 0}, { 8, 1}, {13, 0},  /* Row 7 */
  { 9, 1}, {13, 0}, { 9, 1}, {12, 0},  /* Row 8 */
  {10, 1}, {12, 0}, {10, 1}, {11, 0},  /* Row 9 */
  {11, 1}, {11, 0}, {11, 1}, {10, 0},  /* Row 10 */
  {12, 1}, {10, 0}, {12, 1}, { 9, 0},  /* Row 11 */
  {13, 1}, { 9, 0}, {13, 1}, { 8, 0},  /* Row 12 */
  {14, 1}, { 8, 0}, {14, 1}, { 7, 0},  /* Row 13 */
  {15, 1}, { 7, 0}, {15, 1}, { 6, 0},  /* Row 14 */
  {16, 1}, { 6, 0}, {16, 1}, { 5, 0},  /* Row 15 */
  {17, 1}, { 5, 0}, {17, 1}, { 4, 0},  /* Row 16 */
  {18, 1}, { 4, 0}, {18, 1}, { 3, 0},  /* Row 17 */
  {19, 1}, { 3, 0}, {19, 1}, { 2, 0},  /* Row 18 */
  {20, 1}, { 2, 0}, {20, 1}, { 1, 0},  /* Row 19 */
  {21, 1}, { 1, 0}, {21, 1},           /* Row 20 */
  {20, 1}, { 2, 0}, {20, 1}, { 1, 0},  /* Row 21 */
  {19, 1}, { 3, 0}, {19, 1}, { 2, 0},  /* Row 22 */
  {18, 1}, { 4, 0}, {18, 1}, { 3, 0},  /* Row 23 */
  {17, 1}, { 5, 0}, {17, 1}, { 4, 0},  /* Row 24 */
  {16, 1}, { 6, 0}, {16, 1}, { 5, 0},  /* Row 25 */
  {15, 1}, { 7, 0}, {15, 1}, { 6, 0},  /* Row 26 */
  {14, 1}, { 8, 0}, {14, 1}, { 7, 0},  /* Row 27 */
  {13, 1}, { 9, 0}, {13, 1}, { 8, 0},  /* Row 28 */
  {12, 1}, {10, 0}, {12, 1}, { 9, 0},  /* Row 29 */
  {11, 1}, {11, 0}, {11, 1}, {10, 0},  /* Row 30 */
  {10, 1}, {12, 0}, {10, 1}, {11, 0},  /* Row 31 */
  { 9, 1}, {13, 0}, { 9, 1}, {12, 0},  /* Row 32 */
  { 8, 1}, {14, 0}, { 8, 1}, {13, 0},  /* Row 33 */
  { 7, 1}, {15, 0}, { 7, 1}, {14, 0},  /* Row 34 */
  { 6, 1}, {16, 0}, { 6, 1}, {15, 0},  /* Row 35 */
  { 5, 1}, {17, 0}, { 5, 1}, {16, 0},  /* Row 36 */
  { 4, 1}, {18, 0}, { 4, 1}, {17, 0},  /* Row 37 */
  { 3, 1}, {19, 0}, { 3, 1}, {18, 0},  /* Row 38 */
  { 2, 1}, {20, 0}, { 2, 1}, {19, 0},  /* Row 39 */
  { 1, 1}, {21, 0}, { 1, 1}, {20, 0}   /* Row 40 */
};

/* Bitmap definition for the "Play" button */

static const struct NXWidgets::SRlePaletteBitmapEntry g_playRleEntries[] =
{
  { 1, 1}, {20, 0},  /* Row 0 */
  { 2, 1}, {19, 0},  /* Row 1 */
  { 3, 1}, {18, 0},  /* Row 2 */
  { 4, 1}, {17, 0},  /* Row 3 */
  { 5, 1}, {16, 0},  /* Row 4 */
  { 6, 1}, {15, 0},  /* Row 5 */
  { 7, 1}, {14, 0},  /* Row 6 */
  { 8, 1}, {13, 0},  /* Row 7 */
  { 9, 1}, {12, 0},  /* Row 8 */
  {10, 1}, {11, 0},  /* Row 9 */
  {11, 1}, {10, 0},  /* Row 10 */
  {12, 1}, { 9, 0},  /* Row 11 */
  {13, 1}, { 8, 0},  /* Row 12 */
  {14, 1}, { 7, 0},  /* Row 13 */
  {15, 1}, { 6, 0},  /* Row 14 */
  {16, 1}, { 5, 0},  /* Row 15 */
  {17, 1}, { 4, 0},  /* Row 16 */
  {18, 1}, { 3, 0},  /* Row 17 */
  {19, 1}, { 2, 0},  /* Row 18 */
  {20, 1}, { 1, 0},  /* Row 19 */
  {21, 1},           /* Row 20 */
  {20, 1}, { 1, 0},  /* Row 21 */
  {19, 1}, { 2, 0},  /* Row 22 */
  {18, 1}, { 3, 0},  /* Row 23 */
  {17, 1}, { 4, 0},  /* Row 24 */
  {16, 1}, { 5, 0},  /* Row 25 */
  {15, 1}, { 6, 0},  /* Row 26 */
  {14, 1}, { 7, 0},  /* Row 27 */
  {13, 1}, { 8, 0},  /* Row 28 */
  {12, 1}, { 9, 0},  /* Row 29 */
  {11, 1}, {10, 0},  /* Row 30 */
  {10, 1}, {11, 0},  /* Row 31 */
  { 9, 1}, {12, 0},  /* Row 32 */
  { 8, 1}, {13, 0},  /* Row 33 */
  { 7, 1}, {14, 0},  /* Row 34 */
  { 6, 1}, {15, 0},  /* Row 35 */
  { 5, 1}, {16, 0},  /* Row 36 */
  { 4, 1}, {17, 0},  /* Row 37 */
  { 3, 1}, {18, 0},  /* Row 38 */
  { 2, 1}, {19, 0},  /* Row 39 */
  { 1, 1}, {20, 0}  /* Row 40 */
};

/* Bitmap definition for "Rewind" control */

static const struct NXWidgets::SRlePaletteBitmapEntry g_rewindRleEntries[] =
{
  {20, 0}, { 1, 1}, {21, 0}, { 1, 1},  /* Row 0 */
  {19, 0}, { 2, 1}, {20, 0}, { 2, 1},  /* Row 1 */
  {18, 0}, { 3, 1}, {19, 0}, { 3, 1},  /* Row 2 */
  {17, 0}, { 4, 1}, {18, 0}, { 4, 1},  /* Row 3 */
  {16, 0}, { 5, 1}, {17, 0}, { 5, 1},  /* Row 4 */
  {15, 0}, { 6, 1}, {16, 0}, { 6, 1},  /* Row 5 */
  {14, 0}, { 7, 1}, {15, 0}, { 7, 1},  /* Row 6 */
  {13, 0}, { 8, 1}, {14, 0}, { 8, 1},  /* Row 7 */
  {12, 0}, { 9, 1}, {13, 0}, { 9, 1},  /* Row 8 */
  {11, 0}, {10, 1}, {12, 0}, {10, 1},  /* Row 9 */
  {10, 0}, {11, 1}, {11, 0}, {11, 1},  /* Row 10 */
  { 9, 0}, {12, 1}, {10, 0}, {12, 1},  /* Row 11 */
  { 8, 0}, {13, 1}, { 9, 0}, {13, 1},  /* Row 12 */
  { 7, 0}, {14, 1}, { 8, 0}, {14, 1},  /* Row 13 */
  { 6, 0}, {15, 1}, { 7, 0}, {15, 1},  /* Row 14 */
  { 5, 0}, {16, 1}, { 6, 0}, {16, 1},  /* Row 15 */
  { 4, 0}, {17, 1}, { 5, 0}, {17, 1},  /* Row 16 */
  { 3, 0}, {18, 1}, { 4, 0}, {18, 1},  /* Row 17 */
  { 2, 0}, {19, 1}, { 3, 0}, {19, 1},  /* Row 18 */
  { 1, 0}, {20, 1}, { 2, 0}, {20, 1},  /* Row 19 */
  {21, 1}, { 1, 0}, {21, 1},           /* Row 20 */
  { 1, 0}, {20, 1}, { 2, 0}, {20, 1},  /* Row 21 */
  { 2, 0}, {19, 1}, { 3, 0}, {19, 1},  /* Row 22 */
  { 3, 0}, {18, 1}, { 4, 0}, {18, 1},  /* Row 23 */
  { 4, 0}, {17, 1}, { 5, 0}, {17, 1},  /* Row 24 */
  { 5, 0}, {16, 1}, { 6, 0}, {16, 1},  /* Row 25 */
  { 6, 0}, {15, 1}, { 7, 0}, {15, 1},  /* Row 26 */
  { 7, 0}, {14, 1}, { 8, 0}, {14, 1},  /* Row 27 */
  { 8, 0}, {13, 1}, { 9, 0}, {13, 1},  /* Row 28 */
  { 9, 0}, {12, 1}, {10, 0}, {12, 1},  /* Row 29 */
  {10, 0}, {11, 1}, {11, 0}, {11, 1},  /* Row 30 */
  {11, 0}, {10, 1}, {12, 0}, {10, 1},  /* Row 31 */
  {12, 0}, { 9, 1}, {13, 0}, { 9, 1},  /* Row 32 */
  {13, 0}, { 8, 1}, {14, 0}, { 8, 1},  /* Row 33 */
  {14, 0}, { 7, 1}, {15, 0}, { 7, 1},  /* Row 34 */
  {15, 0}, { 6, 1}, {16, 0}, { 6, 1},  /* Row 35 */
  {16, 0}, { 5, 1}, {17, 0}, { 5, 1},  /* Row 36 */
  {17, 0}, { 4, 1}, {18, 0}, { 4, 1},  /* Row 37 */
  {18, 0}, { 3, 1}, {19, 0}, { 3, 1},  /* Row 38 */
  {19, 0}, { 2, 1}, {20, 0}, { 2, 1},  /* Row 39 */
  {20, 0}, { 1, 1}, {21, 0}, { 1, 1}   /* Row 40 */
};

/* Bitmap definition for the "Pause" button */

static const struct NXWidgets::SRlePaletteBitmapEntry g_pauseRleEntries[] =
{
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 0 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 1 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 2 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 3 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 4 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 5 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 6 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 7 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 8 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 9 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 10 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 11 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 12 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 13 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 14 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 15 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 16 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 17 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 18 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 19 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 20 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 21 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 22 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 23 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 24 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 25 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 26 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 27 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 28 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 29 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 30 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 31 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 32 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 33 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 34 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 35 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 36 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 37 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 38 */
  { 2, 1}, { 5, 0}, {14, 1},  /* Row 39 */
  { 2, 1}, { 5, 0}, {14, 1}   /* Row 40 */
};

/********************************************************************************************
 * Public Data
 ********************************************************************************************/

const struct NXWidgets::SRlePaletteBitmap NxWM::g_mplayerFwdBitmap =
{
  CONFIG_NXWIDGETS_BPP,    // bpp    - Bits per pixel
  CONFIG_NXWIDGETS_FMT,    // fmt    - Color format
  BITMAP_NLUTCODES,        // nlut   - Number of colors in the lLook-Up Table (LUT)
  FORWARD_BITMAP_NCOLUMNS, // width  - Width in pixels
  FORWARD_BITMAP_NROWS,    // height - Height in rows
  {                        // lut    - Pointer to the beginning of the Look-Up Table (LUT)
    g_normalLut,           //          Index 0: Unselected LUT
    g_brightLut,           //          Index 1: Selected LUT
  },
  g_forwardRleEntries      // data   - Pointer to the beginning of the RLE data
};

const struct NXWidgets::SRlePaletteBitmap NxWM::g_mplayerPlayBitmap =
{
  CONFIG_NXWIDGETS_BPP,    // bpp    - Bits per pixel
  CONFIG_NXWIDGETS_FMT,    // fmt    - Color format
  BITMAP_NLUTCODES,        // nlut   - Number of colors in the lLook-Up Table (LUT)
  PLAY_BITMAP_NCOLUMNS,    // width  - Width in pixels
  PLAY_BITMAP_NROWS,       // height - Height in rows
  {                        // lut    - Pointer to the beginning of the Look-Up Table (LUT)
    g_normalLut,           //          Index 0: Unselected LUT
    g_brightLut,           //          Index 1: Selected LUT
  },
  g_playRleEntries         // data   - Pointer to the beginning of the RLE data
};

const struct NXWidgets::SRlePaletteBitmap NxWM::g_mplayerRewBitmap =
{
  CONFIG_NXWIDGETS_BPP,  // bpp    - Bits per pixel
  CONFIG_NXWIDGETS_FMT,  // fmt    - Color format
  BITMAP_NLUTCODES,      // nlut   - Number of colors in the lLook-Up Table (LUT)
  REWIND_BITMAPNCOLUMNS, // width  - Width in pixels
  REWIND_BITMAPNROWS,    // height - Height in rows
  {                      // lut    - Pointer to the beginning of the Look-Up Table (LUT)
    g_normalLut,         //          Index 0: Unselected LUT
    g_brightLut,         //          Index 1: Selected LUT
  },
  g_rewindRleEntries     // data   - Pointer to the beginning of the RLE data
};

const struct NXWidgets::SRlePaletteBitmap NxWM::g_mplayerPauseBitmap =
{
  CONFIG_NXWIDGETS_BPP,  // bpp    - Bits per pixel
  CONFIG_NXWIDGETS_FMT,  // fmt    - Color format
  BITMAP_NLUTCODES,      // nlut   - Number of colors in the lLook-Up Table (LUT)
  PAUSE_BITMAP_NCOLUMNS, // width  - Width in pixels
  PAUSE_BITMAP_NROWS,    // height - Height in rows
  {                      // lut    - Pointer to the beginning of the Look-Up Table (LUT)
    g_normalLut,         //          Index 0: Unselected LUT
    g_brightLut,         //          Index 1: Selected LUT
  },
  g_pauseRleEntries      // data   - Pointer to the beginning of the RLE data
};
