/****************************************************************************
 * apps/include/graphics/nxwidgets/nxglyphs.hxx
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
 ****************************************************************************/

#ifndef __APPS_INCLUDE_GRAPHICS_NXGLYPHS_HXX
#define __APPS_INCLUDE_GRAPHICS_NXGLYPHS_HXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/nxwidgets/nxconfig.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/* Default background color */

#ifndef CONFIG_NXGLYPHS_BACKGROUNDCOLOR
#  define CONFIG_NXGLYPHS_BACKGROUNDCOLOR  MKRGB(148,189,215)
#endif

/****************************************************************************
 * Bitmap Glyph References
 ****************************************************************************/

#if defined(__cplusplus)

namespace NXWidgets
{
  struct SBitmap;

  // Bitmaps used by NxWidgets
  // Global RLE Paletted Bitmaps

  extern const struct SRlePaletteBitmap g_nuttxBitmap160x160;
  extern const struct SRlePaletteBitmap g_nuttxBitmap320x320;

  // Global Simple Bitmaps

  extern const struct SBitmap g_screenDepthUp;
  extern const struct SBitmap g_screenDepthDown;
  extern const struct SBitmap g_windowClose;
  extern const struct SBitmap g_windowDepthUp;
  extern const struct SBitmap g_windowDepthDown;
  extern const struct SBitmap g_radioButtonOn;
  extern const struct SBitmap g_radioButtonOff;
  extern const struct SBitmap g_radioButtonMu;
  extern const struct SBitmap g_checkBoxOff;
  extern const struct SBitmap g_checkBoxOn;
  extern const struct SBitmap g_checkBoxMu;
  extern const struct SBitmap g_screenFlipUp;
  extern const struct SBitmap g_screenFlipDown;
  extern const struct SBitmap g_arrowUp;
  extern const struct SBitmap g_arrowDown;
  extern const struct SBitmap g_arrowLeft;
  extern const struct SBitmap g_arrowRight;
  extern const struct SBitmap g_cycle;
  extern const struct SBitmap g_backspace;
  extern const struct SBitmap g_return;
  extern const struct SBitmap g_shift;
  extern const struct SBitmap g_capslock;
  extern const struct SBitmap g_control;

  // Bitmaps used by NxWM, Twm4Nx, and SLcd
  // Global RLE Paletted Bitmaps

  extern const struct SRlePaletteBitmap g_calculatorBitmap;
  extern const struct SRlePaletteBitmap g_calibrationBitmap;
  extern const struct SRlePaletteBitmap g_cmdBitmap;
  extern const struct SRlePaletteBitmap g_menuBitmap;
  extern const struct SRlePaletteBitmap g_menu2Bitmap;
  extern const struct SRlePaletteBitmap g_resizeBitmap;
  extern const struct SRlePaletteBitmap g_resize2Bitmap;
  extern const struct SRlePaletteBitmap g_nxiconBitmap;
  extern const struct SRlePaletteBitmap g_lcdClockBitmap;

  // Used by NxWM media player

  extern const struct SRlePaletteBitmap g_mediaplayerBitmap;
  extern const struct SRlePaletteBitmap g_mplayerFwdBitmap;
  extern const struct SRlePaletteBitmap g_mplayerPlayBitmap;
  extern const struct SRlePaletteBitmap g_mplayerPauseBitmap;
  extern const struct SRlePaletteBitmap g_mplayerRewBitmap;
  extern const struct SRlePaletteBitmap g_mplayerVolBitmap;
  extern const struct SRlePaletteBitmap g_minimizeBitmap;
  extern const struct SRlePaletteBitmap g_minimize2Bitmap;
  extern const struct SRlePaletteBitmap g_playBitmap;
  extern const struct SRlePaletteBitmap g_stopBitmap;
  extern const struct SRlePaletteBitmap g_stop2Bitmap;
}

#endif // __cplusplus

#endif // __APPS_INCLUDE_GRAPHICS_NXGLYPHS_HXX
