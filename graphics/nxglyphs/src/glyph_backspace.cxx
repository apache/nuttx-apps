/****************************************************************************
 * apps/graphics/nxglyphs/src/glyph_backspace.cxx
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
 ****************************************************************************
 *
 * Portions of this package derive from Woopsi (http://woopsi.org/) and
 * portions are original efforts.  It is difficult to determine at this
 * point what parts are original efforts and which parts derive from Woopsi.
 * However, in any event, the work of  Antony Dzeryn will be acknowledged
 * in most NxWidget files.  Thanks Antony!
 *
 *   Copyright (c) 2007-2011, Antony Dzeryn
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * * Neither the names "Woopsi", "Simian Zombie" nor the
 *   names of its contributors may be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Antony Dzeryn ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Antony Dzeryn BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>

#include <nuttx/nx/nxglib.h>
#include <nuttx/video/fb.h>
#include <nuttx/video/rgbcolors.h>

#include "graphics/nxwidgets/nxconfig.hxx"
#include "graphics/nxwidgets/cbitmap.hxx"
#include "graphics/nxglyphs.hxx"

#if CONFIG_NXWIDGETS_BPP != 8 // No support for 8-bit color format

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Bitmap Data
 ****************************************************************************/

using namespace NXWidgets;

#if CONFIG_NXWIDGETS_BPP == 16
#  define COLOR_FMT FB_FMT_RGB16_565
#  define RGB16_TRANSP 0x0000

static const uint16_t g_backspaceGlyph[] =
{
  RGB16_TRANSP,  RGB16_TRANSP,  RGB16_TRANSP,  RGB16_TRANSP,  RGB16_TRANSP,  RGB16_TRANSP,  RGB16_TRANSP,
  RGB16_TRANSP,  RGB16_TRANSP,  RGB16_TRANSP,  RGB16_TRANSP,  RGB16_DARKRED, RGB16_TRANSP,  RGB16_TRANSP,
  RGB16_TRANSP,  RGB16_TRANSP,  RGB16_TRANSP,  RGB16_DARKRED, RGB16_DARKRED, RGB16_TRANSP,  RGB16_TRANSP,
  RGB16_TRANSP,  RGB16_TRANSP,  RGB16_DARKRED, RGB16_DARKRED, RGB16_TRANSP,  RGB16_TRANSP,  RGB16_TRANSP,
  RGB16_TRANSP,  RGB16_DARKRED, RGB16_DARKRED, RGB16_DARKRED, RGB16_DARKRED, RGB16_DARKRED, RGB16_DARKRED,
  RGB16_TRANSP,  RGB16_DARKRED, RGB16_DARKRED, RGB16_DARKRED, RGB16_DARKRED, RGB16_DARKRED, RGB16_DARKRED,
  RGB16_TRANSP,  RGB16_TRANSP,  RGB16_DARKRED, RGB16_DARKRED, RGB16_TRANSP,  RGB16_TRANSP,  RGB16_TRANSP,
  RGB16_TRANSP,  RGB16_TRANSP,  RGB16_TRANSP,  RGB16_DARKRED, RGB16_DARKRED, RGB16_TRANSP,  RGB16_TRANSP,
  RGB16_TRANSP,  RGB16_TRANSP,  RGB16_TRANSP,  RGB16_TRANSP,  RGB16_DARKRED, RGB16_TRANSP,  RGB16_TRANSP,
  RGB16_TRANSP,  RGB16_TRANSP,  RGB16_TRANSP,  RGB16_TRANSP,  RGB16_TRANSP,  RGB16_TRANSP,  RGB16_TRANSP,
};

#elif CONFIG_NXWIDGETS_BPP == 24 || CONFIG_NXWIDGETS_BPP == 32
#  define COLOR_FMT FB_FMT_RGB24
#  define RGB24_TRANSP 0x00000000

static const uint32_t g_backspaceGlyph[] =
{
  RGB24_TRANSP,  RGB24_TRANSP,  RGB24_TRANSP,  RGB24_TRANSP,  RGB24_TRANSP,  RGB24_TRANSP,  RGB24_TRANSP,
  RGB24_TRANSP,  RGB24_TRANSP,  RGB24_TRANSP,  RGB24_TRANSP,  RGB24_DARKRED, RGB24_TRANSP,  RGB24_TRANSP,
  RGB24_TRANSP,  RGB24_TRANSP,  RGB24_TRANSP,  RGB24_DARKRED, RGB24_DARKRED, RGB24_TRANSP,  RGB24_TRANSP,
  RGB24_TRANSP,  RGB24_TRANSP,  RGB24_DARKRED, RGB24_DARKRED, RGB24_TRANSP,  RGB24_TRANSP,  RGB24_TRANSP,
  RGB24_TRANSP,  RGB24_DARKRED, RGB24_DARKRED, RGB24_DARKRED, RGB24_DARKRED, RGB24_DARKRED, RGB24_DARKRED,
  RGB24_TRANSP,  RGB24_DARKRED, RGB24_DARKRED, RGB24_DARKRED, RGB24_DARKRED, RGB24_DARKRED, RGB24_DARKRED,
  RGB24_TRANSP,  RGB24_TRANSP,  RGB24_DARKRED, RGB24_DARKRED, RGB24_TRANSP,  RGB24_TRANSP,  RGB24_TRANSP,
  RGB24_TRANSP,  RGB24_TRANSP,  RGB24_TRANSP,  RGB24_DARKRED, RGB24_DARKRED, RGB24_TRANSP,  RGB24_TRANSP,
  RGB24_TRANSP,  RGB24_TRANSP,  RGB24_TRANSP,  RGB24_TRANSP,  RGB24_DARKRED, RGB24_TRANSP,  RGB24_TRANSP,
  RGB24_TRANSP,  RGB24_TRANSP,  RGB24_TRANSP,  RGB24_TRANSP,  RGB24_TRANSP,  RGB24_TRANSP,  RGB24_TRANSP,
};

#else
#  warning "Other pixel depths not yet supported"
#endif

/****************************************************************************
 * Public Bitmap Structure Definitions
 ****************************************************************************/

const struct SBitmap NXWidgets::g_backspace =
{
  CONFIG_NXWIDGETS_BPP,              // bpp    - Bits per pixel
  COLOR_FMT,                         // fmt    - Color format
  8,                                 // width  - Width in pixels
  10,                                // height - Height in rows
  (8*CONFIG_NXWIDGETS_BPP + 7) / 8,  // stride - Width in bytes
  g_backspaceGlyph                   // data   - Pointer to the beginning of pixel data
};

#endif // CONFIG_NXWIDGETS_BPP != 8
