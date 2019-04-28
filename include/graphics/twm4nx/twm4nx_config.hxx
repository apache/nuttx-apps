/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/include/twm4nx_config.hxx
// Twm4Nx configuration settings
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

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_TWM4NX_CONFIG_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_TWM4NX_CONFIG_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>

#include "graphics/nxglyphs.hxx"
#include "graphics/nxwidgets/nxconfig.hxx"
#include "graphics/nxwidgets/crlepalettebitmap.hxx"

/////////////////////////////////////////////////////////////////////////////
// Pre-Processor Definitions
/////////////////////////////////////////////////////////////////////////////

// General Configuration ////////////////////////////////////////////////////

/**
 * Required settings:
 *
 * CONFIG_HAVE_CXX        : C++ support is required
 * CONFIG_NX              : NX must enabled
 * CONFIG_NXTERM=y        : For NxTerm support
 */

#ifndef CONFIG_HAVE_CXX
#  error "C++ support is required (CONFIG_HAVE_CXX)"
#endif

/**
 * NX Multi-user support is required
 */

#ifndef CONFIG_NX
#  error "NX support is required (CONFIG_NX)"
#endif

/**
 * NxTerm support is (probably) required to support NxTWM terminals
 */

#if defined(CONFIG_TWM4NX_NXTERM) && !defined(CONFIG_NXTERM)
#  warning "NxTerm support may be needed (CONFIG_NXTERM)"
#endif

// Background ///////////////////////////////////////////////////////////////

/**
 * CONFIG_TWM4NX_DEFAULT_BACKGROUNDCOLOR - Normal background color.  Default:
 *    MKRGB(148,189,215)
 * CONFIG_TWM4NX_BACKGROUND_IMAGE - The name of the image to use in the
 *   background window.  Default:NXWidgets::g_nuttxBitmap160x160
 */

#ifndef CONFIG_TWM4NX_BACKGROUND_IMAGE
#  define CONFIG_TWM4NX_BACKGROUND_IMAGE NXWidgets::g_nuttxBitmap160x160
#endif

// Windows //////////////////////////////////////////////////////////////////

// Toolbar /////////////////////////////////////////////////////////////////

/**
 * Toolbar Icons.  The Title bar contains (from left to right):
 *
 * 1. Menu button
 * 2. Window title (text)
 * 3. Minimize (Iconify) button
 * 4. Resize button
 * 5. Delete button
 *
 * There is no focus indicator
 */

#ifndef CONFIG_TWM4NX_MENU_IMAGE
#  define CONFIG_TWM4NX_MENU_IMAGE      NXWidgets::g_menuBitmap
#endif

#ifndef CONFIG_TWM4NX_MINIMIZE_IMAGE
#  define CONFIG_TWM4NX_MINIMIZE_IMAGE  NXWidgets::g_minimizeBitmap
#endif

#ifndef CONFIG_TWM4NX_RESIZE_IMAGE
#  define CONFIG_TWM4NX_RESIZE_IMAGE    NXWidgets::g_resizeBitmap
#endif

#ifndef CONFIG_TWM4NX_TERMINATE_IMAGE
#  define CONFIG_TWM4NX_TERMINATE_IMAGE NXWidgets::g_stopBitmap
#endif

#ifndef CONFIG_TWM4NX_ICONMGR_IMAGE
#  define CONFIG_TWM4NX_ICONMGR_IMAGE   NXWidgets::g_nxiconBitmap
#endif

// Spacing.  Defaults values good at 75 and 100 DPI

#ifndef CONFIG_TWM4NX_TOOLBAR_HSPACING
#  define CONFIG_TWM4NX_TOOLBAR_HSPACING 8
#endif

#ifndef CONFIG_TWM4NX_ICONMGR_VSPACING
#  define CONFIG_TWM4NX_FRAME_VSPACING   2
#endif

#ifndef CONFIG_TWM4NX_BUTTON_INDENT
#  define CONFIG_TWM4NX_BUTTON_INDENT   1
#endif

#ifndef CONFIG_TWM4NX_ICONMGR_VSPACING
#  define CONFIG_TWM4NX_ICONMGR_VSPACING   2
#endif

