/****************************************************************************
 * apps/include/graphics/nxwm/nxwmconfig.hxx
 *
 *   Copyright (C) 2012, 2014 Gregory Nutt. All rights reserved.
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

#ifndef __APPS_INCLUDE_GRAPHICS_NXWM_NXWMCONFIG_HXX
#define __APPS_INCLUDE_GRAPHICS_NXWM_NXWMCONFIG_HXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <nuttx/input/touchscreen.h>

#include "graphics/nxwidgets/nxconfig.hxx"
#include "graphics/nxwidgets/crlepalettebitmap.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/
/* General Configuration ****************************************************/
/**
 * Required settings:
 *
 * CONFIG_HAVE_CXX        : C++ support is required
 * CONFIG_NX              : NX must enabled
 * CONFIG_NXTERM=y     : For NxTerm support
 * CONFIG_SCHED_ONEXIT    : Support for on_exit()
 *
 * General settings:
 *
 * CONFIG_NXWM_DEFAULT_FONTID - the NxWM default font ID. Default:
 *   NXFONT_DEFAULT
 * CONFIG_NXWM_TOUCHSCREEN - Define to build in touchscreen support.
 * CONFIG_NXWM_KEYBOARD - Define to build in touchscreen support.
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
 * NxTerm support is (probably) required if CONFIG_NXWM_NXTERM is
 * selected
 */

#if defined(CONFIG_NXWM_NXTERM) && !defined(CONFIG_NXTERM)
#  warning "NxTerm support may be needed (CONFIG_NXTERM)"
#endif

/**
 * on_exit() support is (probably) required.  on_exit() is the normal
 * mechanism used by NxWM applications to clean-up on a application task
 * exit.
 */

#ifndef CONFIG_SCHED_ONEXIT
#  warning "on_exit() support may be needed (CONFIG_SCHED_ONEXIT)"
#endif

/**
 * Default font ID
 */

#ifndef CONFIG_NXWM_DEFAULT_FONTID
#  define CONFIG_NXWM_DEFAULT_FONTID NXFONT_DEFAULT
#endif

/* Colors *******************************************************************/
/**
 * Color configuration
 *
 * CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR - Normal background color.  Default:
 *    MKRGB(148,189,215)
 * CONFIG_NXWM_DEFAULT_SELECTEDBACKGROUNDCOLOR - Select background color.
 *    Default:  MKRGB(206,227,241)
 * CONFIG_NXWM_DEFAULT_SHINEEDGECOLOR - Color of the bright edge of a border.
 *    Default: MKRGB(255,255,255)
 * CONFIG_NXWM_DEFAULT_SHADOWEDGECOLOR - Color of the shadowed edge of a border.
 *    Default: MKRGB(0,0,0)
 * CONFIG_NXWM_DEFAULT_FONTCOLOR - Default font color.  Default:
 *    MKRGB(0,0,0)
 * CONFIG_NXWM_TRANSPARENT_COLOR - The "transparent" color.  Default:
 *    MKRGB(0,0,0)
 */

/**
 * Normal background color
 */

#ifndef CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR
#  define CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR  MKRGB(148,189,215)
#endif

/**
 * Default selected background color
 */

#ifndef CONFIG_NXWM_DEFAULT_SELECTEDBACKGROUNDCOLOR
#  define CONFIG_NXWM_DEFAULT_SELECTEDBACKGROUNDCOLOR  MKRGB(206,227,241)
#endif

/**
 * Border colors
 */

#ifndef CONFIG_NXWM_DEFAULT_SHINEEDGECOLOR
#  define CONFIG_NXWM_DEFAULT_SHINEEDGECOLOR  MKRGB(248,248,248)
#endif

#ifndef CONFIG_NXWM_DEFAULT_SHADOWEDGECOLOR
#  define CONFIG_NXWM_DEFAULT_SHADOWEDGECOLOR  MKRGB(35,58,73)
#endif

/**
 * The default font color
 */

