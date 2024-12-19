/****************************************************************************
 * apps/include/graphics/nxwidgets/nxconfig.hxx
 *
 * SPDX-License-Identifier: Apache-2.0
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

#ifndef __APPS_INCLUDE_GRAPHICS_NXWIDGETS_NXCONFIG_HXX
#define __APPS_INCLUDE_GRAPHICS_NXWIDGETS_NXCONFIG_HXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>

#include <nuttx/video/rgbcolors.h>
#include <nuttx/nx/nxfonts.h>

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/
/* NX Configuration *********************************************************/
/**
 * Prerequisites:
 *
 * CONFIG_HAVE_CXX=y   : C++ support is required
 * CONFIG_NX=y         : NX graphics support must be enabled
 * CONFIG_NX_XYINPUT=y : Required to enable NX mouse/touchscreen support
 * CONFIG_NX_KBD=y     : Required to enabled NX keyboard support
 * CONFIG_NX_NPLANES=1 : Only a single video plane is supported
 *
 * NX Server/Device Configuration
 *
 * CONFIG_NXSTART_SERVERPRIO - Priority of the NX server.
 *   Default: (SCHED_PRIORITY_DEFAULT+10).  NOTE:  Of the three priority
 *   definitions here, CONFIG_NXSTART_SERVERPRIO should have the highest
 *   priority to avoid data overrun race conditions. Such errors would most
 *   likely appear as duplicated rows of data on the display.
 * CONFIG_NXWIDGETS_CLIENTPRIO - The thread that calls CNxServer::connect()
 *   will be re-prioritized to this priority.  Default:
 *   SCHED_PRIORITY_DEFAULT
 * CONFIG_NXWIDGETS_LISTENERPRIO - Priority of the NX event listener thread.
 *   Default: SCHED_PRIORITY_DEFAULT
 * CONFIG_NXWIDGETS_LISTENERSTACK - NX listener thread stack size (in multi-user
 * mode).  Default 2048
 * CONFIG_NXWIDGET_EVENTWAIT - Build in support for external window event, modal
 *   loop management logic.  This includes methods to wait for windows events
 *   to occur so that looping logic can sleep until something interesting
 *   happens with the window.
 *
 * NXWidget Configuration
 *
 * CONFIG_NXWIDGETS_BPP - Supported bits-per-pixel {8, 16, 24, 32}.  Default:
 *   The smallest BPP configuration supported by NX.
 * CONFIG_NXWIDGETS_SIZEOFCHAR - Size of character {1 or 2 bytes}.  Default
 *   Determined by CONFIG_NXWIDGETS_SIZEOFCHAR
 *
 * NXWidget Default Values
 *
 * CONFIG_NXWIDGETS_DEFAULT_FONTID - Default font ID.  Default: NXFONT_DEFAULT
 * CONFIG_NXWIDGETS_TNXARRAY_INITIALSIZE, CONFIG_NXWIDGETS_TNXARRAY_SIZEINCREMENT -
 *   Default dynamic array parameters.  Default: 16, 8
 *
 * CONFIG_NXWIDGETS_DEFAULT_BACKGROUNDCOLOR - Normal background color.  Default:
 *   MKRGB(148,189,215)
 * CONFIG_NXWIDGETS_DEFAULT_SELECTEDBACKGROUNDCOLOR - Default selected background
 *   color.  Default: MKRGB(206,227,241)
 * CONFIG_NXWIDGETS_DEFAULT_SHINEEDGECOLOR - Shiny side boarder color. Default
 *   MKRGB(248,248,248)
 * CONFIG_NXWIDGETS_DEFAULT_SHADOWEDGECOLOR - Shadowed side border color.
 *   Default: MKRGB(35,58,73)
 * CONFIG_NXWIDGETS_DEFAULT_HIGHLIGHTCOLOR - Highlight color.  Default:
 *   MKRGB(192,192,192)
 * CONFIG_NXWIDGETS_DEFAULT_DISABLEDTEXTCOLOR - Text color on a disabled widget:
 *   Default: MKRGB(192,192,192)
 * CONFIG_NXWIDGETS_DEFAULT_ENABLEDTEXTCOLOR - Text color on a enabled widget:
 *   Default: MKRGB(248,248,248)
 * CONFIG_NXWIDGETS_DEFAULT_SELECTEDTEXTCOLOR - Text color on a selected widget:
 *   Default: MKRGB(0,0,0)
 * CONFIG_NXWIDGETS_DEFAULT_FONTCOLOR - Default font color: Default:
 *   MKRGB(255,255,255)
 * CONFIG_NXWIDGETS_TRANSPARENT_COLOR - Transparent color: Default: MKRGB(0,0,0)
 *
 * Keypad behavior
 *
 * CONFIG_NXWIDGETS_FIRST_REPEAT_TIME - Time taken before a key starts
 *   repeating (in milliseconds).  Default: 500
 * CONFIG_NXWIDGETS_CONTINUE_REPEAT_TIME - Time taken before a repeating key
 *   repeats again (in milliseconds).  Default: 200
 * CONFIG_NXWIDGETS_DOUBLECLICK_TIME - Left button release-press time for
 *   double click (in milliseconds).  Default: 350
 * CONFIG_NXWIDGETS_KBDBUFFER_SIZE - Size of incoming character buffer, i.e.,
 *   the maximum number of characters that can be entered between NX polling
 *   cycles without losing data.
 * CONFIG_NXWIDGETS_CURSORCONTROL_SIZE - Size of incoming cursor control
 *   buffer, i.e., the maximum number of cursor controls that can between
 *   entered by NX polling cycles without losing data.  Default: 4
 */