#ifndef CONFIG_TWM4NX_ICONMGR_HSPACING
#  define CONFIG_TWM4NX_ICONMGR_HSPACING   2
#endif

// Menus ////////////////////////////////////////////////////////////////////

/**
 * CONFIG_TWM4NX_MENU_IMAGE.  Menu image (see toolbar icons)
 */

// Colors ///////////////////////////////////////////////////////////////////

/* Colors *******************************************************************/

/**
 * Color configuration
 *
 * CONFIG_TWM4NX_DEFAULT_BACKGROUNDCOLOR - Normal background color.  Default:
 *    MKRGB(148,189,215)
 * CONFIG_TWM4NX_DEFAULT_SELECTEDBACKGROUNDCOLOR - Select background color.
 *    Default:  MKRGB(206,227,241)
 * CONFIG_TWM4NX_DEFAULT_SHINEEDGECOLOR - Color of the bright edge of a border.
 *    Default: MKRGB(255,255,255)
 * CONFIG_TWM4NX_DEFAULT_SHADOWEDGECOLOR - Color of the shadowed edge of a border.
 *    Default: MKRGB(0,0,0)
 * CONFIG_TWM4NX_DEFAULT_FONTCOLOR - Default font color.  Default:
 *    MKRGB(0,0,0)
 * CONFIG_TWM4NX_TRANSPARENT_COLOR - The "transparent" color.  Default:
 *    MKRGB(0,0,0)
 */

/**
 * Normal background color
 */

#ifndef CONFIG_TWM4NX_DEFAULT_BACKGROUNDCOLOR
#  define CONFIG_TWM4NX_DEFAULT_BACKGROUNDCOLOR  MKRGB(148,189,215)
#endif

/**
 * Default selected background color
 */

#ifndef CONFIG_TWM4NX_DEFAULT_SELECTEDBACKGROUNDCOLOR
#  define CONFIG_TWM4NX_DEFAULT_SELECTEDBACKGROUNDCOLOR  MKRGB(206,227,241)
#endif

/**
 * Border colors
 */

#ifndef CONFIG_TWM4NX_DEFAULT_SHINEEDGECOLOR
#  define CONFIG_TWM4NX_DEFAULT_SHINEEDGECOLOR  MKRGB(248,248,248)
#endif

#ifndef CONFIG_TWM4NX_DEFAULT_SHADOWEDGECOLOR
#  define CONFIG_TWM4NX_DEFAULT_SHADOWEDGECOLOR  MKRGB(35,58,73)
#endif

/**
 * The default font color
 */

#ifndef CONFIG_TWM4NX_DEFAULT_FONTCOLOR
#  define CONFIG_TWM4NX_DEFAULT_FONTCOLOR  MKRGB(255,255,255)
#endif

/**
 * The transparent color
 */

#ifndef CONFIG_TWM4NX_TRANSPARENT_COLOR
#  define CONFIG_TWM4NX_TRANSPARENT_COLOR  MKRGB(0,0,0)
#endif

// Toolbar Configuration ////////////////////////////////////////////////////

/**
 * CONFIG_TWM4NX_TOOLBAR_HEIGHT.  The height of the tool bar in each
 *   application window. At present, all icons are 21 or 42 pixels in height
 *   (depending on the setting of CONFIG_TWM4NX_LARGE_ICONS) and, hence require
 *   a task bar of at least that size.
 */

#ifndef CONFIG_TWM4NX_TOOLBAR_HEIGHT
#    define CONFIG_TWM4NX_TOOLBAR_HEIGHT \
       (CONFIG_TWM4NX_TASKBAR_ICONHEIGHT + 2 * CONFIG_TWM4NX_TASKBAR_HSPACING)
#endif

/* CONFIG_TWM4NX_TOOLBAR_FONTID overrides the default Twm4Nx font selection */

#ifndef CONFIG_TWM4NX_TOOLBAR_FONTID
#  define CONFIG_TWM4NX_TOOLBAR_FONTID  CONFIG_TWM4NX_DEFAULT_FONTID
#endif

// Background Image //////////////////////////////////////////////////////////