#ifndef CONFIG_NXWM_DEFAULT_FONTCOLOR
#  define CONFIG_NXWM_DEFAULT_FONTCOLOR  MKRGB(255,255,255)
#endif

/**
 * The transparent color
 */

#ifndef CONFIG_NXWM_TRANSPARENT_COLOR
#  define CONFIG_NXWM_TRANSPARENT_COLOR  MKRGB(0,0,0)
#endif

/* Task Bar Configuration  **************************************************/
/**
 * Horizontal and vertical spacing of icons in the task bar.
 *
 * CONFIG_NXWM_TASKBAR_VSPACING - Vertical spacing.  Default: 2 pixels
 * CONFIG_NXWM_TASKBAR_HSPACING - Horizontal spacing.  Default: 2 rows
 *
 * Task bar location.  Default is CONFIG_NXWM_TASKBAR_TOP.
 *
 * CONFIG_NXWM_TASKBAR_TOP - Task bar is at the top of the display
 * CONFIG_NXWM_TASKBAR_BOTTOM - Task bar is at the bottom of the display
 * CONFIG_NXWM_TASKBAR_LEFT - Task bar is on the left side of the display
 * CONFIG_NXWM_TASKBAR_RIGHT - Task bar is on the right side of the display
 *
 * CONFIG_NXWM_TASKBAR_WIDTH - Task bar thickness (either vertical or
 *   horizontal).  Default: 25 + 2*spacing unless large icons are selected.
 *   Then the default is 50 + 2*spacing.
 */

/**
 * Horizontal and vertical spacing of icons in the task bar.
 */

#ifndef CONFIG_NXWM_TASKBAR_VSPACING
#  define CONFIG_NXWM_TASKBAR_VSPACING (2)
#endif

#ifndef CONFIG_NXWM_TASKBAR_HSPACING
#  define CONFIG_NXWM_TASKBAR_HSPACING (2)
#endif

/**
 * Check task bar location
 */

#if defined(CONFIG_NXWM_TASKBAR_TOP)
#  if defined(CONFIG_NXWM_TASKBAR_BOTTOM) || defined (CONFIG_NXWM_TASKBAR_LEFT) || defined (CONFIG_NXWM_TASKBAR_RIGHT)
#    warning "Multiple task bar positions specified"
#  endif
#elif defined(CONFIG_NXWM_TASKBAR_BOTTOM)
#  if defined (CONFIG_NXWM_TASKBAR_LEFT) || defined (CONFIG_NXWM_TASKBAR_RIGHT)
#    warning "Multiple task bar positions specified"
#  endif
#elif defined(CONFIG_NXWM_TASKBAR_LEFT)
#  if defined (CONFIG_NXWM_TASKBAR_RIGHT)
#    warning "Multiple task bar positions specified"
#  endif
#elif !defined(CONFIG_NXWM_TASKBAR_RIGHT)
#  warning "No task bar position specified"
#  define CONFIG_NXWM_TASKBAR_TOP 1
#endif

// Taskbar ICON scaling

#if defined(CONFIG_NXWM_TASKBAR_ICONSCALE)
#  ifndef CONFIG_NXWM_TASKBAR_ICONWIDTH
#    error Scaling requires CONFIG_NXWM_TASKBAR_ICONWIDTH
#    define CONFIG_NXWM_TASKBAR_ICONWIDTH  50
#  endif
#  ifndef CONFIG_NXWM_TASKBAR_ICONHEIGHT
#    error Scaling requires CONFIG_NXWM_TASKBAR_ICONHEIGHT
#    define CONFIG_NXWM_TASKBAR_ICONHEIGHT 42
#  endif
#elif defined(CONFIG_NXWM_LARGE_ICONS)
#  undef CONFIG_NXWM_TASKBAR_ICONWIDTH
#  define CONFIG_NXWM_TASKBAR_ICONWIDTH  50
#  undef CONFIG_NXWM_TASKBAR_ICONHEIGHT
#  define CONFIG_NXWM_TASKBAR_ICONHEIGHT 42
#else
#  undef CONFIG_NXWM_TASKBAR_ICONWIDTH
#  define CONFIG_NXWM_TASKBAR_ICONWIDTH  25
#  undef CONFIG_NXWM_TASKBAR_ICONHEIGHT
#  define CONFIG_NXWM_TASKBAR_ICONHEIGHT 21
#endif

