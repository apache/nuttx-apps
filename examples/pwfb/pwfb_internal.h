/****************************************************************************
 * examples/pwfb/pwfb_internal.h
 *
 *   Copyright (C) 2019 Gregory Nutt. All rights reserved.
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
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
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

#ifndef __EXAMPLES_PWFB_PWFB_INTERNAL_H
#define __EXAMPLES_PWFB_PWFB_INTERNAL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <semaphore.h>
#include <fixedmath.h>

#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxtk.h>
#include <nuttx/nx/nxfonts.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

/* Required NX Server Settings */

#ifndef CONFIG_NX
#  error NX is not enabled (CONFIG_NX)
#endif

#ifdef CONFIG_DISABLE_MQUEUE
#  error The multi-threaded example requires MQ support (CONFIG_DISABLE_MQUEUE=n)
#endif

#ifdef CONFIG_DISABLE_PTHREAD
#  error This example requires pthread support (CONFIG_DISABLE_PTHREAD=n)
#endif

#ifndef CONFIG_NX_BLOCKING
#  error This example depends on CONFIG_NX_BLOCKING
#endif

/* WARNING:  Verbose graphics debug output interferes with this test because
 * it introduces some weird timing.  The test probably should use
 * nx_synchronize() to keep syncrhonization even with the added delays.
 */

#ifdef CONFIG_DEBUG_GRAPHICS_INFO
#  warning Verbose graphics debug output interferes with this test.
#endif

/* Task priorities */

#if CONFIG_EXAMPLES_PWFB_CLIENT_PRIO >= CONFIG_EXAMPLES_PWFB_LISTENER_PRIO || \
    CONFIG_EXAMPLES_PWFB_CLIENT_PRIO >= CONFIG_NXSTART_SERVERPRIO
#  warning Client priority must be lower than both the listener and server priorities
#endif

#if CONFIG_EXAMPLES_PWFB_LISTENER_PRIO >= CONFIG_NXSTART_SERVERPRIO
#  warning Listener priority must be lower than the server priority
#endif

/* Default colors */

#ifndef CONFIG_EXAMPLES_PWFB_BGCOLOR
#  if CONFIG_EXAMPLES_PWFB_BPP == 24 || CONFIG_EXAMPLES_PWFB_BPP == 32
#    define CONFIG_EXAMPLES_PWFB_BGCOLOR 0x007b68ee
#  elif CONFIG_EXAMPLES_PWFB_BPP == 16
#    define CONFIG_EXAMPLES_PWFB_BGCOLOR 0x7b5d
#  else
#    define CONFIG_EXAMPLES_PWFB_BGCOLOR ' '
# endif
#endif

#ifndef CONFIG_EXAMPLES_PWFB_COLOR1
#  if CONFIG_EXAMPLES_PWFB_BPP == 24 || CONFIG_EXAMPLES_PWFB_BPP == 32
#    define CONFIG_EXAMPLES_PWFB_COLOR1 0x00e6e6fa
#  elif CONFIG_EXAMPLES_PWFB_BPP == 16
#    define CONFIG_EXAMPLES_PWFB_COLOR1 0xe73f
#  else
#    define CONFIG_EXAMPLES_PWFB_COLOR1 '1'
# endif
#endif

#ifndef CONFIG_EXAMPLES_PWFB_COLOR2
#  if CONFIG_EXAMPLES_PWFB_BPP == 24 || CONFIG_EXAMPLES_PWFB_BPP == 32
#    define CONFIG_EXAMPLES_PWFB_COLOR2 0x00dcdcdc
#  elif CONFIG_EXAMPLES_PWFB_BPP == 16
#    define CONFIG_EXAMPLES_PWFB_COLOR2 0xdefb
#  else
#    define CONFIG_EXAMPLES_PWFB_COLOR2 '2'
# endif
#endif

#ifndef CONFIG_EXAMPLES_PWFB_COLOR3
#  if CONFIG_EXAMPLES_PWFB_BPP == 24 || CONFIG_EXAMPLES_PWFB_BPP == 32
#    define CONFIG_EXAMPLES_PWFB_COLOR2 0x00ffecb3
#  elif CONFIG_EXAMPLES_PWFB_BPP == 16
#    define CONFIG_EXAMPLES_PWFB_COLOR2 0xff76
#  else
#    define CONFIG_EXAMPLES_PWFB_COLOR2 '3'
# endif
#endif

#ifndef CONFIG_EXAMPLES_PWFB_TBCOLOR
#  if CONFIG_EXAMPLES_PWFB_BPP == 24 || CONFIG_EXAMPLES_PWFB_BPP == 32
#    define CONFIG_EXAMPLES_PWFB_TBCOLOR 0x00a9a9a9
#  elif CONFIG_EXAMPLES_PWFB_BPP == 16
#    define CONFIG_EXAMPLES_PWFB_TBCOLOR 0xad55
#  else
#    define CONFIG_EXAMPLES_PWFB_TBCOLOR 'T'
#  endif
#endif

