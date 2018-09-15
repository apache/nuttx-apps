/********************************************************************************************
 * NxWidgets/nxwm/src/glyph_play24x24.cxx
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

#define BITMAP_NROWS     48
#define BITMAP_NCOLUMNS  48
#define BITMAP_NLUTCODES 7

#define DARK_PLAY_ICON   1

/********************************************************************************************
 * Private Bitmap Data
 ********************************************************************************************/

using namespace NxWM;

/* RGB24 (8-8-8) Colors */

#if CONFIG_NXWIDGETS_BPP == 24 ||  CONFIG_NXWIDGETS_BPP == 32
#  ifdef DARK_PLAY_ICON

static const uint32_t g_playNormalLut[BITMAP_NLUTCODES] =
{
  0x00a200, 0x00bd00, CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR,            /* Codes 0-2 */
  0x008400, 0x006600, 0x004e00, 0x003600                              /* Codes 3-6 */
};

static const uint32_t g_playBrightLut[BITMAP_NLUTCODES] =
{
  0x00d800, 0x00fc00, CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR,            /* Codes 0-2 */
  0x00b000, 0x008800, 0x006800, 0x004800                              /* Codes 3-6 */
};

#  else /* DARK_PLAY_ICON */

static const uint32_t g_playNormalLut[BITMAP_NLUTCODES] =
{
  0x00d800, 0x00fc00, CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR,            /* Codes 0-2 */ 
  0x00b000, 0x008800, 0x006800, 0x004800                              /* Codes 3-6 */
};

static const uint32_t g_playBrightLut[BITMAP_NLUTCODES] =
{
  0x00ff00, 0x00ff00, CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR,            /* Codes 0-2 */
  0x00dc00, 0x00aa00, 0x008200, 0x005a00                              /* Codes 3-6 */
};

#  endif /* DARK_PLAY_ICON */

/* RGB16 (565) Colors */

#elif CONFIG_NXWIDGETS_BPP == 16
#  ifdef DARK_PLAY_ICON

static const uint16_t g_playNormalLut[BITMAP_NLUTCODES] =
{
  0x0500, 0x05e0, CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR,                /* Codes 0-2 */
  0x0420, 0x0320, 0x0260, 0x01a0                                      /* Codes 3-6 */
};

static const uint16_t g_playBrightLut[BITMAP_NLUTCODES] =
{
  0x06c0, 0x07e0, CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR,                /* Codes 0-2 */
  0x0580, 0x0440, 0x0340, 0x0240                                      /* Codes 3-6 */
};

#  else /* DARK_PLAY_ICON */

static const uint16_t g_playNormalLut[BITMAP_NLUTCODES] =
{
  0x06c0, 0x07e0, CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR,                /* Codes 0-2 */
  0x0580, 0x0440, 0x0340, 0x0240                                      /* Codes 3-6 */
};

static const uint16_t g_playBrightLut[BITMAP_NLUTCODES] =
{
  0x07e0, 0x07e0, CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR,                /* Codes 0-2 */
  0x06e0, 0x0540, 0x0400, 0x02c0                                      /* Codes 3-6 */
};

#  endif /* DARK_PLAY_ICON */

/* 8-bit color lookups.  NOTE:  This is really dumb!  The lookup index is 8-bits and it used
 * to lookup an 8-bit value.  There is no savings in that!  It would be better to just put
 * the 8-bit color/greyscale value in the run-length encoded image and save the cost of these
 * pointless lookups.  But these pointless lookups do make the logic compatible with the
 * 16- and 24-bit types.
 */

#elif CONFIG_NXWIDGETS_BPP == 8
#  ifdef CONFIG_NXWIDGETS_GREYSCALE

/* 8-bit Greyscale */

#    ifdef DARK_PLAY_ICON

static const uint8_t g_playNormalLut[BITMAP_NLUTCODES] =
{
  0x5f, 0x6e, CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR,                    /* Codes 0-2 */
  0x4d, 0x3b, 0x2d, 0x1f                                              /* Codes 3-6 */
};