/**
 * At present, all icons are 25 pixels in "width" and, hence require a
 * task bar of at least that size.
 */

#ifndef CONFIG_NXWM_TASKBAR_WIDTH
#  if defined(CONFIG_NXWM_TASKBAR_TOP) || defined(CONFIG_NXWM_TASKBAR_BOTTOM)
#    define CONFIG_NXWM_TASKBAR_WIDTH \
       (CONFIG_NXWM_TASKBAR_ICONWIDTH+2*CONFIG_NXWM_TASKBAR_HSPACING)
#  else
#    define CONFIG_NXWM_TASKBAR_WIDTH \
       (CONFIG_NXWM_TASKBAR_ICONWIDTH+2*CONFIG_NXWM_TASKBAR_VSPACING)
#  endif
#endif

/* Tool Bar Configuration ***************************************************/
/**
 * CONFIG_NXWM_TOOLBAR_HEIGHT.  The height of the tool bar in each
 *   application window. At present, all icons are 21 or 42 pixels in height
 *   (depending on the setting of CONFIG_NXWM_LARGE_ICONS) and, hence require
 *   a task bar of at least that size.
 */

#ifndef CONFIG_NXWM_TOOLBAR_HEIGHT
#    define CONFIG_NXWM_TOOLBAR_HEIGHT \
       (CONFIG_NXWM_TASKBAR_ICONHEIGHT + 2*CONFIG_NXWM_TASKBAR_HSPACING)
#endif

/* CONFIG_NXWM_TOOLBAR_FONTID overrides the default NxWM font selection */

#ifndef CONFIG_NXWM_TOOLBAR_FONTID
#  define CONFIG_NXWM_TOOLBAR_FONTID  CONFIG_NXWM_DEFAULT_FONTID
#endif

/* Background Image **********************************************************/
/**
 * CONFIG_NXWM_BACKGROUND_IMAGE - The name of the image to use in the
 *   background window.  Default:NXWidgets::g_nuttxBitmap160x160
 */

#ifndef CONFIG_NXWM_BACKGROUND_IMAGE
#  define CONFIG_NXWM_BACKGROUND_IMAGE NXWidgets::g_nuttxBitmap160x160
#endif

/* Start Window Configuration ***********************************************/
/**
 * Horizontal and vertical spacing of icons in the task bar.
 *
 * CONFIG_NXWM_STARTWINDOW_VSPACING - Vertical spacing.  Default: 4 pixels
 * CONFIG_NXWM_STARTWINDOW_HSPACING - Horizontal spacing.  Default: 4 rows
 * CONFIG_NXWM_STARTWINDOW_ICON - The glyph to use as the start window icon
 * CONFIG_NXWM_STARTWINDOW_MQNAME - The well known name of the message queue
 *   Used to communicated from CWindowMessenger to the start window thread.
 *   Default: "nxwm"
 * CONFIG_NXWM_STARTWINDOW_MXMSGS - The maximum number of messages to queue
 *   before blocking.  Default 32
 * CONFIG_NXWM_STARTWINDOW_MXMPRIO - The message priority. Default: 42.
 * CONFIG_NXWM_STARTWINDOW_PRIO - Priority of the StartWindoW task.  Default:
 *   SCHED_PRIORITY_DEFAULT.  NOTE:  This priority should be less than
 *   CONFIG_NXSTART_SERVERPRIO or else there may be data overrun errors.
 *   Such errors would most likely appear as duplicated rows of data on the
 *   display.
 * CONFIG_NXWM_STARTWINDOW_STACKSIZE - The stack size to use when starting the
 *   StartWindow task.  Default: 2048 bytes.
 */

