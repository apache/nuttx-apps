/****************************************************************************
 * apps/include/graphics/nxwidgets/nxglyphs.hxx
 *
 *   Copyright (C) 2012, 2019 Gregory Nutt. All rights reserved.
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
