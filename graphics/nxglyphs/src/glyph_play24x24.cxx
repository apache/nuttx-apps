/********************************************************************************************
 * apps/graphics/nxglyphs/src/glyph_play24x24.cxx
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

#define BITMAP_NROWS     24
#define BITMAP_NCOLUMNS  24
#define BITMAP_NLUTCODES 5

#define DARK_PLAY_ICON   1

/********************************************************************************************
 * Private Bitmap Data
 ********************************************************************************************/

using namespace NXWidgets;

/* RGB24 (8-8-8) Colors */

#if CONFIG_NXWIDGETS_BPP == 24 ||  CONFIG_NXWIDGETS_BPP == 32
#  ifdef DARK_PLAY_ICON

static const uint32_t g_playNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                    /* Code 0 */
  0x00bd00, 0x008100, 0x006300, 0x003600                              /* Codes 1-4 */
};

static const uint32_t g_playBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                    /* Code 0 */
  0x00fc00, 0x00ac00, 0x008400, 0x004800                              /* Codes 1-4 */
};

#  else /* DARK_PLAY_ICON */

static const uint32_t g_playNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                    /* Code 0 */
  0x00fc00, 0x00ac00, 0x008400, 0x004800                              /* Codes 1-4 */
};

static const uint32_t g_playBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                    /* Code 0 */
  0x00fc00, 0x00c000, 0x00a200, 0x007500                              /* Codes 1-4 */
};
#  endif /* DARK_PLAY_ICON */

/* RGB16 (565) Colors (four of the colors in this map are duplicates) */

#elif CONFIG_NXWIDGETS_BPP == 16
#  ifdef DARK_PLAY_ICON

static const uint16_t g_playNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                    /* Code 0 */
  0x05e0, 0x0400, 0x0300, 0x01a0                                      /* Codes 1-4 */
};

static const uint16_t g_playBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                    /* Code 0 */
  0x07e0, 0x0560, 0x0420, 0x0240                                      /* Codes 1-4 */
};

#  else /* DARK_PLAY_ICON */

static const uint16_t g_playNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                    /* Code 0 */
  0x07e0, 0x0560, 0x0420, 0x0240                                      /* Codes 1-4 */
};

static const uint16_t g_playBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                    /* Code 0 */
  0x07e0, 0x0600, 0x0500, 0x03a0                                      /* Codes 1-4 */
};

#  endif /* DARK_PLAY_ICON */

/* 8-bit color lookups.  NOTE:  This is really dumb!  The lookup index is 8-bits and it used
 * to lookup an 8-bit value.  There is no savings in that!  It would be better to just put
 * the 8-bit color/greyscale value in the run-length encoded image and save the cost of these
 * pointless lookups.  But these p;ointless lookups do make the logic compatible with the
 * 16- and 24-bit types.
 */

#elif CONFIG_NXWIDGETS_BPP == 8
#  ifdef CONFIG_NXWIDGETS_GREYSCALE

/* 8-bit Greyscale */

#    ifdef DARK_PLAY_ICON

static const uint8_t g_playNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                    /* Code 0 */
  0x6e, 0x4b, 0x3a, 0x1f                                              /* Codes 1-4 */
};

static const uint8_t g_playBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                    /* Code 0 */
  0x93, 0x64, 0x4d, 0x2a                                              /* Codes 1-4 */
};

#    else /* DARK_PLAY_ICON */

static const uint8_t g_playNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                    /* Code 0 */
  0x93, 0x64, 0x4d, 0x2a                                              /* Codes 1-4 */
};

static const uint8_t g_playBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                    /* Code 0 */
  0x93, 0x70, 0x5f, 0x44                                              /* Codes 1-4 */
};

#    endif /* DARK_PLAY_ICON */
#  else /* CONFIG_NXWIDGETS_GREYSCALE */

/* RGB8 (332) Colors */