#ifndef CONFIG_NXWM_STARTWINDOW_VSPACING
#  define CONFIG_NXWM_STARTWINDOW_VSPACING (4)
#endif

#ifndef CONFIG_NXWM_STARTWINDOW_HSPACING
#  define CONFIG_NXWM_STARTWINDOW_HSPACING (4)
#endif

/**
 * The start window glyph
 */

#ifndef CONFIG_NXWM_STARTWINDOW_ICON
#  define CONFIG_NXWM_STARTWINDOW_ICON NXWidgets::g_playBitmap
#endif

/**
 * Start window task parameters
 */

#ifndef CONFIG_NXWM_STARTWINDOW_MQNAME
#  define CONFIG_NXWM_STARTWINDOW_MQNAME  "nxwm"
#endif

#ifndef CONFIG_NXWM_STARTWINDOW_MXMSGS
#  ifdef CONFIG_NX_MXCLIENTMSGS
#    define CONFIG_NXWM_STARTWINDOW_MXMSGS CONFIG_NX_MXCLIENTMSGS
#  else
#    define CONFIG_NXWM_STARTWINDOW_MXMSGS 32
#  endif
#endif

#ifndef CONFIG_NXWM_STARTWINDOW_MXMPRIO
#  define CONFIG_NXWM_STARTWINDOW_MXMPRIO 42
#endif

#ifndef CONFIG_NXWM_STARTWINDOW_PRIO
#  define CONFIG_NXWM_STARTWINDOW_PRIO  SCHED_PRIORITY_DEFAULT
#endif

#if CONFIG_NXSTART_SERVERPRIO <= CONFIG_NXWM_STARTWINDOW_PRIO
#  warning "CONFIG_NXSTART_SERVERPRIO <= CONFIG_NXWM_STARTWINDOW_PRIO"
#  warning" -- This can result in data overrun errors"
#endif

#ifndef CONFIG_NXWM_STARTWINDOW_STACKSIZE
#  define CONFIG_NXWM_STARTWINDOW_STACKSIZE  2048
#endif

/* NxTerm Window *********************************************************/
/**
 * NxTerm Window Configuration
 *
 * CONFIG_NXWM_NXTERM_PRIO - Priority of the NxTerm task.  Default:
 *   SCHED_PRIORITY_DEFAULT.  NOTE:  This priority should be less than
 *   CONFIG_NXSTART_SERVERPRIO or else there may be data overrun errors.
 *   Such errors would most likely appear as duplicated rows of data on the
 *   display.
 * CONFIG_NXWM_NXTERM_STACKSIZE - The stack size to use when starting the
 *   NxTerm task.  Default: 2048 bytes.
 * CONFIG_NXWM_NXTERM_WCOLOR - The color of the NxTerm window background.
 *   Default:  MKRGB(192,192,192)
 * CONFIG_NXWM_NXTERM_FONTCOLOR - The color of the fonts to use in the
 *   NxTerm window.  Default: MKRGB(0,0,0)
 * CONFIG_NXWM_NXTERM_FONTID - The ID of the font to use in the NxTerm
 *   window.  Default: CONFIG_NXWM_DEFAULT_FONTID
 * CONFIG_NXWM_NXTERM_ICON - The glyph to use as the NxTerm icon
 */

#ifdef CONFIG_NXWM_NXTERM
#  ifndef CONFIG_NXWM_NXTERM_PRIO
#    define CONFIG_NXWM_NXTERM_PRIO  SCHED_PRIORITY_DEFAULT
#  endif

#  if CONFIG_NXSTART_SERVERPRIO <= CONFIG_NXWM_NXTERM_PRIO
#    warning "CONFIG_NXSTART_SERVERPRIO <= CONFIG_NXWM_NXTERM_PRIO"
#    warning" -- This can result in data overrun errors"
#  endif

