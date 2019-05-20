/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/twm4nx/apps/calib_config.hxx
// Calibration configuration settings
//
//   Copyright (C) 2019 Gregory Nutt. All rights reserved.
//   Author: Gregory Nutt <gnutt@nuttx.org>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the
//    distribution.
// 3. Neither the name NuttX nor the names of its contributors may be
//    used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
// AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_APPS_CALIB_CONFIG_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_APPS_CALIB_CONFIG_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>
#include <debug.h>

#include "graphics/nxwidgets/nxconfig.hxx"
#include "graphics/twm4nx/twm4nx_config.hxx"

#ifdef CONFIG_TWM4NX_CALIBRATION

/////////////////////////////////////////////////////////////////////////////
// Pre-Processor Definitions
/////////////////////////////////////////////////////////////////////////////

// General Configuration ////////////////////////////////////////////////////

/**
 * Required settings:
 *
 * CONFIG_HAVE_CXX         : C++ support is required
 * CONFIG_NX               : NX must enabled
 */

#ifndef CONFIG_HAVE_CXX
#  error C++ support is required (CONFIG_HAVE_CXX)
#endif

#ifndef CONFIG_NX
#  error NX support is required (CONFIG_NX)
#endif

// Calibration Window ///////////////////////////////////////////////////////

/**
 * Calibration display settings:
 *
 * CONFIG_TWM4NX_CALIBRATION_BACKGROUNDCOLOR - The background color of the
 *   touchscreen calibration display.  Default:  Same as
 *   CONFIG_TWM4NX_DEFAULT_BACKGROUNDCOLOR
 * CONFIG_TWM4NX_CALIBRATION_LINECOLOR - The color of the lines used in the
 *   touchscreen calibration display.  Default:  MKRGB(0, 0, 128) (dark blue)
 * CONFIG_TWM4NX_CALIBRATION_CIRCLECOLOR - The color of the circle in the
 *   touchscreen calibration display.  Default:  MKRGB(255, 255, 255) (white)
 * CONFIG_TWM4NX_CALIBRATION_TOUCHEDCOLOR - The color of the circle in the
 *   touchscreen calibration display after the touch is recorder.  Default:
 *   MKRGB(255, 255, 96) (very light yellow)
 * CONFIG_TWM4NX_CALIBRATION_FONTID - Use this default NxWidgets font ID
 *   instead of the system font ID (NXFONT_DEFAULT).
 * CONFIG_TWM4NX_CALIBRATION_ICON - The ICON to use for the touchscreen
 *   calibration application.  Default:  NXWidgets::g_calibrationBitmap
 * CONFIG_TWM4NX_CALIBRATION_SIGNO - The real-time signal used to wake up the
 *   touchscreen calibration thread.  Default: 5
 * CONFIG_TWM4NX_CALIBRATION_LISTENERPRIO - Priority of the calibration
 *   listener thread.  Default: SCHED_PRIORITY_DEFAULT
 * CONFIG_TWM4NX_CALIBRATION_LISTENERSTACK - Calibration listener thread
 *   stack size.  Default 2048
 * CONFIG_TWM4NX_CALIBRATION_MARGIN
 *   The Calibration display consists of a target press offset from the edges
 *   of the display by this number of pixels (in the horizontal direction)
 *   or rows (in the vertical).  The closer that you can comfortably
 *   position the press positions to the edge, the more accurate will be the
 *   linear interpolation (provide that the hardware provides equally good
 *   measurements near the edges).
 */

#ifndef CONFIG_TWM4NX_CALIBRATION_BACKGROUNDCOLOR
#  define CONFIG_TWM4NX_CALIBRATION_BACKGROUNDCOLOR \
          CONFIG_TWM4NX_DEFAULT_BACKGROUNDCOLOR
#endif

#ifndef CONFIG_TWM4NX_CALIBRATION_LINECOLOR
#  define CONFIG_TWM4NX_CALIBRATION_LINECOLOR MKRGB(0, 0, 128)
#endif

#ifndef CONFIG_TWM4NX_CALIBRATION_CIRCLECOLOR
#  define CONFIG_TWM4NX_CALIBRATION_CIRCLECOLOR MKRGB(255, 255, 255)
#endif

#ifndef CONFIG_TWM4NX_CALIBRATION_TOUCHEDCOLOR
#  define CONFIG_TWM4NX_CALIBRATION_TOUCHEDCOLOR MKRGB(255, 255, 96)
#endif

#ifndef CONFIG_TWM4NX_CALIBRATION_FONTID
#  define CONFIG_TWM4NX_CALIBRATION_FONTID NXFONT_DEFAULT
#endif

#ifndef CONFIG_TWM4NX_CALIBRATION_ICON
#  define CONFIG_TWM4NX_CALIBRATION_ICON NXWidgets::g_calibrationBitmap
#endif

#ifndef CONFIG_TWM4NX_CALIBRATION_SIGNO
#  define CONFIG_TWM4NX_CALIBRATION_SIGNO 5
#endif

#ifndef CONFIG_TWM4NX_CALIBRATION_LISTENERPRIO
#  define CONFIG_TWM4NX_CALIBRATION_LISTENERPRIO SCHED_PRIORITY_DEFAULT
#endif

#ifndef CONFIG_TWM4NX_CALIBRATION_LISTENERSTACK
#  define CONFIG_TWM4NX_CALIBRATION_LISTENERSTACK 2048
#endif

#ifndef CONFIG_TWM4NX_CALIBRATION_MARGIN
#  define CONFIG_TWM4NX_CALIBRATION_MARGIN 40
#endif

// Calibration sample averaging

#ifndef CONFIG_TWM4NX_CALIBRATION_AVERAGE
#  undef CONFIG_TWM4NX_CALIBRATION_AVERAGE
#  undef CONFIG_TWM4NX_CALIBRATION_NSAMPLES
#  define CONFIG_TWM4NX_CALIBRATION_NSAMPLES 1
#  undef CONFIG_TWM4NX_CALIBRATION_DISCARD_MINMAX
#endif

#if !defined(CONFIG_TWM4NX_CALIBRATION_NSAMPLES) || \
    CONFIG_TWM4NX_CALIBRATION_NSAMPLES < 2
#  undef CONFIG_TWM4NX_CALIBRATION_AVERAGE
#  undef CONFIG_TWM4NX_CALIBRATION_NSAMPLES
#  define CONFIG_TWM4NX_CALIBRATION_NSAMPLES 1
#  undef CONFIG_TWM4NX_CALIBRATION_DISCARD_MINMAX
#endif

#if CONFIG_TWM4NX_CALIBRATION_NSAMPLES < 3
#  undef CONFIG_TWM4NX_CALIBRATION_DISCARD_MINMAX
#endif

#if CONFIG_TWM4NX_CALIBRATION_NSAMPLES > 255
#  define CONFIG_TWM4NX_CALIBRATION_NSAMPLES 255
#endif

#ifdef CONFIG_TWM4NX_CALIBRATION_DISCARD_MINMAX
#  define TWM4NX_CALIBRATION_NAVERAGE (CONFIG_TWM4NX_CALIBRATION_NSAMPLES - 2)
#else
#  define TWM4NX_CALIBRATION_NAVERAGE CONFIG_TWM4NX_CALIBRATION_NSAMPLES
#endif

#endif // CONFIG_TWM4NX_CALIBRATION
#endif // __APPS_INCLUDE_GRAPHICS_TWM4NX_APPS_CALIB_CONFIG_HXX
