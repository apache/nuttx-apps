/********************************************************************************************
 * apps/graphics/nxglyphs/src/glyph_xxxxxx.cxx
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

#define BITMAP_NROWS     42
#define BITMAP_NCOLUMNS  42
#define BITMAP_NLUTCODES 3

/********************************************************************************************
 * Private Bitmap Data
 ********************************************************************************************/

using namespace NXWidgets;

#if CONFIG_NXWIDGETS_BPP == 24 ||  CONFIG_NXWIDGETS_BPP == 32
// RGB24 (8-8-8) Colors

static const uint32_t g_menu2NormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR, 0xfcfcfc, 0xd8fcfc,  /* Codes 0-2 */
};

static const uint32_t g_menu2BrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR, 0xfcfcfc, 0xe1fcfc,  /* Codes 0-2 */
};

#elif CONFIG_NXWIDGETS_BPP == 16
// RGB16 (565) Colors (four of the colors in this map are duplicates)

static const uint16_t g_menu2NormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR, 0xffff, 0xdfff,  /* Codes 0-2 */
};

static const uint16_t g_menu2BrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR, 0xffff, 0xe7ff,  /* Codes 0-2 */
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

static const uint8_t g_menu2NormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR, 0xfc, 0xf1,  /* Codes 0-2 */
}

static const uint8_t g_menu2BrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR, 0xfc, 0xf3,  /* Codes 0-2 */
};

#  else /* CONFIG_NXWIDGETS_GREYSCALE */
// RGB8 (332) Colors

static const nxgl_mxpixel_t g_menu2NormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR, 0xff, 0xdf,  /* Codes 0-2 */
};

static const uint8_t g_menu2BrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR, 0xff, 0xff,  /* Codes 0-2 */
};

#  endif
#else
#  error Unsupported pixel format
#endif

static const struct SRlePaletteBitmapEntry g_menu2RleEntries[] =
{
  { 42,   0},                                                                                        // Row 0
  { 42,   0},                                                                                        // Row 1
  { 42,   0},                                                                                        // Row 2
  { 42,   0},                                                                                        // Row 3
  { 42,   0},                                                                                        // Row 4
  { 42,   0},                                                                                        // Row 5
  { 42,   0},                                                                                        // Row 6
  { 42,   0},                                                                                        // Row 7
  {  7,   0}, { 27,   1}, {  8,   0},                                                                // Row 8
  {  7,   0}, {  1,   1}, { 25,   2}, {  1,   1}, {  1,   2}, {  7,   0},                            // Row 9
  {  7,   0}, {  1,   1}, {  1,   2}, { 24,   0}, {  1,   1}, {  1,   2}, {  7,   0},                // Row 10
  {  7,   0}, {  1,   1}, {  1,   2}, { 24,   0}, {  1,   1}, {  1,   2}, {  7,   0},                // Row 11
  {  7,   0}, {  1,   1}, {  1,   2}, { 24,   0}, {  1,   1}, {  1,   2}, {  7,   0},                // Row 12
  {  7,   0}, {  1,   1}, {  1,   2}, { 24,   0}, {  1,   1}, {  1,   2}, {  7,   0},                // Row 13
  {  7,   0}, {  1,   1}, {  1,   2}, { 24,   0}, {  1,   1}, {  1,   2}, {  7,   0},                // Row 14
  {  7,   0}, { 27,   1}, {  1,   2}, {  7,   0},                                                    // Row 15
  {  7,   0}, {  1,   1}, { 25,   2}, {  1,   1}, {  1,   2}, {  7,   0},                            // Row 16
  {  7,   0}, {  1,   1}, {  1,   2}, { 24,   0}, {  1,   1}, {  1,   2}, {  7,   0},                // Row 17
  {  7,   0}, {  1,   1}, {  1,   2}, { 24,   0}, {  1,   1}, {  1,   2}, {  7,   0},                // Row 18
  {  7,   0}, {  1,   1}, {  1,   2}, { 24,   0}, {  1,   1}, {  1,   2}, {  7,   0},                // Row 19
  {  7,   0}, {  1,   1}, {  1,   2}, { 24,   0}, {  1,   1}, {  1,   2}, {  7,   0},                // Row 20
  {  7,   0}, {  1,   1}, {  1,   2}, { 24,   0}, {  1,   1}, {  1,   2}, {  7,   0},                // Row 21
  {  7,   0}, { 27,   1}, {  1,   2}, {  7,   0},                                                    // Row 22
  {  7,   0}, {  1,   1}, { 25,   2}, {  1,   1}, {  1,   2}, {  7,   0},                            // Row 23
  {  7,   0}, {  1,   1}, {  1,   2}, { 24,   0}, {  1,   1}, {  1,   2}, {  7,   0},                // Row 24
  {  7,   0}, {  1,   1}, {  1,   2}, { 24,   0}, {  1,   1}, {  1,   2}, {  7,   0},                // Row 25
  {  7,   0}, {  1,   1}, {  1,   2}, { 24,   0}, {  1,   1}, {  1,   2}, {  7,   0},                // Row 26
  {  7,   0}, {  1,   1}, {  1,   2}, { 24,   0}, {  1,   1}, {  1,   2}, {  7,   0},                // Row 27
  {  7,   0}, { 27,   1}, {  1,   2}, {  7,   0},                                                    // Row 28
  {  7,   0}, {  1,   1}, { 25,   2}, {  1,   1}, {  1,   2}, {  7,   0},                            // Row 29
  {  7,   0}, {  1,   1}, {  1,   2}, { 24,   0}, {  1,   1}, {  1,   2}, {  7,   0},                // Row 30
  {  7,   0}, {  1,   1}, {  1,   2}, { 24,   0}, {  1,   1}, {  1,   2}, {  7,   0},                // Row 31
  {  7,   0}, {  1,   1}, {  1,   2}, { 24,   0}, {  1,   1}, {  1,   2}, {  7,   0},                // Row 32
  {  7,   0}, {  1,   1}, {  1,   2}, { 24,   0}, {  1,   1}, {  1,   2}, {  7,   0},                // Row 33
  {  7,   0}, { 27,   1}, {  1,   2}, {  7,   0},                                                    // Row 34
  {  8,   0}, { 27,   2}, {  7,   0},                                                                // Row 35
  { 42,   0},                                                                                        // Row 36
  { 42,   0},                                                                                        // Row 37
  { 42,   0},                                                                                        // Row 38
  { 42,   0},                                                                                        // Row 39
  { 42,   0},                                                                                        // Row 40
  { 42,   0},                                                                                        // Row 41
 };

/********************************************************************************************
 * Public Bitmap Structure Definitions
 ********************************************************************************************/

const struct SRlePaletteBitmap NXWidgets::g_menu2Bitmap =
{
  CONFIG_NXWIDGETS_BPP,  // bpp    - Bits per pixel
  CONFIG_NXWIDGETS_FMT,  // fmt    - Color format
  BITMAP_NLUTCODES,      // nlut   - Number of colors in the lLook-Up Table (LUT)
  BITMAP_NCOLUMNS,       // width  - Width in pixels
  BITMAP_NROWS,          // height - Height in rows
  {                      // lut    - Pointer to the beginning of the Look-Up Table (LUT)
    g_menu2NormalLut,    //          Index 0: Unselected LUT
    g_menu2BrightLut,    //          Index 1: Selected LUT
  },
  g_menu2RleEntries      // data   - Pointer to the beginning of the RLE data
};