#  ifndef CONFIG_NXWM_NXTERM_STACKSIZE
#    define CONFIG_NXWM_NXTERM_STACKSIZE  2048
#  endif

#  ifndef CONFIG_NXWM_NXTERM_WCOLOR
#    define CONFIG_NXWM_NXTERM_WCOLOR  CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR
#  endif

#  ifndef CONFIG_NXWM_NXTERM_FONTCOLOR
#    define CONFIG_NXWM_NXTERM_FONTCOLOR  CONFIG_NXWM_DEFAULT_FONTCOLOR
#  endif

#  ifndef CONFIG_NXWM_NXTERM_FONTID
#    define CONFIG_NXWM_NXTERM_FONTID  CONFIG_NXWM_DEFAULT_FONTID
#  endif

  /**
   * The NxTerm window glyph
   */

#  ifndef CONFIG_NXWM_NXTERM_ICON
#    define CONFIG_NXWM_NXTERM_ICON NXWidgets::g_cmdBitmap
#  endif
#endif

/* Touchscreen device *******************************************************/
/**
 * Touchscreen device settings
 *
 * CONFIG_NXWM_TOUCHSCREEN_DEVNO - Touchscreen device minor number, i.e., the
 *   N in /dev/inputN.  Default: 0
 * CONFIG_NXWM_TOUCHSCREEN_DEVPATH - The full path to the touchscreen device.
 *   Default: "/dev/input0"
 * CONFIG_NXWM_TOUCHSCREEN_SIGNO - The realtime signal used to wake up the
 *   touchscreen listener thread.  Default: 5
 * CONFIG_NXWM_TOUCHSCREEN_LISTENERPRIO - Priority of the touchscreen listener
 *   thread.  Default: (SCHED_PRIORITY_DEFAULT + 20)
 * CONFIG_NXWM_TOUCHSCREEN_LISTENERSTACK - Touchscreen listener thread stack
 *   size.  Default 1024
 */

#ifndef CONFIG_NXWM_TOUCHSCREEN_DEVNO
#  define CONFIG_NXWM_TOUCHSCREEN_DEVNO 0
#endif

#ifndef CONFIG_NXWM_TOUCHSCREEN_DEVPATH
#  define CONFIG_NXWM_TOUCHSCREEN_DEVPATH "/dev/input0"
#endif

#ifndef CONFIG_NXWM_TOUCHSCREEN_SIGNO
#  define CONFIG_NXWM_TOUCHSCREEN_SIGNO 5
#endif

#ifndef CONFIG_NXWM_TOUCHSCREEN_LISTENERPRIO
#  define CONFIG_NXWM_TOUCHSCREEN_LISTENERPRIO (SCHED_PRIORITY_DEFAULT + 20)
#endif

#if CONFIG_NXWM_TOUCHSCREEN_LISTENERPRIO <= CONFIG_NXWM_CALIBRATION_LISTENERPRIO
#  warning You should have CONFIG_NXWM_TOUCHSCREEN_LISTENERPRIO > CONFIG_NXWM_CALIBRATION_LISTENERPRIO
#endif

#ifndef CONFIG_NXWM_TOUCHSCREEN_LISTENERSTACK
#  define CONFIG_NXWM_TOUCHSCREEN_LISTENERSTACK 1024
#endif

/* Keyboard device **********************************************************/
/**
 * Keyboard device settings
 *
 * CONFIG_NXWM_KEYBOARD_DEVPATH - The full path to the keyboard device.
 *   Default: "/dev/console"
 * CONFIG_NXWM_KEYBOARD_SIGNO - The realtime signal used to wake up the
 *   touchscreen listener thread.  Default: 6
 * CONFIG_NXWM_KEYBOARD_BUFSIZE - The size of the keyboard read data buffer.
 *   Default: 16
 * CONFIG_NXWM_KEYBOARD_LISTENERPRIO - Priority of the touchscreen listener
 *   thread.  Default: (SCHED_PRIORITY_DEFAULT + 20)
 * CONFIG_NXWM_KEYBOARD_LISTENERSTACK - Keyboard listener thread stack
 *   size.  Default 1024
 */