/**
 * CONFIG_TWM4NX_BACKGROUND_IMAGE - The name of the image to use in the
 *   background window.  Default:NXWidgets::g_nuttxBitmap160x160
 */

#ifndef CONFIG_TWM4NX_BACKGROUND_IMAGE
#  define CONFIG_TWM4NX_BACKGROUND_IMAGE NXWidgets::g_nuttxBitmap160x160
#endif

// Cursor ////////////////////////////////////////////////////////////////////
// Cursor Images

#ifndef CONFIG_TWM4NX_CURSOR_IMAGE             // The normal cursor image
#  define CONFIG_TWM4NX_CURSOR_IMAGE   g_arrow1Cursor
#endif

#ifndef CONFIG_TWM4NX_GBCURSOR_IMAGE           // Grab cursor image
#  define CONFIG_TWM4NX_GBCURSOR_IMAGE g_grabCursor
#endif

#ifndef CONFIG_TWM4NX_WTCURSOR_IMAGE           // Wait cursor image
#  define CONFIG_TWM4NX_WTCURSOR_IMAGE g_waitCursor
#endif

// Fonts /////////////////////////////////////////////////////////////////////

// Font IDs

#ifndef CONFIG_TWM4NX_TITLE_FONTID
#  define CONFIG_TWM4NX_TITLE_FONTID NXFONT_DEFAULT
#endif

#ifndef CONFIG_TWM4NX_MENU_FONTID
#  define CONFIG_TWM4NX_MENU_FONTID NXFONT_DEFAULT
#endif

#ifndef CONFIG_TWM4NX_ICON_FONTID
#  define CONFIG_TWM4NX_ICON_FONTID NXFONT_DEFAULT
#endif

#ifndef CONFIG_TWM4NX_SIZE_FONTID
#  define CONFIG_TWM4NX_SIZE_FONTID NXFONT_DEFAULT
#endif

#ifndef CONFIG_TWM4NX_ICONMGR_SIZEFONTID
#  define CONFIG_TWM4NX_ICONMGR_SIZEFONTID NXFONT_DEFAULT
#endif

// Font Colors

#ifndef CONFIG_TWM4NX_TITLE_FONTCOLOR
#  define CONFIG_TWM4NX_TITLE_FONTCOLOR MKRGB(0,64,0)
#endif

#ifndef CONFIG_TWM4NX_MENU_FONTCOLOR
#  define CONFIG_TWM4NX_MENU_FONTCOLOR MKRGB(0,64,0)
#endif

#ifndef CONFIG_TWM4NX_ICON_FONTCOLOR
#  define CONFIG_TWM4NX_ICON_FONTCOLOR MKRGB(0,64,0)
#endif

#ifndef CONFIG_TWM4NX_SIZE_FONTCOLOR
#  define CONFIG_TWM4NX_SIZE_FONTCOLOR MKRGB(0,64,0)
#endif

#ifndef CONFIG_TWM4NX_ICONMGR_FONTCOLOR
#  define CONFIG_TWM4NX_ICONMGR_FONTCOLOR MKRGB(0,64,0)
#endif

#ifndef CONFIG_TWM4NX_DEFAULT_FONTCOLOR
#  define CONFIG_TWM4NX_DEFAULT_FONTCOLOR MKRGB(0,64,0)
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

#ifdef CONFIG_TWM4NX_NXTERM
#  ifndef CONFIG_TWM4NX_NXTERM_PRIO
#    define CONFIG_TWM4NX_NXTERM_PRIO  SCHED_PRIORITY_DEFAULT
#  endif

#  if CONFIG_NXSTART_SERVERPRIO <= CONFIG_TWM4NX_NXTERM_PRIO
#    warning "CONFIG_NXSTART_SERVERPRIO <= CONFIG_TWM4NX_NXTERM_PRIO"
#    warning" -- This can result in data overrun errors"
#  endif

#  ifndef CONFIG_TWM4NX_NXTERM_STACKSIZE
#    define CONFIG_TWM4NX_NXTERM_STACKSIZE  2048
#  endif

