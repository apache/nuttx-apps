/********************************************************************************************
 * apps/graphics/nxglyphs/src/glyph_menu21x21.cxx
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

#include "graphics/nxwidgets/crlepalettebitmap.hxx"
#include "graphics/nxglyphs.hxx"

/********************************************************************************************
 * Pre-Processor Definitions
 ********************************************************************************************/

#define BITMAP_NROWS     21
#define BITMAP_NCOLUMNS  21
#define BITMAP_NLUTCODES 6

/********************************************************************************************
 * Private Bitmap Data
 ********************************************************************************************/

using namespace NXWidgets;

#if CONFIG_NXWIDGETS_BPP == 24 ||  CONFIG_NXWIDGETS_BPP == 32
// RGB24 (8-8-8) Colors

static const uint32_t g_menuNormalLut[BITMAP_NLUTCODES] =
{
  0xc49c4c, 0xdc9000, 0xfcb800, 0xfcfcfc, 0xc47c00, 0x9c5800,  /* Codes 0-5 */
};

static const uint32_t g_menuBrightLut[BITMAP_NLUTCODES] =
{
  0xd2b478, 0xe4ab3f, 0xfcc93f, 0xfcfcfc, 0xd29c3f, 0xb4813f,  /* Codes 0-5 */
};

#elif CONFIG_NXWIDGETS_BPP == 16
// RGB16 (565) Colors (four of the colors in this map are duplicates)

static const uint16_t g_menuNormalLut[BITMAP_NLUTCODES] =
{
  0xc4e9, 0xdc80, 0xfdc0, 0xffff, 0xc3e0, 0x9ac0,  /* Codes 0-5 */
};

static const uint16_t g_menuBrightLut[BITMAP_NLUTCODES] =
{
  0xd5af, 0xe547, 0xfe47, 0xffff, 0xd4e7, 0xb407,  /* Codes 0-5 */
};

#elif CONFIG_NXWIDGETS_BPP == 8
// 8-bit color lookups.  NOTE:  This is really dumb!  The lookup index is 8-bits and it used
// to lookup an 8-bit value.  There is no savings in that!  It would be better to just put
// the 8-bit color/greyscale value in the run-length encoded image and save the cost of these
// pointless lookups.  But these p;ointless lookups do make the logic compatible with the
// 16- and 24-bit types.
///

#  ifdef CONFIG_NXWIDGETS_GREYSCALE
// 8-bit Greyscale

static const uint8_t g_menuNormalLut[BITMAP_NLUTCODES] =
{
  0x9e, 0x96, 0xb7, 0xfc, 0x83, 0x62,  /* Codes 0-5 */
}

static const uint8_t g_menuBrightLut[BITMAP_NLUTCODES] =
{
  0xb6, 0xaf, 0xc8, 0xfc, 0xa1, 0x88,  /* Codes 0-5 */
};

#  else /* CONFIG_NXWIDGETS_GREYSCALE */
// RGB8 (332) Colors

static const nxgl_mxpixel_t g_menuNormalLut[BITMAP_NLUTCODES] =
{
  0xd1, 0xd0, 0xf4, 0xff, 0xcc, 0x88,  /* Codes 0-5 */
};

static const uint8_t g_menuBrightLut[BITMAP_NLUTCODES] =
{
  0xd5, 0xf5, 0xf9, 0xff, 0xd1, 0xb1,  /* Codes 0-5 */
};

#  endif
#else
#  error Unsupported pixel format
#endif

static const struct SRlePaletteBitmapEntry g_menuRleEntries[] =
{
  { 21,   0},                                                                                        // Row 0
  {  1,   0}, {  4,   1}, {  7,   2}, {  9,   1},                                                    // Row 1
  {  1,   0}, {  3,   1}, {  9,   2}, {  8,   1},                                                    // Row 2
  {  1,   0}, {  2,   1}, { 10,   2}, {  8,   1},                                                    // Row 3
  {  1,   0}, {  1,   1}, {  2,   2}, { 14,   3}, {  3,   1},                                        // Row 4
  {  1,   0}, {  1,   1}, {  2,   2}, {  1,   3}, {  6,   2}, {  6,   1}, {  1,   3}, {  2,   1},    // Row 5
  {  1,   4},
  {  1,   0}, {  3,   2}, {  1,   3}, {  5,   2}, {  7,   1}, {  1,   3}, {  2,   1}, {  1,   4},    // Row 6
  {  1,   0}, {  3,   2}, {  1,   3}, {  4,   2}, {  8,   1}, {  1,   3}, {  2,   1}, {  1,   4},    // Row 7
  {  1,   0}, {  3,   2}, { 14,   3}, {  1,   1}, {  2,   4},                                        // Row 8
  {  1,   0}, {  3,   2}, {  1,   3}, {  1,   2}, { 11,   1}, {  1,   3}, {  1,   1}, {  2,   4},    // Row 9
  {  1,   0}, {  1,   1}, {  2,   2}, {  1,   3}, { 12,   1}, {  1,   3}, {  3,   4},                // Row 10
  {  1,   0}, {  3,   1}, { 14,   3}, {  3,   4},                                                    // Row 11
  {  1,   0}, {  3,   1}, {  1,   3}, { 11,   1}, {  1,   4}, {  1,   3}, {  3,   4},                // Row 12
  {  1,   0}, {  3,   1}, {  1,   3}, { 10,   1}, {  2,   4}, {  1,   3}, {  3,   4},                // Row 13
  {  1,   0}, {  3,   1}, { 14,   3}, {  3,   4},                                                    // Row 14
  {  1,   0}, {  3,   1}, {  1,   3}, {  8,   1}, {  4,   4}, {  1,   3}, {  3,   4},                // Row 15
  {  1,   0}, {  3,   1}, {  1,   3}, {  6,   1}, {  6,   4}, {  1,   3}, {  3,   4},                // Row 16
  {  1,   0}, {  3,   1}, { 14,   3}, {  3,   4},                                                    // Row 17
  {  1,   0}, {  4,   1}, { 16,   4},                                                                // Row 18
  {  1,   0}, {  2,   1}, { 18,   4},                                                                // Row 19
  {  1,   0}, { 19,   4}, {  1,   5},                                                                // Row 20
 };

/********************************************************************************************
 * Public Bitmap Structure Definitions
 ********************************************************************************************/

const struct SRlePaletteBitmap NXWidgets::g_menuBitmap =
{
  CONFIG_NXWIDGETS_BPP,  // bpp    - Bits per pixel
  CONFIG_NXWIDGETS_FMT,  // fmt    - Color format
  BITMAP_NLUTCODES,      // nlut   - Number of colors in the lLook-Up Table (LUT)
  BITMAP_NCOLUMNS,       // width  - Width in pixels
  BITMAP_NROWS,          // height - Height in rows
  {                      // lut    - Pointer to the beginning of the Look-Up Table (LUT)
    g_menuNormalLut,     //          Index 0: Unselected LUT
    g_menuBrightLut,     //          Index 1: Selected LUT
  },
  g_menuRleEntries       // data   - Pointer to the beginning of the RLE data
};