#ifndef CONFIG_NXWM_KEYBOARD_DEVPATH
#  define CONFIG_NXWM_KEYBOARD_DEVPATH "/dev/console"
#endif

#ifndef CONFIG_NXWM_KEYBOARD_SIGNO
#  define CONFIG_NXWM_KEYBOARD_SIGNO 6
#endif

#ifndef CONFIG_NXWM_KEYBOARD_BUFSIZE
#  define CONFIG_NXWM_KEYBOARD_BUFSIZE 6
#endif

#ifndef CONFIG_NXWM_KEYBOARD_LISTENERPRIO
#  define CONFIG_NXWM_KEYBOARD_LISTENERPRIO (SCHED_PRIORITY_DEFAULT + 20)
#endif

#ifndef CONFIG_NXWM_KEYBOARD_LISTENERSTACK
#  define CONFIG_NXWM_KEYBOARD_LISTENERSTACK 1024
#endif

/* Calibration display ******************************************************/
/**
 * Calibration display settings:
 *
 * CONFIG_NXWM_CALIBRATION_BACKGROUNDCOLOR - The background color of the
 *   touchscreen calibration display.  Default:  Same as
 *   CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR
 * CONFIG_NXWM_CALIBRATION_LINECOLOR - The color of the lines used in the
 *   touchscreen calibration display.  Default:  MKRGB(0, 0, 128) (dark blue)
 * CONFIG_NXWM_CALIBRATION_CIRCLECOLOR - The color of the circle in the
 *   touchscreen calibration display.  Default:  MKRGB(255, 255, 255) (white)
 * CONFIG_NXWM_CALIBRATION_TOUCHEDCOLOR - The color of the circle in the
 *   touchscreen calibration display after the touch is recorder.  Default:
 *   MKRGB(255, 255, 96) (very light yellow)
 * CONFIG_NXWM_CALIBRATION_FONTID - Use this default NxWidgets font ID
 *   instead of the system font ID (NXFONT_DEFAULT).
 * CONFIG_NXWM_CALIBRATION_ICON - The ICON to use for the touchscreen
 *   calibration application.  Default:  NXWidgets::g_calibrationBitmap
 * CONFIG_NXWM_CALIBRATION_SIGNO - The realtime signal used to wake up the
 *   touchscreen calibration thread.  Default: 5
 * CONFIG_NXWM_CALIBRATION_LISTENERPRIO - Priority of the calibration listener
 *   thread.  Default: SCHED_PRIORITY_DEFAULT
 * CONFIG_NXWM_CALIBRATION_LISTENERSTACK - Calibration listener thread stack
 *   size.  Default 2048
 * CONFIG_NXWM_CALIBRATION_MARGIN
 *   The Calbration display consists of a target press offset from the edges
 *   of the display by this number of pixels (in the horizontal direction)
 *   or rows (in the vertical).  The closer that you can comfortably
 *   position the press positions to the edge, the more accurate will be the
 *   linear interpolation (provide that the hardware provides equally good
 *   measurements near the edges).
 */

#ifndef CONFIG_NXWM_CALIBRATION_BACKGROUNDCOLOR
#  define CONFIG_NXWM_CALIBRATION_BACKGROUNDCOLOR CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR
#endif

#ifndef CONFIG_NXWM_CALIBRATION_LINECOLOR
#  define CONFIG_NXWM_CALIBRATION_LINECOLOR MKRGB(0, 0, 128)
#endif

#ifndef CONFIG_NXWM_CALIBRATION_CIRCLECOLOR
#  define CONFIG_NXWM_CALIBRATION_CIRCLECOLOR MKRGB(255, 255, 255)
#endif

#ifndef CONFIG_NXWM_CALIBRATION_TOUCHEDCOLOR
#  define CONFIG_NXWM_CALIBRATION_TOUCHEDCOLOR MKRGB(255, 255, 96)
#endif

