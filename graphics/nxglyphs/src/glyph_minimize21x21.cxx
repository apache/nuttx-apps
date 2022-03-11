/********************************************************************************************
 * apps/graphics/nxglyphs/src/glyph_minimize21x21.cxx
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

#define BITMAP_NROWS     21
#define BITMAP_NCOLUMNS  21
#define BITMAP_NLUTCODES 8

/********************************************************************************************
 * Private Bitmap Data
 ********************************************************************************************/

using namespace NXWidgets;

/* RGB24 (8-8-8) Colors */

#if CONFIG_NXWIDGETS_BPP == 24 ||  CONFIG_NXWIDGETS_BPP == 32

static const uint32_t g_minimizeNormalLut[BITMAP_NLUTCODES] =
{
  0x2448b4, 0x486cd8, 0x0024b4, 0x0024d8, 0x242490, 0x0000b4, 0xfcfcfc, 0xd8fcfc  /* Codes 0-7 */
};

static const uint32_t g_minimizeBrightLut[BITMAP_NLUTCODES] =
{
  0x5a75c6, 0x7590e1, 0x3f5ac6, 0x3f5ae1, 0x5a5aab, 0x3f3fc6, 0xfcfcfc, 0xe1fcfc  /* Codes 0-7 */
};

/* RGB16 (565) Colors (four of the colors in this map are duplicates) */

#elif CONFIG_NXWIDGETS_BPP == 16

static const uint16_t g_minimizeNormalLut[BITMAP_NLUTCODES] =
{
  0x2256, 0x4b7b, 0x0136, 0x013b, 0x2132, 0x0016, 0xffff, 0xdfff  /* Codes 0-7 */
};

static const uint16_t g_minimizeBrightLut[BITMAP_NLUTCODES] =
{
  0x5bb8, 0x749c, 0x3ad8, 0x3adc, 0x5ad5, 0x39f8, 0xffff, 0xe7ff  /* Codes 0-7 */
};

/* 8-bit color lookups.  NOTE:  This is really dumb!  The lookup index is 8-bits and it used
 * to lookup an 8-bit value.  There is no savings in that!  It would be better to just put
 * the 8-bit color/greyscale value in the run-length encoded image and save the cost of these
 * pointless lookups.  But these p;ointless lookups do make the logic compatible with the
 * 16- and 24-bit types.
 */

#elif CONFIG_NXWIDGETS_BPP == 8
#  ifdef CONFIG_NXWIDGETS_GREYSCALE

/* 8-bit Greyscale */

static const uint8_t g_minimizeNormalLut[BITMAP_NLUTCODES] =
{
  0x49, 0x6d, 0x29, 0x2d, 0x30, 0x14, 0xfc, 0xf1  /* Codes 0-7 */
};

static const uint8_t g_minimizeBrightLut[BITMAP_NLUTCODES] =
{
  0x76, 0x91, 0x5e, 0x61, 0x63, 0x4e, 0xfc, 0xf3  /* Codes 0-7 */
};

#  else /* CONFIG_NXWIDGETS_GREYSCALE */

/* RGB8 (332) Colors */

static const nxgl_mxpixel_t g_minimizeNormalLut[BITMAP_NLUTCODES] =
{
  0x2a, 0x4f, 0x06, 0x07, 0x26, 0x02, 0xff, 0xdf  /* Codes 0-7 */
};

static const uint8_t g_minimizeBrightLut[BITMAP_NLUTCODES] =
{
  0x4f, 0x73, 0x2b, 0x2b, 0x4a, 0x27, 0xff, 0xff  /* Codes 0-7 */
};

#  endif
#else
# error "Unsupported pixel format"
#endif

static const struct SRlePaletteBitmapEntry g_minimizeRleEntries[] =
{
  {  1,   0}, { 20,   1},  /* Row 0 */
  {  1,   1}, {  5,   2}, {  5,   3}, {  8,   2}, {  1,   4}, {  1,   0},  /* Row 1 */
  {  1,   1}, {  3,   2}, {  8,   3}, {  8,   2}, {  1,   4},              /* Row 2 */
  {  1,   1}, {  2,   2}, { 10,   3}, {  7,   2}, {  1,   4},              /* Row 3 */
  {  1,   1}, {  1,   2}, { 10,   3}, {  8,   2}, {  1,   4},              /* Row 4 */
  {  1,   1}, {  1,   2}, {  9,   3}, {  9,   2}, {  1,   4},              /* Row 5 */
  {  1,   1}, {  9,   3}, {  9,   2}, {  1,   5}, {  1,   4},              /* Row 6 */
  {  1,   1}, {  8,   3}, { 10,   2}, {  1,   5}, {  1,   4},              /* Row 7 */
  {  1,   1}, {  7,   3}, { 11,   2}, {  1,   5}, {  1,   4},              /* Row 8 */
  {  1,   1}, {  1,   2}, {  4,   3}, { 12,   2}, {  2,   5}, {  1,   4},  /* Row 9 */
  {  1,   1}, {  2,   2}, {  1,   3}, { 14,   2}, {  2,   5}, {  1,   4},  /* Row 10 */
  {  1,   1}, { 16,   2}, {  3,   5}, {  1,   4},                          /* Row 11 */
  {  1,   1}, { 15,   2}, {  4,   5}, {  1,   4},                          /* Row 12 */
  {  1,   1}, { 13,   2}, {  6,   5}, {  1,   4},                          /* Row 13 */
  {  1,   1}, { 11,   2}, {  8,   5}, {  1,   4},                          /* Row 14 */
  {  1,   1}, {  8,   2}, { 11,   5}, {  1,   4},                          /* Row 15 */
  {  1,   1}, {  4,   2}, { 15,   5}, {  1,   4},                          /* Row 16 */
  {  1,   1}, {  2,   2}, { 14,   6}, {  1,   7}, {  2,   5}, {  1,   4},  /* Row 17 */
  {  1,   1}, {  1,   2}, {  1,   5}, { 15,   7}, {  2,   5}, {  1,   4},  /* Row 18 */
  {  1,   1}, { 19,   5}, {  1,   0},                                      /* Row 19 */
  {  1,   1}, {  1,   0}, { 17,   4}, {  1,   0}, {  1,   1},              /* Row 20 */
};

/********************************************************************************************
 * Public Bitmap Structure Definitions
 ********************************************************************************************/

const struct SRlePaletteBitmap NXWidgets::g_minimizeBitmap =
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
