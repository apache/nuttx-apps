/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/twm4nx/apps/nxterm_config.hxx
// NxTerm configuration settings
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

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_APPS_NXTERM_CONFIG_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_APPS_NXTERM_CONFIG_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>
#include <debug.h>

#include "graphics/nxglyphs.hxx"
#include "graphics/nxwidgets/crlepalettebitmap.hxx"
#include "graphics/twm4nx/twm4nx_config.hxx"

#ifdef CONFIG_TWM4NX_NXTERM

/////////////////////////////////////////////////////////////////////////////
// Pre-Processor Definitions
/////////////////////////////////////////////////////////////////////////////

// General Configuration ////////////////////////////////////////////////////

/**
 * Required settings:
 *
 * CONFIG_HAVE_CXX         : C++ support is required
 * CONFIG_NX               : NX must enabled
 * CONFIG_NXTERM=y         : For NxTerm support
 * CONFIG_NXTERM_NXKBDIN=y : May be needed if Twm4Nx is managing the keyboard
 */

#ifndef CONFIG_HAVE_CXX
#  error C++ support is required (CONFIG_HAVE_CXX)
#endif

#ifndef CONFIG_NX
#  error NX support is required (CONFIG_NX)
#endif

#ifndef CONFIG_NXTERM
#  warning NxTerm support is required (CONFIG_NXTERM)
#endif

// Keyboard input can come from either /dev/console or via Twm4Nx.  If
// keyboard input comes from Twm4Nx, then CONFIG_NXTERM_NXKBDIN must be
// defined to support injection of keyboard input from applications.
// Otherwise, CONFIG_NXTERM_NXKBDIN must not be defined to support use of
// /dev/console for keyboard input.

#if !defined(CONFIG_TWM4NX_NOKEYBOARD) && !defined(CONFIG_NXTERM_NXKBDIN)
#  warning Nxterm needs CONFIG_NXTERM_NXKBDIN for keyboard input
#elif defined(CONFIG_TWM4NX_NOKEYBOARD) && defined(CONFIG_NXTERM_NXKBDIN)
#  warning Nxterm has no keyboard input.  Undefine CONFIG_NXTERM_NXKBDIN
#endif

// NxTerm Window /////////////////////////////////////////////////////////////

/**
 * NxTerm Window Configuration
 *
 * CONFIG_TWM4NX_NXTERM_PRIO - Priority of the NxTerm task.  Default:
 *   SCHED_PRIORITY_DEFAULT.  NOTE:  This priority should be less than
 *   CONFIG_NXSTART_SERVERPRIO or else there may be data overrun errors.
 *   Such errors would most likely appear as duplicated rows of data on the
 *   display.
 * CONFIG_TWM4NX_NXTERM_STACKSIZE - The stack size to use when starting the
 *   NxTerm task.  Default: 2048 bytes.
 * CONFIG_TWM4NX_NXTERM_WCOLOR - The color of the NxTerm window background.
 *   Default:  MKRGB(192,192,192)
 * CONFIG_TWM4NX_NXTERM_FONTCOLOR - The color of the fonts to use in the
 *   NxTerm window.  Default: MKRGB(0,0,0)
 * CONFIG_TWM4NX_NXTERM_FONTID - The ID of the font to use in the NxTerm
 *   window.  Default: CONFIG_TWM4NX_DEFAULT_FONTID
 * CONFIG_TWM4NX_NXTERM_ICON - The glyph to use as the NxTerm icon
 */

// Tasking

#ifndef CONFIG_TWM4NX_NXTERM_PRIO
#  define CONFIG_TWM4NX_NXTERM_PRIO       SCHED_PRIORITY_DEFAULT
#endif

#if CONFIG_NXSTART_SERVERPRIO <= CONFIG_TWM4NX_NXTERM_PRIO
#  warning "CONFIG_NXSTART_SERVERPRIO <= CONFIG_TWM4NX_NXTERM_PRIO"
#  warning" -- This can result in data overrun errors"
#endif

#ifndef CONFIG_TWM4NX_NXTERM_STACKSIZE
#  define CONFIG_TWM4NX_NXTERM_STACKSIZE  2048
#endif

// Colors

#ifndef CONFIG_TWM4NX_NXTERM_WCOLOR
#  ifdef CONFIG_TWM4NX_CLASSIC
#    define CONFIG_TWM4NX_NXTERM_WCOLOR   CONFIG_TWM4NX_DEFAULT_BACKGROUNDCOLOR
#  else
#    define CONFIG_TWM4NX_NXTERM_WCOLOR   MKRGB(188,152,106)
#  endif
#endif

#ifndef CONFIG_TWM4NX_NXTERM_FONTCOLOR
#  ifdef CONFIG_TWM4NX_CLASSIC
#    define CONFIG_TWM4NX_NXTERM_FONTCOLOR  CONFIG_TWM4NX_DEFAULT_FONTCOLOR
#  else
#    define CONFIG_TWM4NX_NXTERM_FONTCOLOR  MKRGB(112,70,14)
#  endif
#endif

// Font ID

#ifndef CONFIG_TWM4NX_NXTERM_FONTID
#  define CONFIG_TWM4NX_NXTERM_FONTID     NXFONT_DEFAULT
#endif

// The NxTerm window glyph

#ifndef CONFIG_TWM4NX_NXTERM_ICON
#  define CONFIG_TWM4NX_NXTERM_ICON       NXWidgets::g_cmdBitmap
#endif

#endif // CONFIG_TWM4NX_NXTERM
#endif // __APPS_INCLUDE_GRAPHICS_TWM4NX_APPS_NXTERM_CONFIG_HXX