#ifndef CONFIG_NXWM_CALIBRATION_FONTID
#  define CONFIG_NXWM_CALIBRATION_FONTID NXFONT_DEFAULT
#endif

#ifndef CONFIG_NXWM_CALIBRATION_ICON
#  define CONFIG_NXWM_CALIBRATION_ICON NXWidgets::g_calibrationBitmap
#endif

#ifndef CONFIG_NXWM_CALIBRATION_SIGNO
#  define CONFIG_NXWM_CALIBRATION_SIGNO 5
#endif

#ifndef CONFIG_NXWM_CALIBRATION_LISTENERPRIO
#  define CONFIG_NXWM_CALIBRATION_LISTENERPRIO SCHED_PRIORITY_DEFAULT
#endif

#ifndef CONFIG_NXWM_CALIBRATION_LISTENERSTACK
#  define CONFIG_NXWM_CALIBRATION_LISTENERSTACK 2048
#endif

#ifndef CONFIG_NXWM_CALIBRATION_MARGIN
#  define CONFIG_NXWM_CALIBRATION_MARGIN 40
#endif

// Calibration sample averaging

#ifndef CONFIG_NXWM_CALIBRATION_AVERAGE
#  undef CONFIG_NXWM_CALIBRATION_AVERAGE
#  undef CONFIG_NXWM_CALIBRATION_NSAMPLES
#  define CONFIG_NXWM_CALIBRATION_NSAMPLES 1
#  undef CONFIG_NXWM_CALIBRATION_DISCARD_MINMAX
#endif

#if !defined(CONFIG_NXWM_CALIBRATION_NSAMPLES) || CONFIG_NXWM_CALIBRATION_NSAMPLES < 2
#  undef CONFIG_NXWM_CALIBRATION_AVERAGE
#  undef CONFIG_NXWM_CALIBRATION_NSAMPLES
#  define CONFIG_NXWM_CALIBRATION_NSAMPLES 1
#  undef CONFIG_NXWM_CALIBRATION_DISCARD_MINMAX
#endif

#if CONFIG_NXWM_CALIBRATION_NSAMPLES < 3
#  undef CONFIG_NXWM_CALIBRATION_DISCARD_MINMAX
#endif

#if CONFIG_NXWM_CALIBRATION_NSAMPLES > 255
#  define CONFIG_NXWM_CALIBRATION_NSAMPLES 255
#endif

#ifdef CONFIG_NXWM_CALIBRATION_DISCARD_MINMAX
#  define NXWM_CALIBRATION_NAVERAGE (CONFIG_NXWM_CALIBRATION_NSAMPLES - 2)
#else
#  define NXWM_CALIBRATION_NAVERAGE CONFIG_NXWM_CALIBRATION_NSAMPLES
#endif

/* Hexcalculator applications ***********************************************/
/**
 * Calibration display settings:
 *
 * CONFIG_NXWM_HEXCALCULATOR_BACKGROUNDCOLOR - The background color of the
 *   calculator display.  Default:  Same as CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR
 * CONFIG_NXWM_HEXCALCULATOR_ICON - The ICON to use for the hex calculator
 *   application.  Default:  NXWidgets::g_calculatorBitmap
 * CONFIG_NXWM_HEXCALCULATOR_FONTID - The font used with the calculator.
 *   Default: CONFIG_NXWM_DEFAULT_FONTID
 */

#ifndef CONFIG_NXWM_HEXCALCULATOR_BACKGROUNDCOLOR
#  define CONFIG_NXWM_HEXCALCULATOR_BACKGROUNDCOLOR CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR
#endif

#ifndef CONFIG_NXWM_HEXCALCULATOR_ICON
#  define CONFIG_NXWM_HEXCALCULATOR_ICON NXWidgets::g_calculatorBitmap
#endif

#ifndef CONFIG_NXWM_HEXCALCULATOR_FONTID
#  define CONFIG_NXWM_HEXCALCULATOR_FONTID CONFIG_NXWM_DEFAULT_FONTID
#endif

