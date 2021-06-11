/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/twm4nx/twm4nx_config.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_TWM4NX_CONFIG_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_TWM4NX_CONFIG_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>
#include <debug.h>

#include "graphics/nxglyphs.hxx"
#include "graphics/nxwidgets/nxconfig.hxx"
#include "graphics/nxwidgets/crlepalettebitmap.hxx"

/////////////////////////////////////////////////////////////////////////////
// Pre-Processor Definitions
/////////////////////////////////////////////////////////////////////////////

// Debug ////////////////////////////////////////////////////////////////////

#ifdef CONFIG_TWM4NX_DEBUG
#  define twminfo(format, ...)   _info(format, ##__VA_ARGS__)
#  define twmwarn(format, ...)   _warn(format, ##__VA_ARGS__)
#  define twmerr(format, ...)    _err(format, ##__VA_ARGS__)
#else
#  define twminfo(format, ...)   ginfo(format, ##__VA_ARGS__)
#  define twmwarn(format, ...)   gwarn(format, ##__VA_ARGS__)
#  define twmerr(format, ...)    gerr(format, ##__VA_ARGS__)
#endif

// General Configuration ////////////////////////////////////////////////////

/**
 * Required settings:
 *
 * CONFIG_HAVE_CXX        : C++ support is required
 * CONFIG_NX              : NX must enabled
 */

#ifndef CONFIG_HAVE_CXX
#  error "C++ support is required (CONFIG_HAVE_CXX)"
#endif

#ifndef CONFIG_NX
#  error "NX support is required (CONFIG_NX)"
#endif

// Background ///////////////////////////////////////////////////////////////

/**
 * CONFIG_TWM4NX_DEFAULT_BACKGROUNDCOLOR - Normal background color.  Default:
 *    MKRGB(148,189,215)
 * CONFIG_TWM4NX_BACKGROUND_HASIMAGE - True if the background has an image
 * CONFIG_TWM4NX_BACKGROUND_IMAGE - The name of the image to use in the
 *   background window.  Default:NXWidgets::g_nuttxBitmap160x160
 */

#ifdef CONFIG_TWM4NX_CLASSIC
#  define CONFIG_TWM4NX_BACKGROUND_HASIMAGE 1
#  ifndef CONFIG_TWM4NX_BACKGROUND_IMAGE
#    define CONFIG_TWM4NX_BACKGROUND_IMAGE NXWidgets::g_nuttxBitmap160x160
#  endif
#else
#  undef CONFIG_TWM4NX_BACKGROUND_HASIMAGE
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
#  ifdef CONFIG_TWM4NX_CLASSIC
#    define CONFIG_TWM4NX_MENU_IMAGE      NXWidgets::g_menuBitmap
#  else
#    define CONFIG_TWM4NX_MENU_IMAGE      NXWidgets::g_menu2Bitmap
#  endif
#endif

#ifndef CONFIG_TWM4NX_MINIMIZE_IMAGE
#  ifdef CONFIG_TWM4NX_CLASSIC
#    define CONFIG_TWM4NX_MINIMIZE_IMAGE  NXWidgets::g_minimizeBitmap
#  else
#    define CONFIG_TWM4NX_MINIMIZE_IMAGE  NXWidgets::g_minimize2Bitmap
#  endif
#endif

#ifndef CONFIG_TWM4NX_RESIZE_IMAGE
#  ifdef CONFIG_TWM4NX_CLASSIC
#    define CONFIG_TWM4NX_RESIZE_IMAGE    NXWidgets::g_resizeBitmap
#  else
#    define CONFIG_TWM4NX_RESIZE_IMAGE    NXWidgets::g_resize2Bitmap
#  endif
#endif

#ifndef CONFIG_TWM4NX_TERMINATE_IMAGE
#  ifdef CONFIG_TWM4NX_CLASSIC
#    define CONFIG_TWM4NX_TERMINATE_IMAGE NXWidgets::g_stopBitmap
#  else
#    define CONFIG_TWM4NX_TERMINATE_IMAGE NXWidgets::g_stop2Bitmap
#  endif
#endif

