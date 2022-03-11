/********************************************************************************************
 * apps/graphics/nxglyphs/src/glyph_cmd24x22.cxx
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

#define BITMAP_NROWS     22
#define BITMAP_NCOLUMNS  25
#define BITMAP_NLUTCODES 8

/********************************************************************************************
 * Private Bitmap Data
 ********************************************************************************************/

using namespace NXWidgets;

/* RGB24 (8-8-8) Colors */

#if CONFIG_NXWIDGETS_BPP == 24 ||  CONFIG_NXWIDGETS_BPP == 32

static const uint32_t g_cmdNormalLut[BITMAP_NLUTCODES] =
{
  0x909090, 0x000000, 0xb4fcfc, 0xb4d8fc, 0x6cb4fc, 0x6c6c6c, 0xfcfcfc, 0x484848
};

static const uint32_t g_cmdBrightLut[BITMAP_NLUTCODES] =
{
  0xababab, 0x3f3f3f, 0xc6fcfc, 0xc6e1fc, 0x90c6fc, 0x909090, 0xfcfcfc, 0x757575
};
/* RGB16 (565) Colors (four of the colors in this map are duplicates) */

#elif CONFIG_NXWIDGETS_BPP == 16

static const uint16_t g_cmdNormalLut[BITMAP_NLUTCODES] =
{
  0x9492, 0x0000, 0xb7ff, 0xb6df, 0x6dbf, 0x6b6d, 0xffff, 0x4a49
};

static const uint16_t g_cmdBrightLut[BITMAP_NLUTCODES] =
{
  0xad55, 0x39e7, 0xc7ff, 0xc71f, 0x963f, 0x9492, 0xffff, 0x73ae
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

static const uint8_t g_cmdNormalLut[BITMAP_NLUTCODES] =
{
  0x90, 0x00, 0xe6, 0xd1, 0xa6, 0x6c, 0xfc, 0x48
};

static const uint8_t g_cmdBrightLut[BITMAP_NLUTCODES] =
{
  0xab, 0x3f, 0xeb, 0xdc, 0xbc, 0x90, 0xfc, 0x75
};

#  else /* CONFIG_NXWIDGETS_GREYSCALE */

/* RGB8 (332) Colors */

static const nxgl_mxpixel_t g_cmdNormalLut[BITMAP_NLUTCODES] =
{
  0x92, 0x00, 0xbf, 0xbb, 0x77, 0x6d, 0xff, 0x49
};

static const nxgl_mxpixel_t g_cmdBrightLut[BITMAP_NLUTCODES] =
{
  0xb6, 0x24, 0xdf, 0xdf, 0x9b, 0x92, 0xff, 0x6d
};

#  endif
#else
# error "Unsupported pixel format"
#endif

static const struct SRlePaletteBitmapEntry g_cmdRleEntries[] =
{
  { 24,   0}, {  1,   1},                                                                          /* Row 0 */
  { 24,   0}, {  1,   1},                                                                          /* Row 1 */
  {  2,   0}, {  1,   2}, { 13,   3}, {  7,   4}, {  1,   0}, {  1,   1},                          /* Row 2 */
  {  2,   0}, {  3,   3}, { 18,   4}, {  1,   0}, {  1,   1},                                      /* Row 3 */
  {  2,   0}, {  3,   3}, { 18,   4}, {  1,   0}, {  1,   1},                                      /* Row 4 */
  { 24,   0}, {  1,   1},                                                                          /* Row 5 */
  {  1,   0}, { 22,   1}, {  1,   0}, {  1,   1},                                                  /* Row 6 */
  {  1,   0}, {  1,   1}, { 21,   5}, {  1,   0}, {  1,   1},                                      /* Row 7 */
  {  1,   0}, {  1,   1}, { 21,   5}, {  1,   0}, {  1,   1},                                      /* Row 8 */
  {  1,   0}, {  1,   1}, {  2,   5}, {  1,   6}, { 18,   5}, {  1,   0}, {  1,   1},              /* Row 9 */
  {  1,   0}, {  1,   1}, {  3,   5}, {  1,   6}, { 17,   5}, {  1,   0}, {  1,   1},              /* Row 10 */
  {  1,   0}, {  1,   1}, {  4,   5}, {  1,   6}, { 16,   5}, {  1,   0}, {  1,   1},              /* Row 11 */
  {  1,   0}, {  1,   1}, {  5,   5}, {  1,   6}, { 15,   5}, {  1,   0}, {  1,   1},              /* Row 12 */
  {  1,   0}, {  1,   1}, {  6,   5}, {  1,   6}, { 14,   5}, {  1,   0}, {  1,   1},              /* Row 13 */
  {  1,   0}, {  1,   1}, {  5,   5}, {  1,   6}, {  1,   7}, { 14,   5}, {  1,   0}, {  1,   1},  /* Row 14 */
  {  1,   0}, {  1,   1}, {  4,   5}, {  1,   6}, {  1,   7}, { 15,   5}, {  1,   0}, {  1,   1},  /* Row 15 */
  {  1,   0}, {  1,   1}, {  3,   5}, {  1,   6}, {  1,   7}, {  4,   5}, {  9,   6}, {  3,   5},  /* Row 16 */
  {  1,   0}, {  1,   1},
  {  1,   0}, {  1,   1}, {  2,   5}, {  1,   6}, {  1,   7}, {  6,   5}, {  9,   7}, {  2,   5},  /* Row 17 */
  {  1,   0}, {  1,   1},
  {  1,   0}, {  1,   1}, {  2,   5}, {  1,   7}, { 18,   5}, {  1,   0}, {  1,   1},              /* Row 18 */
  {  1,   0}, {  1,   1}, { 21,   5}, {  1,   0}, {  1,   1},                                      /* Row 19 */
  { 24,   0}, {  1,   1},                                                                          /* Row 20 */
  { 25,   1},                                                                                      /* Row 21 */
};

/********************************************************************************************
 * Public Bitmap Structure Definitions
 ********************************************************************************************/

const struct SRlePaletteBitmap NXWidgets::g_cmdBitmap =
{
  CONFIG_NXWIDGETS_BPP,  // bpp    - Bits per pixel
  CONFIG_NXWIDGETS_FMT,  // fmt    - Color format
  BITMAP_NLUTCODES,      // nlut   - Number of colors in the lLook-Up Table (LUT)
  BITMAP_NCOLUMNS,       // width  - Width in pixels
  BITMAP_NROWS,          // height - Height in rows
  {                      // lut    - Pointer to the beginning of the Look-Up Table (LUT)
    g_cmdNormalLut,      //          Index 0: Unselected LUT
    g_cmdBrightLut,      //          Index 1: Selected LUT
  },
  g_cmdRleEntries        // data   - Pointer to the beginning of the RLE data
};