static const uint8_t g_playBrightLut[BITMAP_NLUTCODES] =
{
  0x7e, 0x93, CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR,                    /* Codes 0-2 */
  0x67, 0x4f, 0x3d, 0x2a                                              /* Codes 3-6 */
};

#    else /* DARK_PLAY_ICON */

static const uint8_t g_playNormalLut[BITMAP_NLUTCODES] =
{
  0x7e, 0x93, CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR,                    /* Codes 0-2 */
  0x67, 0x4f, 0x3d, 0x2a                                              /* Codes 3-6 */
};

static const uint8_t g_playBrightLut[BITMAP_NLUTCODES] =
{
  0x95, 0x95, CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR,                    /* Codes 0-2 */
  0x81, 0x63, 0x4c, 0x34                                              /* Codes 3-6 */
};

#    endif /* DARK_PLAY_ICON */
#  else /* CONFIG_NXWIDGETS_GREYSCALE */

/* RGB8 (332) Colors */

#    ifdef DARK_PLAY_ICON

static const nxgl_mxpixel_t g_playNormalLut[BITMAP_NLUTCODES] =
{
  0x14, 0x14, CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR,                    /* Codes 0-2 */
  0x10, 0x0c, 0x08, 0x04                                              /* Codes 3-6 */
};

static const nxgl_mxpixel_t g_playBrightLut[BITMAP_NLUTCODES] =
{
  0x18, 0x1c, CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR,                    /* Codes 0-2 */
  0x14, 0x10, 0x0c, 0x08                                              /* Codes 3-6 */
};

#    else /* DARK_PLAY_ICON */

static const nxgl_mxpixel_t g_playNormalLut[BITMAP_NLUTCODES] =
{
  0x18, 0x1c, CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR,                    /* Codes 0-2 */
  0x14, 0x10, 0x0c, 0x08                                              /* Codes 3-6 */
};

static const nxgl_mxpixel_t g_playBrightLut[BITMAP_NLUTCODES] =
{
  0x1c, 0x1c, CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR,                    /* Codes 0-2 */
  0x18, 0x14, 0x10, 0x08                                              /* Codes 3-6 */
};

#    endif /* DARK_PLAY_ICON */
#  endif /* CONFIG_NXWIDGETS_GREYSCALE */
#else
# error "Unsupport pixel format"
#endif