#ifndef CONFIG_TWM4NX_ICONMGR_IMAGE
#  define CONFIG_TWM4NX_ICONMGR_IMAGE   NXWidgets::g_nxiconBitmap
#endif

// Spacing.  Defaults values good at 75 and 100 DPI

#ifndef CONFIG_TWM4NX_TOOLBAR_HSPACING
#  define CONFIG_TWM4NX_TOOLBAR_HSPACING   2
#endif

#ifndef CONFIG_TWM4NX_TOOLBAR_VSPACING
#  define CONFIG_TWM4NX_TOOLBAR_VSPACING   2
#endif

#ifndef CONFIG_TWM4NX_BUTTON_INDENT
#  define CONFIG_TWM4NX_BUTTON_INDENT      1
#endif

#ifndef CONFIG_TWM4NX_MENU_HSPACING
#  define CONFIG_TWM4NX_MENU_HSPACING      2
#endif

#ifndef CONFIG_TWM4NX_MENU_VSPACING
#  define CONFIG_TWM4NX_MENU_VSPACING      0
#endif

#ifndef CONFIG_TWM4NX_ICON_VSPACING
#  define CONFIG_TWM4NX_ICON_VSPACING      2
#endif

#ifndef CONFIG_TWM4NX_ICON_HSPACING
#  define CONFIG_TWM4NX_ICON_HSPACING      2
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

// Themes ///////////////////////////////////////////////////////////////////

/**
 * CONFIG_TWM4NX_CLASSIC.  Strong bordered windows with dark primary colors.
 *   Reminiscent of Windows 98.
 * CONFIG_TWM4NX_CONTEMPORARY.  Border-less windows in pastel shades for a
 *   more contemporary look
 *
 * Selecting a theme selects default colors, button images, and icons.  For
 * a given theme to work correctly, it may depend on other configuration
 * settings outside of Twm4Nx.
 */

#if defined(CONFIG_TWM4NX_CONTEMPORARY) && CONFIG_NXTK_BORDERWIDTH != 0
#  warning Contemporary theme needs border-less windows (CONFIG_NXTK_BORDERWIDTH == 0)
#endif

// Colors ///////////////////////////////////////////////////////////////////

/**
 * Color configuration
 *
 * CONFIG_TWM4NX_DEFAULT_BACKGROUNDCOLOR - Normal background color.  Defaults:
 *    Classic Theme:      MKRGB(148,189,215)
 *    Contemporary Theme: MKRGB(101,157,189)
 * CONFIG_TWM4NX_DEFAULT_SELECTEDBACKGROUNDCOLOR - Select background color.
 *    Defaults:
 *    Classic Theme:      MKRGB(206,227,241)
 *    Contemporary Theme: MKRGB(143,191,219)
 * CONFIG_TWM4NX_DEFAULT_TOOLBARCOLOR - Normal toolbar background color.  Defaults:
 *    Classic Theme:      MKRGB(148,189,215)
 *    Contemporary Theme: MKRGB(193,167,79)
 * CONFIG_TWM4NX_DEFAULT_SELECTTOOLBARCOLOR - Select toolbar color.
 *    Defaults:
 *    Classic Theme:      MKRGB(206,227,241)
 *    Contemporary Theme: MKRGB(251,238,193)
 * CONFIG_TWM4NX_DEFAULT_SHINEEDGECOLOR - Color of the bright edge of a border.
 *    Defaults:
 *    Classic Theme:      MKRGB(255,255,255)
 *    Contemporary Theme: MKRGB(251,240,199)
 * CONFIG_TWM4NX_DEFAULT_SHADOWEDGECOLOR - Color of the shadowed edge of a border.
 *    Defaults:
 *    Classic Theme:      MKRGB(0,0,0)
 *    Contemporary Theme: MKRGB(201,165,116)
 * CONFIG_TWM4NX_DEFAULT_FONTCOLOR - Default font color.  Defaults:
 *    Classic Theme:      MKRGB(255,255,255)
 *    Contemporary Theme: MKRGB(255,255,255)
 * CONFIG_TWM4NX_TRANSPARENT_COLOR - The "transparent" color.  Defaults:
 *    Classic Theme:      MKRGB(0,0,0)
 *    Contemporary Theme: MKRGB(0,0,0)
 */