#  ifndef CONFIG_TWM4NX_NXTERM_WCOLOR
#    define CONFIG_TWM4NX_NXTERM_WCOLOR  CONFIG_TWM4NX_DEFAULT_BACKGROUNDCOLOR
#  endif

#  ifndef CONFIG_TWM4NX_NXTERM_FONTCOLOR
#    define CONFIG_TWM4NX_NXTERM_FONTCOLOR  CONFIG_TWM4NX_DEFAULT_FONTCOLOR
#  endif

#  ifndef CONFIG_TWM4NX_NXTERM_FONTID
#    define CONFIG_TWM4NX_NXTERM_FONTID  CONFIG_TWM4NX_DEFAULT_FONTID
#  endif

  /**
   * The NxTerm window glyph
   */

#  ifndef CONFIG_TWM4NX_NXTERM_ICON
#    define CONFIG_TWM4NX_NXTERM_ICON NXWidgets::g_cmdBitmap
#  endif
#endif

// Input Devices /////////////////////////////////////////////////////////////

/**
 * Configuration settings
 *
 * CONFIG_VNCSERVER - If selected, then keyboard and positional input will
 *   come from the VNC server.  In this case all input settings are ignored.
 *
 * Common input device settings
 *
 * CONFIG_TWM4NX_INPUT_SIGNO - The realtime signal used to wake up the
 *   keyboard/mouse listener thread.  Default: 6
 * CONFIG_TWM4NX_INPUT_LISTENERPRIO - Priority of the touchscreen listener
 *   thread.  Default: (SCHED_PRIORITY_DEFAULT + 20)
 * CONFIG_TWM4NX_INPUT_LISTENERSTACK - Input listener thread stack
 *   size.  Default 1024
 */

#ifndef CONFIG_TWM4NX_INPUT_SIGNO
#  define CONFIG_TWM4NX_INPUT_SIGNO 6
#endif

#ifndef CONFIG_TWM4NX_INPUT_LISTENERPRIO
#  define CONFIG_TWM4NX_INPUT_LISTENERPRIO (SCHED_PRIORITY_DEFAULT + 20)
#endif

#ifndef CONFIG_TWM4NX_INPUT_LISTENERSTACK
#  define CONFIG_TWM4NX_INPUT_LISTENERSTACK 1024
#endif

// Mouse Input Device ////////////////////////////////////////////////////////

/**
 * Mouse device settings
 *
 * CONFIG_TWM4NX_MOUSE_DEVPATH - The full path to the mouse device.
 *   Default: "/dev/console"
 * CONFIG_TWM4NX_MOUSE_USBHOST - Indicates the the mouse is attached via
 *   USB
 * CONFIG_TWM4NX_MOUSE_BUFSIZE - The size of the mouse read data buffer.
 *   Default: sizeof(struct mouse_report_s)
 */

#ifndef CONFIG_TWM4NX_MOUSE_DEVPATH
#  define CONFIG_TWM4NX_MOUSE_DEVPATH "/dev/mouse0"
#endif

#ifndef CONFIG_TWM4NX_MOUSE_BUFSIZE
#  define CONFIG_TWM4NX_MOUSE_BUFSIZE sizeof(struct mouse_report_s)
#endif

// Keyboard device ///////////////////////////////////////////////////////////

/**
 * Keyboard device settings
 *
 * CONFIG_TWM4NX_KEYBOARD_DEVPATH - The full path to the keyboard device.
 *   Default: "/dev/console"
 * CONFIG_TWM4NX_KEYBOARD_USBHOST - Indicates the the keyboard is attached via
 *   USB
 * CONFIG_TWM4NX_KEYBOARD_BUFSIZE - The size of the keyboard read data buffer.
 *   Default: 16
 */

#ifndef CONFIG_TWM4NX_KEYBOARD_DEVPATH
#  define CONFIG_TWM4NX_KEYBOARD_DEVPATH "/dev/kbd0"
#endif

#ifndef CONFIG_TWM4NX_KEYBOARD_BUFSIZE
#  define CONFIG_TWM4NX_KEYBOARD_BUFSIZE 6
#endif

#endif // __APPS_INCLUDE_GRAPHICS_TWM4NX_TWM4NX_CONFIG_HXX