/* Prerequisites ************************************************************/
/**
 * C++ support is required
 */

#ifndef CONFIG_HAVE_CXX
#  error "C++ support is required (CONFIG_HAVE_CXX)"
#endif

/**
 * NX graphics support must be enabled
 */

#ifndef CONFIG_NX
#  error "NX graphics support is required (CONFIG_NX)"
#endif

/**
 * Required to enable NX mouse/touchscreen support
 */

#ifndef CONFIG_NX_XYINPUT
#  warning "NX mouse/touchscreen support is required (CONFIG_NX_XYINPUT)"
#endif

/**
 * Required to enabled NX keyboard support
 */

#ifndef CONFIG_NX_KBD
#  warning "NX keyboard support is required (CONFIG_NX_KBD)"
#endif

/**
 * Only a single video plane is supported
 */

#ifndef CONFIG_NX_NPLANES
#  define CONFIG_NX_NPLANES 1
#endif

#if CONFIG_NX_NPLANES != 1
#  error "Only a single color plane is supported (CONFIG_NX_NPLANES)"
#endif

/* NxTerm checks.  This just simplifies the conditional compilation by
 * reducing the AND of these three conditions to a single condition.
 */

#if !defined(CONFIG_NX_KBD) || !defined(CONFIG_NXTERM)
#  undef CONFIG_NXTERM_NXKBDIN
#endif

/* NX Server/Device Configuration *******************************************/
/**
 * Priority of the NX server (in multi-user mode)
 */

#ifndef CONFIG_NXWIDGETS_CLIENTPRIO
#  define CONFIG_NXWIDGETS_CLIENTPRIO SCHED_PRIORITY_DEFAULT
#endif

#if CONFIG_NXSTART_SERVERPRIO <= CONFIG_NXWIDGETS_CLIENTPRIO
#  warning "CONFIG_NXSTART_SERVERPRIO <= CONFIG_NXWIDGETS_CLIENTPRIO"
#  warning" -- This can result in data overrun errors"
#endif

/**
 * Priority of the NX event listener thread (in multi-user mode)
 */

#ifndef CONFIG_NXWIDGETS_LISTENERPRIO
#  define CONFIG_NXWIDGETS_LISTENERPRIO SCHED_PRIORITY_DEFAULT
#endif

#if CONFIG_NXSTART_SERVERPRIO <= CONFIG_NXWIDGETS_LISTENERPRIO
#  warning "CONFIG_NXSTART_SERVERPRIO <= CONFIG_NXWIDGETS_LISTENERPRIO"
#  warning" -- This can result in data overrun errors"
#endif

/**
 * NX listener thread stack size (in multi-user mode)
 */

#ifndef CONFIG_NXWIDGETS_LISTENERSTACK
#  define CONFIG_NXWIDGETS_LISTENERSTACK 2048
#endif

/* NXWidget Configuration ***************************************************/
/**
 * Bits per pixel
 */

#ifndef CONFIG_NXWIDGETS_BPP
#  if !defined(CONFIG_NX_DISABLE_8BPP)
#    warning "Assuming 8-bits per pixel, RGB 3:3:2"
#    define CONFIG_NXWIDGETS_BPP 8
#  elif !defined(CONFIG_NX_DISABLE_16BPP)
#    warning "Assuming 16-bits per pixel, RGB 5:6:5"
#    define CONFIG_NXWIDGETS_BPP 16
#  elif !defined(CONFIG_NX_DISABLE_24BPP)
#    warning "Assuming 24-bits per pixel, RGB 8:8:8"
#    define CONFIG_NXWIDGETS_BPP 24
#  elif !defined(CONFIG_NX_DISABLE_32BPP)
#    warning "Assuming 32-bits per pixel, RGB 8:8:8"
#    define CONFIG_NXWIDGETS_BPP 32
#  else
#    error "No supported pixel depth is enabled"
#  endif
#endif