/**
 * Normal background color
 */

#ifndef CONFIG_TWM4NX_DEFAULT_BACKGROUNDCOLOR
#  ifdef CONFIG_TWM4NX_CLASSIC
#    define CONFIG_TWM4NX_DEFAULT_BACKGROUNDCOLOR  MKRGB(148,189,215)
#  else
#    define CONFIG_TWM4NX_DEFAULT_BACKGROUNDCOLOR  MKRGB(101,148,188)
#  endif
#endif

/**
 * Default selected background color
 */

#ifndef CONFIG_TWM4NX_DEFAULT_SELECTEDBACKGROUNDCOLOR
#  ifdef CONFIG_TWM4NX_CLASSIC
#    define CONFIG_TWM4NX_DEFAULT_SELECTEDBACKGROUNDCOLOR  MKRGB(206,227,241)
#  else
#    define CONFIG_TWM4NX_DEFAULT_SELECTEDBACKGROUNDCOLOR  MKRGB(158,193,211)
#  endif
#endif

/**
 * Normal toolbar background color
 */

#ifndef CONFIG_TWM4NX_DEFAULT_TOOLBARCOLOR
#  ifdef CONFIG_TWM4NX_CLASSIC
#    define CONFIG_TWM4NX_DEFAULT_TOOLBARCOLOR  MKRGB(148,189,215)
#  else
#    define CONFIG_TWM4NX_DEFAULT_TOOLBARCOLOR  MKRGB(193,167,79)
#  endif
#endif

/**
 * Default selected toolbar background color
 */

#ifndef CONFIG_TWM4NX_DEFAULT_SELECTEDTOOLBARCOLOR
#  ifdef CONFIG_TWM4NX_CLASSIC
#    define CONFIG_TWM4NX_DEFAULT_SELECTEDTOOLBARCOLOR  MKRGB(206,227,241)
#  else
#    define CONFIG_TWM4NX_DEFAULT_SELECTEDTOOLBARCOLOR  MKRGB(251,238,193)
#  endif
#endif

/**
 * Border colors
 */

#ifndef CONFIG_TWM4NX_DEFAULT_SHINEEDGECOLOR
#  ifdef CONFIG_TWM4NX_CLASSIC
#    define CONFIG_TWM4NX_DEFAULT_SHINEEDGECOLOR  MKRGB(248,248,248)
#  else
#    define CONFIG_TWM4NX_DEFAULT_SHINEEDGECOLOR  MKRGB(251,240,199)
#  endif
#endif

#ifndef CONFIG_TWM4NX_DEFAULT_SHADOWEDGECOLOR
#  ifdef CONFIG_TWM4NX_CLASSIC
#    define CONFIG_TWM4NX_DEFAULT_SHADOWEDGECOLOR  MKRGB(35,58,73)
#  else
#    define CONFIG_TWM4NX_DEFAULT_SHADOWEDGECOLOR  MKRGB(201,165,116)
#  endif
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

// Icon Manager //////////////////////////////////////////////////////////////

// CONFIG_TWM4NX_ICONMGR_NCOLUMNS - The number of horizontal entries in the
// Icon Manager button array.

// See also:
//  CONFIG_TWM4NX_ICONMGR_FONTID
//  CONFIG_TWM4NX_ICONMGR_FONTCOLOR
//  CONFIG_TWM4NX_ICONMGR_IMAGE
//  CONFIG_TWM4NX_ICONMGR_VSPACING
//  CONFIG_TWM4NX_ICONMGR_HSPACING

#ifndef CONFIG_TWM4NX_ICONMGR_NCOLUMNS
#  define CONFIG_TWM4NX_ICONMGR_NCOLUMNS 4
#endif

// Cursor ////////////////////////////////////////////////////////////////////
// Cursor Images

