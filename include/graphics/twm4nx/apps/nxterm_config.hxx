/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/twm4nx/apps/nxterm_config.hxx
//
// Licensed to the Apache Software Foundation (ASF) under one or more
// contributor license agreements.  See the NOTICE file distributed with
// this work for additional information regarding copyright ownership.  The
// ASF licenses this file to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance with the
// License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations
// under the License.
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