static const struct NXWidgets::SRlePaletteBitmapEntry g_playRleEntries[] =
{
  {  1,  0}, {  2,  1}, { 45,  2},            /* Row 0 */
  {  2,  3}, {  1,  0}, {  2,  1}, { 43,  2}, /* Row 1 */
  {  4,  3}, {  1,  0}, {  2,  1}, { 41,  2}, /* Row 2 */
  {  6,  3}, {  1,  0}, {  2,  1}, { 39,  2}, /* Row 3 */
  {  8,  3}, {  1,  0}, {  2,  1}, { 37,  2}, /* Row 4 */
  { 10,  3}, {  1,  0}, {  2,  1}, { 35,  2}, /* Row 5 */
  { 12,  3}, {  1,  0}, {  2,  1}, { 33,  2}, /* Row 6 */
  { 14,  3}, {  1,  0}, {  2,  1}, { 31,  2}, /* Row 7 */
  { 16,  3}, {  1,  0}, {  2,  1}, { 29,  2}, /* Row 8 */
  { 18,  3}, {  1,  0}, {  2,  1}, { 27,  2}, /* Row 9 */
  { 20,  3}, {  1,  0}, {  2,  1}, { 25,  2}, /* Row 10 */
  { 22,  3}, {  1,  0}, {  2,  1}, { 23,  2}, /* Row 11 */
  { 24,  3}, {  1,  0}, {  2,  1}, { 21,  2}, /* Row 12 */
  { 26,  3}, {  1,  0}, {  2,  1}, { 19,  2}, /* Row 13 */
  { 28,  3}, {  1,  0}, {  2,  1}, { 17,  2}, /* Row 14 */
  { 30,  3}, {  1,  0}, {  2,  1}, { 15,  2}, /* Row 15 */
  { 32,  3}, {  1,  0}, {  2,  1}, { 13,  2}, /* Row 16 */
  { 34,  3}, {  1,  0}, {  2,  1}, { 11,  2}, /* Row 17 */
  { 36,  3}, {  1,  0}, {  2,  1}, {  9,  2}, /* Row 18 */
  { 38,  3}, {  1,  0}, {  2,  1}, {  7,  2}, /* Row 19 */
  { 40,  3}, {  1,  0}, {  2,  1}, {  5,  2}, /* Row 20 */
  { 42,  3}, {  1,  0}, {  2,  1}, {  3,  2}, /* Row 21 */
  { 44,  3}, {  1,  0}, {  2,  1}, {  1,  2}, /* Row 22 */
  { 46,  3}, {  1,  0}, {  1,  1},            /* Row 23 */
  { 46,  4}, {  1,  5}, {  1,  6},            /* Row 24 */
  { 44,  4}, {  1,  5}, {  2,  6}, {  1,  2}, /* Row 25 */
  { 42,  4}, {  3,  6}, {  3,  2},            /* Row 26 */
  { 40,  4}, {  1,  5}, {  2,  6}, {  5,  2}, /* Row 27 */
  { 38,  4}, {  3,  6}, {  7,  2},            /* Row 28 */
  { 36,  4}, {  1,  5}, {  2,  6}, {  9,  2}, /* Row 29 */
  { 34,  4}, {  3,  6}, { 11,  2},            /* Row 30 */
  { 32,  4}, {  1,  5}, {  2,  6}, { 13,  2}, /* Row 31 */
  { 30,  4}, {  3,  6}, { 15,  2},            /* Row 32 */
  { 28,  4}, {  1,  5}, {  2,  6}, { 17,  2}, /* Row 33 */
  { 26,  4}, {  3,  6}, { 19,  2},            /* Row 34 */
  { 24,  4}, {  1,  5}, {  2,  6}, { 21,  2}, /* Row 35 */
  { 22,  4}, {  3,  6}, { 23,  2},            /* Row 36 */
  { 20,  4}, {  1,  5}, {  2,  6}, { 25,  2}, /* Row 37 */
  { 18,  4}, {  3,  6}, { 27,  2},            /* Row 38 */
  { 16,  4}, {  1,  5}, {  2,  6}, { 29,  2}, /* Row 39 */
  { 14,  4}, {  3,  6}, { 31,  2},            /* Row 40 */
  { 12,  4}, {  1,  5}, {  2,  6}, { 33,  2}, /* Row 41 */
  { 10,  4}, {  3,  6}, { 35,  2},            /* Row 42 */
  {  8,  4}, {  1,  5}, {  2,  6}, { 37,  2}, /* Row 43 */
  {  6,  4}, {  3,  6}, { 39,  2},            /* Row 44 */
  {  4,  4}, {  1,  5}, {  2,  6}, { 41,  2}, /* Row 45 */
  {  2,  4}, {  3,  6}, { 43,  2},            /* Row 46 */
  {  1,  5}, {  2,  6}, { 45,  2},            /* Row 47 */
};

/********************************************************************************************
 * Public Bitmap Structure Definitions
 ********************************************************************************************/

const struct NXWidgets::SRlePaletteBitmap NxWM::g_playBitmap =
{
  CONFIG_NXWIDGETS_BPP, // bpp    - Bits per pixel
  CONFIG_NXWIDGETS_FMT, // fmt    - Color format
  BITMAP_NLUTCODES,     // nlut   - Number of colors in the lLook-Up Table (LUT)
  BITMAP_NCOLUMNS,      // width  - Width in pixels
  BITMAP_NROWS,         // height - Height in rows
  {                     // lut    - Pointer to the beginning of the Look-Up Table (LUT)
    g_playNormalLut,    //          Index 0: Unselected LUT
    g_playBrightLut,    //          Index 1: Selected LUT
  },
  g_playRleEntries      // data   - Pointer to the beginning of the RLE data
};