#ifndef CONFIG_TWM4NX_CURSOR_IMAGE             // The normal cursor image
#  define CONFIG_TWM4NX_CURSOR_IMAGE   g_arrow1Cursor
#endif

#ifndef CONFIG_TWM4NX_GBCURSOR_IMAGE           // Grab cursor image
#  define CONFIG_TWM4NX_GBCURSOR_IMAGE g_grabCursor
#endif

#ifndef CONFIG_TWM4NX_RZCURSOR_IMAGE           // Resize cursor image
#  define CONFIG_TWM4NX_RZCURSOR_IMAGE g_resizeCursor
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

#ifndef CONFIG_TWM4NX_ICONMGR_FONTID
#  define CONFIG_TWM4NX_ICONMGR_FONTID NXFONT_DEFAULT
#endif

// Font Colors

#ifndef CONFIG_TWM4NX_TITLE_FONTCOLOR
#  define CONFIG_TWM4NX_TITLE_FONTCOLOR CONFIG_TWM4NX_DEFAULT_FONTCOLOR
#endif

#ifndef CONFIG_TWM4NX_MENU_FONTCOLOR
#  define CONFIG_TWM4NX_MENU_FONTCOLOR CONFIG_TWM4NX_DEFAULT_FONTCOLOR
#endif

#ifndef CONFIG_TWM4NX_ICON_FONTCOLOR
#  define CONFIG_TWM4NX_ICON_FONTCOLOR CONFIG_TWM4NX_DEFAULT_FONTCOLOR
#endif

#ifndef CONFIG_TWM4NX_SIZE_FONTCOLOR
#  define CONFIG_TWM4NX_SIZE_FONTCOLOR CONFIG_TWM4NX_DEFAULT_FONTCOLOR
#endif

#ifndef CONFIG_TWM4NX_ICONMGR_FONTCOLOR
#  define CONFIG_TWM4NX_ICONMGR_FONTCOLOR CONFIG_TWM4NX_DEFAULT_FONTCOLOR
#endif

// Input Devices /////////////////////////////////////////////////////////////

/**
 * Configuration settings
 *
 * CONFIG_TWM4NX_VNCSERVER - If selected, then keyboard and positional input
 *   will come from the VNC server.  In this case all other input settings
 *   are ignored.
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
 * CONFIG_TWM4NX_NOMOUSE - Can be used to disable mouse input.
 * CONFIG_TWM4NX_MOUSE - Input is from a mouse.
 * CONFIG_TWM4NX_TOUSCREEN - Input is from a touchscreen.
 * CONFIG_TWM4NX_MOUSE_DEVPATH - The full path to the mouse device.
 *   Default: "/dev/mouse0" if a mouse is being used; /dev/input0 is a
 *   touchscreen is being used.
 * CONFIG_TWM4NX_MOUSE_USBHOST - Indicates the the mouse is attached via
 *   USB
 * CONFIG_TWM4NX_MOUSE_BUFSIZE - The size of the mouse read data buffer.
 *   Default: sizeof(struct mouse_report_s) or SIZEOF_TOUCH_SAMPLE_S(1)
 */

#ifndef CONFIG_TWM4NX_MOUSE_DEVPATH
#  ifdef CONFIG_TWM4NX_MOUSE
#    define CONFIG_TWM4NX_MOUSE_DEVPATH "/dev/mouse0"
#  else
#    define CONFIG_TWM4NX_MOUSE_DEVPATH "/dev/input0"
#  endif
#endif

#ifndef CONFIG_TWM4NX_MOUSE_BUFSIZE
#  ifdef CONFIG_TWM4NX_MOUSE
#    define CONFIG_TWM4NX_MOUSE_BUFSIZE sizeof(struct mouse_report_s)
#  else
#    define CONFIG_TWM4NX_MOUSE_BUFSIZE SIZEOF_TOUCH_SAMPLE_S(1)
#  endif
#endif

// Keyboard device ///////////////////////////////////////////////////////////

/**
 * Keyboard device settings
 *
 * CONFIG_TWM4NX_NOKEYBOARD - Can be used to disable keyboard input.
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
