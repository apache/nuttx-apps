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

#define BITMAP_NROWS     21
#define BITMAP_NCOLUMNS  21
#define BITMAP_NLUTCODES 2

/********************************************************************************************
 * Private Bitmap Data
 ********************************************************************************************/

using namespace NXWidgets;

#if CONFIG_NXWIDGETS_BPP == 24 ||  CONFIG_NXWIDGETS_BPP == 32
// RGB24 (8-8-8) Colors

static const uint32_t g_menu2NormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR, 0xfcfcfc,  /* Codes 0-1 */
};

static const uint32_t g_menu2BrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR, 0xfcfcfc,  /* Codes 0-1 */
};

#elif CONFIG_NXWIDGETS_BPP == 16
// RGB16 (565) Colors (four of the colors in this map are duplicates)

static const uint16_t g_menu2NormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR, 0xffff,  /* Codes 0-1 */
};

static const uint16_t g_menu2BrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR, 0xffff,  /* Codes 0-1 */
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
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR, 0xfc,  /* Codes 0-1 */
}

static const uint8_t g_menu2BrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR, 0xfc,  /* Codes 0-1 */
};

#  else /* CONFIG_NXWIDGETS_GREYSCALE */
// RGB8 (332) Colors

static const nxgl_mxpixel_t g_menu2NormalLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR, 0xff,  /* Codes 0-1 */
};

static const uint8_t g_menu2BrightLut[BITMAP_NLUTCODES] =
{
  CONFIG_NXGLYPHS_BACKGROUNDCOLOR, 0xff,  /* Codes 0-1 */
};

#  endif
#else
#  error Unsupported pixel format
#endif

static const struct SRlePaletteBitmapEntry g_menu2RleEntries[] =
{
  { 21,   0},                                                                                        // Row 0
  { 21,   0},                                                                                        // Row 1
  { 21,   0},                                                                                        // Row 2
  { 21,   0},                                                                                        // Row 3
  {  4,   0}, { 14,   1}, {  3,   0},                                                                // Row 4
  {  4,   0}, {  1,   1}, { 12,   0}, {  1,   1}, {  3,   0},                                        // Row 5
  {  4,   0}, {  1,   1}, { 12,   0}, {  1,   1}, {  3,   0},                                        // Row 6
  {  4,   0}, {  1,   1}, { 12,   0}, {  1,   1}, {  3,   0},                                        // Row 7
  {  4,   0}, { 14,   1}, {  3,   0},                                                                // Row 8
  {  4,   0}, {  1,   1}, { 12,   0}, {  1,   1}, {  3,   0},                                        // Row 9
  {  4,   0}, {  1,   1}, { 12,   0}, {  1,   1}, {  3,   0},                                        // Row 10
  {  4,   0}, { 14,   1}, {  3,   0},                                                                // Row 11
  {  4,   0}, {  1,   1}, { 12,   0}, {  1,   1}, {  3,   0},                                        // Row 12
  {  4,   0}, {  1,   1}, { 12,   0}, {  1,   1}, {  3,   0},                                        // Row 13
  {  4,   0}, { 14,   1}, {  3,   0},                                                                // Row 14
  {  4,   0}, {  1,   1}, { 12,   0}, {  1,   1}, {  3,   0},                                        // Row 15
  {  4,   0}, {  1,   1}, { 12,   0}, {  1,   1}, {  3,   0},                                        // Row 16
  {  4,   0}, { 14,   1}, {  3,   0},                                                                // Row 17
  { 21,   0},                                                                                        // Row 18
  { 21,   0},                                                                                        // Row 19
  { 21,   0},                                                                                        // Row 20
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