#if CONFIG_NXWIDGETS_BPP == 8
#  ifdef CONFIG_NX_DISABLE_8BPP
#    error "NX 8-bit support is disabled (CONFIG_NX_DISABLE_8BPP)"
#  endif
#  define CONFIG_NXWIDGETS_FMT FB_FMT_RGB8_332
#  define MKRGB                RGBTO8
#  define RGB2RED              RGB8RED
#  define RGB2GREEN            RGB8GREEN
#  define RGB2BLUE             RGB8BLUE
#  define FONT_RENDERER        nxf_convert_8bpp
#elif CONFIG_NXWIDGETS_BPP == 16
#  ifdef CONFIG_NX_DISABLE_16BPP
#    error "NX 16-bit support is disabled (CONFIG_NX_DISABLE_16BPP)"
#  endif
#  define CONFIG_NXWIDGETS_FMT FB_FMT_RGB16_565
#  define MKRGB                RGBTO16
#  define RGB2RED              RGB16RED
#  define RGB2GREEN            RGB16GREEN
#  define RGB2BLUE             RGB16BLUE
#  define FONT_RENDERER        nxf_convert_16bpp
#elif CONFIG_NXWIDGETS_BPP == 24
#  ifdef CONFIG_NX_DISABLE_24BPP
#    error "NX 24-bit support is disabled (CONFIG_NX_DISABLE_24BPP)"
#  endif
#  define CONFIG_NXWIDGETS_FMT FB_FMT_RGB24
#  define MKRGB                RGBTO24
#  define RGB2RED              RGB24RED
#  define RGB2GREEN            RGB24GREEN
#  define RGB2BLUE             RGB24BLUE
#  define FONT_RENDERER        nxf_convert_24bpp
#elif CONFIG_NXWIDGETS_BPP == 32
#  ifdef CONFIG_NX_DISABLE_32BPP
#    error "NX 32-bit support is disabled (CONFIG_NX_DISABLE_32BPP)"
#  endif
#  define CONFIG_NXWIDGETS_FMT FB_FMT_RGB32
#  define MKRGB                RGBTO24
#  define RGB2RED              RGB24RED
#  define RGB2GREEN            RGB24GREEN
#  define RGB2BLUE             RGB24BLUE
#  define FONT_RENDERER        nxf_convert_32bpp
#else
#  error "Pixel depth not supported (CONFIG_NXWIDGETS_BPP)"
#endif

/* Size of a character */

#ifndef CONFIG_NXWIDGETS_SIZEOFCHAR
# if CONFIG_NXFONTS_CHARBITS <= 8
#    define CONFIG_NXWIDGETS_SIZEOFCHAR 1
# else
#    define CONFIG_NXWIDGETS_SIZEOFCHAR 2
# endif
#endif

#if CONFIG_NXWIDGETS_SIZEOFCHAR != 1 && CONFIG_NXWIDGETS_SIZEOFCHAR != 2
#  error "Unsupported character width (CONFIG_NXWIDGETS_SIZEOFCHAR)"
#endif

/* NXWidget Default Values **************************************************/
/**
 * Default font ID
 */

#ifndef CONFIG_NXWIDGETS_DEFAULT_FONTID
#  define CONFIG_NXWIDGETS_DEFAULT_FONTID NXFONT_DEFAULT
#endif

/**
 * Default dynamic array parameters
 */

#ifndef CONFIG_NXWIDGETS_TNXARRAY_INITIALSIZE
#  define CONFIG_NXWIDGETS_TNXARRAY_INITIALSIZE 16
#endif

#ifndef CONFIG_NXWIDGETS_TNXARRAY_SIZEINCREMENT
#  define CONFIG_NXWIDGETS_TNXARRAY_SIZEINCREMENT 8
#endif

/**
 * Normal background color
 */

#ifndef CONFIG_NXWIDGETS_DEFAULT_BACKGROUNDCOLOR
#  define CONFIG_NXWIDGETS_DEFAULT_BACKGROUNDCOLOR MKRGB(148,189,215)
#endif

/**
 * Default selected background color
 */

#ifndef CONFIG_NXWIDGETS_DEFAULT_SELECTEDBACKGROUNDCOLOR
#  define CONFIG_NXWIDGETS_DEFAULT_SELECTEDBACKGROUNDCOLOR MKRGB(206,227,241)
#endif

