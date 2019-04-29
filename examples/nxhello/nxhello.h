/****************************************************************************
 * examples/nxhello/nxhello.h
 *
 *   Copyright (C) 2011, 2015, 2017 Gregory Nutt. All rights reserved.
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

#ifndef __APPS_EXAMPLES_NXHELLO_NXHELLO_H
#define __APPS_EXAMPLES_NXHELLO_NXHELLO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <semaphore.h>

#include <nuttx/nx/nxglib.h>
#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxfonts.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

#ifndef CONFIG_NX
#  error "NX is not enabled (CONFIG_NX)"
#endif

/* If not specified, assume that the hardware supports one video plane */

#if CONFIG_NX_NPLANES != 1
#  error "Only CONFIG_NX_NPLANES==1 supported"
#endif

#ifndef CONFIG_EXAMPLES_NXHELLO_BGCOLOR
#  if CONFIG_EXAMPLES_NXHELLO_BPP == 24 || CONFIG_EXAMPLES_NXHELLO_BPP == 32
#    define CONFIG_EXAMPLES_NXHELLO_BGCOLOR 0x007b68ee
#  elif CONFIG_EXAMPLES_NXHELLO_BPP == 16
#    define CONFIG_EXAMPLES_NXHELLO_BGCOLOR 0x7b5d
#  elif CONFIG_EXAMPLES_NXHELLO_BPP < 8
#    define CONFIG_EXAMPLES_NXHELLO_BGCOLOR 0x00
#  else
#    define CONFIG_EXAMPLES_NXHELLO_BGCOLOR ' '
# endif
#endif

#ifndef CONFIG_EXAMPLES_NXHELLO_FONTID
#  define CONFIG_EXAMPLES_NXHELLO_FONTID NXFONT_DEFAULT
#endif

#ifndef CONFIG_EXAMPLES_NXHELLO_FONTCOLOR
#  if CONFIG_EXAMPLES_NXHELLO_BPP == 24 || CONFIG_EXAMPLES_NXHELLO_BPP == 32
#    define CONFIG_EXAMPLES_NXHELLO_FONTCOLOR 0x00000000
#  elif CONFIG_EXAMPLES_NXHELLO_BPP == 16
#    define CONFIG_EXAMPLES_NXHELLO_FONTCOLOR 0x0000
#  elif CONFIG_EXAMPLES_NXHELLO_BPP < 1
#    define CONFIG_EXAMPLES_NXHELLO_FONTCOLOR 0x01
#  else
#    define CONFIG_EXAMPLES_NXHELLO_FONTCOLOR 'F'
#  endif
#endif

/* NX server support */

#ifdef CONFIG_DISABLE_MQUEUE
#  error "The multi-threaded example requires MQ support (CONFIG_DISABLE_MQUEUE=n)"
#endif
#ifdef CONFIG_DISABLE_PTHREAD
#  error "This example requires pthread support (CONFIG_DISABLE_PTHREAD=n)"
#endif
#ifndef CONFIG_NX_BLOCKING
#  error "This example depends on CONFIG_NX_BLOCKING"
#endif
#ifndef CONFIG_EXAMPLES_NXHELLO_LISTENER_STACKSIZE
#  define CONFIG_EXAMPLES_NXHELLO_LISTENER_STACKSIZE 2048
#endif
#ifndef CONFIG_EXAMPLES_NXHELLO_LISTENERPRIO
#  define CONFIG_EXAMPLES_NXHELLO_LISTENERPRIO 100
#endif
#ifndef CONFIG_EXAMPLES_NXHELLO_CLIENTPRIO
#  define CONFIG_EXAMPLES_NXHELLO_CLIENTPRIO 100
#endif
#ifndef CONFIG_EXAMPLES_NXHELLO_SERVERPRIO
#  define CONFIG_EXAMPLES_NXHELLO_SERVERPRIO 120
#endif
#ifndef CONFIG_EXAMPLES_NXHELLO_NOTIFYSIGNO
#  define CONFIG_EXAMPLES_NXHELLO_NOTIFYSIGNO 4
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum exitcode_e
{
  NXEXIT_SUCCESS = 0,
  NXEXIT_INIT,
  NXEXIT_FONTOPEN,
  NXEXIT_NXREQUESTBKGD,
  NXEXIT_NXSETBGCOLOR
};

/* Describes one cached glyph bitmap */

struct nxhello_glyph_s
{
  uint8_t code;                        /* Character code */
  uint8_t height;                      /* Height of this glyph (in rows) */
  uint8_t width;                       /* Width of this glyph (in pixels) */
  uint8_t stride;                      /* Width of the glyph row (in bytes) */
  uint8_t usecnt;                      /* Use count */
  FAR uint8_t *bitmap;                 /* Allocated bitmap memory */
};

/* Describes on character on the display */

struct nxhello_bitmap_s
{
  uint8_t code;                        /* Character code */
  uint8_t flags;                       /* See BMFLAGS_* */
  struct nxgl_point_s pos;             /* Character position */
};

struct nxhello_data_s
{
  /* The NX handles */

  NXHANDLE hnx;
  NXHANDLE hbkgd;
  NXHANDLE hfont;
  bool connected;

  /* The screen resolution */

  nxgl_coord_t xres;
  nxgl_coord_t yres;

  volatile bool havepos;
  sem_t eventsem;
  volatile int code;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* NXHELLO state data */

extern struct nxhello_data_s g_nxhello;

/* NX callback vtables */

extern const struct nx_callback_s g_nxhellocb;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* NX event listener */

FAR void *nxhello_listener(FAR void *arg);

/* Background window interfaces */

void nxhello_hello(NXWINDOW hwnd);

#endif /* __APPS_EXAMPLES_NXHELLO_NXHELLO_H */