#ifndef CONFIG_EXAMPLES_PWFB_FONTCOLOR
#  if CONFIG_EXAMPLES_PWFB_BPP == 24 || CONFIG_EXAMPLES_PWFB_BPP == 32
#    define CONFIG_EXAMPLES_PWFB_FONTCOLOR 0x00000000
#  elif CONFIG_EXAMPLES_PWFB_BPP == 16
#    define CONFIG_EXAMPLES_PWFB_FONTCOLOR 0x0000
#  else
#    define CONFIG_EXAMPLES_PWFB_FONTCOLOR 'F'
#  endif
#endif

/* Cursor timing */

#if CONFIG_EXAMPLES_PWFB_RATECONTROL > 0
#  define CURSOR_MOVING_DELAY     (3000 / CONFIG_EXAMPLES_PWFB_RATECONTROL)
#  define CURSOR_STATIONARY_DELAY (2000 / CONFIG_EXAMPLES_PWFB_RATECONTROL)
#  define CURSOR_BLINKING_DELAY   (5000 / CONFIG_EXAMPLES_PWFB_RATECONTROL)
#  define CURSOR_BLINK_DELAY      ( 500 / CONFIG_EXAMPLES_PWFB_RATECONTROL)
#else
#  define CURSOR_MOVING_DELAY     (0)
#  define CURSOR_STATIONARY_DELAY (0)
#  define CURSOR_BLINKING_DELAY   (0)
#  define CURSOR_BLINK_DELAY      (0)
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Describes the unique state of one window */

struct pwfb_window_s
{
  NXTKWINDOW hwnd;                         /* Window handle */
  nxgl_mxpixel_t color[CONFIG_NX_NPLANES]; /* Window color */
  FCACHE fcache;                           /* Font cache handle */
  b16_t xmax;                              /* Max X position */
  b16_t ymax;                              /* Max Y position */
  b16_t xpos;                              /* Current X position */
  b16_t ypos;                              /* Current Y position */
  b16_t deltax;                            /* Current X speed */
  b16_t deltay;                            /* Current Y speed */
};

#ifdef CONFIG_NX_SWCURSOR
/* We will run the cursor in 3 different states:
 *
 * 1. Moving
 * 2. Stationary
 * 3. Enabling/disabling
 */

enum pfwb_cursor_state_s
{
  PFWB_CURSOR_MOVING = 0,
  PFWB_CURSOR_STATIONARY,
  PFWB_CURSOR_BLINKING
};

/* Describes the unique state of the cursor */

struct pwfb_cursor_s
{
  enum pfwb_cursor_state_s state;          /* Current cursor state */
  bool visible;                            /* True:  The cursor is visible */
  int countdown;                           /* Countdown until next state */
  int blinktime;                           /* Time remaining enabled/disabled */
  b16_t xmax;                              /* Max X position */
  b16_t ymax;                              /* Max Y position */
  b16_t xpos;                              /* Current X position */
  b16_t ypos;                              /* Current Y position */
  b16_t deltax;                            /* Current X speed */
  b16_t deltay;                            /* Current Y speed */
};
#endif
/* Describes the overall state of the example */

struct pwfb_state_s
{
  /* NX server */

  volatile bool haveres;                   /* True:  Have screen resolution */
  volatile bool connected;                 /* True:  Connected to server */
  sem_t semevent;                          /* Event wait semaphore */
  NXHANDLE hnx;                            /* Connection handle */

  /* Font */

  NXHANDLE hfont;                          /* The font handle */

  /* Graphics hardware */

  nxgl_coord_t xres;                       /* Horizontal resolution */
  nxgl_coord_t yres;                       /* Vertical resolution */

  /* Common toolbar properties */

  nxgl_mxpixel_t color[CONFIG_NX_NPLANES]; /* Toolbar color */

  /* Font properties */

  uint8_t fheight;                         /* Max height of a font in pixels */
  uint8_t fwidth;                          /* Max width of a font in pixels */
  uint8_t spwidth;                         /* The width of a space */

#ifdef CONFIG_NX_SWCURSOR
  /* Cursor-specific state */

  struct pwfb_cursor_s cursor;
#endif

  /* Window-specific state */

  struct pwfb_window_s wndo[CONFIG_EXAMPLES_PWFB_NWINDOWS];
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* NX callback vtables */

extern const struct nx_callback_s g_pwfb_wncb;
extern const struct nx_callback_s g_pwfb_tbcb;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

FAR void *pwfb_listener(FAR void *arg);
bool pwfb_motion(FAR struct pwfb_state_s *st);

#endif /* __EXAMPLES_PWFB_PWFB_INTERNAL_H */