/**
 * Shiny side border color
 */

#ifndef CONFIG_NXWIDGETS_DEFAULT_SHINEEDGECOLOR
#  define CONFIG_NXWIDGETS_DEFAULT_SHINEEDGECOLOR MKRGB(248,248,248)
#endif

/**
 * Shadowed side border color
 */

#ifndef CONFIG_NXWIDGETS_DEFAULT_SHADOWEDGECOLOR
#  define CONFIG_NXWIDGETS_DEFAULT_SHADOWEDGECOLOR MKRGB(35,58,73)
#endif

/**
 * Highlight color
 */

#ifndef CONFIG_NXWIDGETS_DEFAULT_HIGHLIGHTCOLOR
#  define CONFIG_NXWIDGETS_DEFAULT_HIGHLIGHTCOLOR MKRGB(192,192,192)
#endif

/* Text colors */

#ifndef CONFIG_NXWIDGETS_DEFAULT_DISABLEDTEXTCOLOR
#  define CONFIG_NXWIDGETS_DEFAULT_DISABLEDTEXTCOLOR MKRGB(192,192,192)
#endif

#ifndef CONFIG_NXWIDGETS_DEFAULT_ENABLEDTEXTCOLOR
#  define CONFIG_NXWIDGETS_DEFAULT_ENABLEDTEXTCOLOR MKRGB(248,248,248)
#endif

#ifndef CONFIG_NXWIDGETS_DEFAULT_SELECTEDTEXTCOLOR
#  define CONFIG_NXWIDGETS_DEFAULT_SELECTEDTEXTCOLOR MKRGB(0,0,0)
#endif

/**
 * Default font color
 */

#ifndef CONFIG_NXWIDGETS_DEFAULT_FONTCOLOR
#  define CONFIG_NXWIDGETS_DEFAULT_FONTCOLOR MKRGB(255,255,255)
#endif

/**
 * Transparent color
 */

#ifndef CONFIG_NXWIDGETS_TRANSPARENT_COLOR
#  define CONFIG_NXWIDGETS_TRANSPARENT_COLOR MKRGB(0,0,0)
#endif

/* Keypad behavior **********************************************************/
/**
 * Time taken before a key starts repeating (in milliseconds).
 */

#ifndef CONFIG_NXWIDGETS_FIRST_REPEAT_TIME
#  define CONFIG_NXWIDGETS_FIRST_REPEAT_TIME 500
#endif

/**
 * Time taken before a repeating key repeats again (in milliseconds).
 */

#ifndef CONFIG_NXWIDGETS_CONTINUE_REPEAT_TIME
#  define CONFIG_NXWIDGETS_CONTINUE_REPEAT_TIME 200
#endif

/**
 * Left button release-press time for double click (in milliseconds).
 */

#ifndef CONFIG_NXWIDGETS_DOUBLECLICK_TIME
#  define CONFIG_NXWIDGETS_DOUBLECLICK_TIME 350
#endif

/**
 * Size of incoming character buffer, i.e., the maximum number of characters
 * that can be entered between NX polling cycles without losing data.
 */

#ifndef CONFIG_NXWIDGETS_KBDBUFFER_SIZE
#  define CONFIG_NXWIDGETS_KBDBUFFER_SIZE 8
#endif

/**
 * Size of incoming cursor control buffer, i.e., the maximum number of cursor
 * controls that can between entered by NX polling cycles without losing data.
 */

#ifndef CONFIG_NXWIDGETS_CURSORCONTROL_SIZE
#  define CONFIG_NXWIDGETS_CURSORCONTROL_SIZE 4
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

namespace NXWidgets
{
#if CONFIG_NXWIDGETS_BPP == 8
  typedef uint8_t nxwidget_pixel_t;
#elif CONFIG_NXWIDGETS_BPP == 16
  typedef uint16_t nxwidget_pixel_t;
#elif CONFIG_NXWIDGETS_BPP == 24
  typedef uint32_t nxwidget_pixel_t;
#elif CONFIG_NXWIDGETS_BPP == 32
  typedef uint32_t nxwidget_pixel_t;
#else
#  error "Pixel depth is unknown"
#endif

#if CONFIG_NXWIDGETS_SIZEOFCHAR == 2
  typedef uint16_t nxwidget_char_t;
#elif CONFIG_NXWIDGETS_SIZEOFCHAR == 1
  typedef uint8_t nxwidget_char_t;
#else
#  error "Character width is unknown"
#endif
}

/****************************************************************************
 * Implementation Classes
 ****************************************************************************/

#endif // __APPS_INCLUDE_GRAPHICS_NXWIDGETS_NXCONFIG_HXX