#    ifdef DARK_PLAY_ICON
static const nxgl_mxpixel_t g_playNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                    /* Code 0 */
  0x14, 0x10, 0x0c, 0x04                                              /* Codes 1-4 */
};

static const nxgl_mxpixel_t g_playBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                    /* Code 0 */
  0x1c, 0x14, 0x10, 0x08                                              /* Codes 1-4 */
};

#    else /* DARK_PLAY_ICON */

static const nxgl_mxpixel_t g_playNormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                    /* Code 0 */
  0x1c, 0x14, 0x10, 0x08                                              /* Codes 1-4 */
};

static const nxgl_mxpixel_t g_playBrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR,                                    /* Code 0 */
  0x1c, 0x18, 0x14, 0x0c                                              /* Codes 1-4 */
};

#    endif /* DARK_PLAY_ICON */
#  endif /* CONFIG_NXWIDGETS_GREYSCALE */
#else
# error "Unsupported pixel format"
#endif

static const struct SRlePaletteBitmapEntry g_playRleEntries[] =
{
  {  2,   1}, { 22,   0},              /* Row 0 */
  {  2,   2}, {  2,   1}, { 20,   0},  /* Row 1 */
  {  4,   2}, {  2,   1}, { 18,   0},  /* Row 2 */
  {  6,   2}, {  2,   1}, { 16,   0},  /* Row 3 */
  {  8,   2}, {  2,   1}, { 14,   0},  /* Row 4 */
  { 10,   2}, {  2,   1}, { 12,   0},  /* Row 5 */
  { 12,   2}, {  2,   1}, { 10,   0},  /* Row 6 */
  { 14,   2}, {  2,   1}, {  8,   0},  /* Row 7 */
  { 16,   2}, {  2,   1}, {  6,   0},  /* Row 8 */
  { 18,   2}, {  2,   1}, {  4,   0},  /* Row 9 */
  { 20,   2}, {  2,   1}, {  2,   0},  /* Row 10 */
  { 22,   2}, {  2,   1},              /* Row 11 */
  { 22,   3}, {  2,   4},              /* Row 12 */
  { 20,   3}, {  2,   4}, {  2,   0},  /* Row 13 */
  { 18,   3}, {  2,   4}, {  4,   0},  /* Row 14 */
  { 16,   3}, {  2,   4}, {  6,   0},  /* Row 15 */
  { 14,   3}, {  2,   4}, {  8,   0},  /* Row 16 */
  { 12,   3}, {  2,   4}, { 10,   0},  /* Row 17 */
  { 10,   3}, {  2,   4}, { 12,   0},  /* Row 18 */
  {  8,   3}, {  2,   4}, { 14,   0},  /* Row 19 */
  {  6,   3}, {  2,   4}, { 16,   0},  /* Row 20 */
  {  4,   3}, {  2,   4}, { 18,   0},  /* Row 21 */
  {  2,   3}, {  2,   4}, { 20,   0},  /* Row 22 */
  {  2,   4}, { 22,   0},              /* Row 23 */
};

/********************************************************************************************
 * Public Bitmap Structure Definitions
 ********************************************************************************************/

const struct SRlePaletteBitmap NXWidgets::g_playBitmap =
{
  CONFIG_NXWIDGETS_BPP,  // bpp    - Bits per pixel
  CONFIG_NXWIDGETS_FMT,  // fmt    - Color format
  BITMAP_NLUTCODES,      // nlut   - Number of colors in the lLook-Up Table (LUT)
  BITMAP_NCOLUMNS,       // width  - Width in pixels
  BITMAP_NROWS,          // height - Height in rows
  {                      // lut    - Pointer to the beginning of the Look-Up Table (LUT)
    g_playNormalLut,     //          Index 0: Unselected LUT
    g_playBrightLut,     //          Index 1: Selected LUT
  },
  g_playRleEntries       // data   - Pointer to the beginning of the RLE data
};