/* Media Player application ***********************************************/
/**
 *
 * CONFIG_NXWM_HEXCALCULATOR_BACKGROUNDCOLOR - The background color of the
 *   calculator display.  Default:  Same as CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR
 * CONFIG_NXWM_HEXCALCULATOR_ICON - The ICON to use for the hex calculator
 *   application.  Default:  NXWidgets::g_calculatorBitmap
 * CONFIG_NXWM_HEXCALCULATOR_FONTID - The font used with the calculator.
 *   Default: CONFIG_NXWM_DEFAULT_FONTID
 */

#ifndef CONFIG_NXWM_MEDIAPLAYER_PREFERRED_DEVICE
#  define CONFIG_NXWM_MEDIAPLAYER_PREFERRED_DEVICE "pcm0"
#endif

#ifndef CONFIG_NXWM_MEDIAPLAYER_MEDIAPATH
#  define CONFIG_NXWM_MEDIAPLAYER_MEDIAPATH "/mnt/sdcard"
#endif

#ifndef CONFIG_NXWM_MEDIAPLAYER_BACKGROUNDCOLOR
#  define CONFIG_NXWM_MEDIAPLAYER_BACKGROUNDCOLOR CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR
#endif

#ifndef CONFIG_NXWM_MEDIAPLAYER_XSPACING
#  define CONFIG_NXWM_MEDIAPLAYER_XSPACING 12
#endif

#ifndef CONFIG_NXWM_MEDIAPLAYER_YSPACING
#  define CONFIG_NXWM_MEDIAPLAYER_YSPACING 8
#endif

#ifndef CONFIG_NXWM_MEDIAPLAYER_VOLUMESTEP
#  define CONFIG_NXWM_MEDIAPLAYER_VOLUMESTEP 5
#endif

#ifndef CONFIG_NXWM_MEDIAPLAYER_MINVOLUMEHEIGHT
#  define CONFIG_NXWM_MEDIAPLAYER_MINVOLUMEHEIGHT 6
#endif

#ifndef CONFIG_NXWM_MEDIAPLAYER_VOLUMECOLOR
#  define CONFIG_NXWM_MEDIAPLAYER_VOLUMECOLOR MKRGB(63,90,192)
#endif

#ifndef CONFIG_NXWM_MEDIAPLAYER_ICON
#  define CONFIG_NXWM_MEDIAPLAYER_ICON NXWidgets::g_mediaplayerBitmap
#endif

#ifndef CONFIG_NXWM_MPLAYER_FWD_ICON
#  define CONFIG_NXWM_MPLAYER_FWD_ICON NXWidgets::g_mplayerFwdBitmap
#endif

#ifndef CONFIG_NXWM_MPLAYER_PLAY_ICON
#  define CONFIG_NXWM_MPLAYER_PLAY_ICON NXWidgets::g_mplayerPlayBitmap
#endif

#ifndef CONFIG_NXWM_MPLAYER_PAUSE_ICON
#  define CONFIG_NXWM_MPLAYER_PAUSE_ICON NXWidgets::g_mplayerPauseBitmap
#endif

#ifndef CONFIG_NXWM_MPLAYER_REW_ICON
#  define CONFIG_NXWM_MPLAYER_REW_ICON NXWidgets::g_mplayerRewBitmap
#endif

#ifndef CONFIG_NXWM_MPLAYER_VOL_ICON
#  define CONFIG_NXWM_MPLAYER_VOL_ICON NXWidgets::g_mplayerVolBitmap
#endif

#ifndef CONFIG_NXWM_MEDIAPLAYER_FONTID
#  define CONFIG_NXWM_MEDIAPLAYER_FONTID CONFIG_NXWM_DEFAULT_FONTID
#endif

/****************************************************************************
 * Global Function Prototypes
 ****************************************************************************/

#endif // __APPS_INCLUDE_GRAPHICS_NXWM_NXWMCONFIG_HXX
