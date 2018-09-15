/****************************************************************************
 * examples/nxdemo/nxdemo.h
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
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

#ifndef __APPS_EXAMPLES_NXDEMO_NXDEMO_H
#define __APPS_EXAMPLES_NXDEMO_NXDEMO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <semaphore.h>

#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxglib.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

#ifndef CONFIG_NX
#  error "NX is not enabled (CONFIG_NX)"
#endif

#ifndef CONFIG_EXAMPLES_NXDEMO_VPLANE
#    define CONFIG_EXAMPLES_NXDEMO_VPLANE 0
#endif

#ifndef CONFIG_EXAMPLES_NXDEMO_BPP
#  define CONFIG_EXAMPLES_NXDEMO_BPP 32
#endif

#ifndef CONFIG_EXAMPLES_NXDEMO_BGCOLOR
#  if CONFIG_EXAMPLES_NXDEMO_BPP == 24 || CONFIG_EXAMPLES_NXDEMO_BPP == 32
#    define CONFIG_EXAMPLES_NXDEMO_BGCOLOR 0x007b68ee
#  elif CONFIG_EXAMPLES_NXDEMO_BPP == 16
#    define CONFIG_EXAMPLES_NXDEMO_BGCOLOR 0x7b5d
#  elif CONFIG_EXAMPLES_NXDEMO_BPP < 8
#    define CONFIG_EXAMPLES_NXDEMO_BGCOLOR 0x00
#  else
#    define CONFIG_EXAMPLES_NXDEMO_BGCOLOR ' '
# endif
#endif

#ifndef CONFIG_EXAMPLES_NXDEMO_DRAWCOLOR
#  if CONFIG_EXAMPLES_NXDEMO_BPP == 24 || CONFIG_EXAMPLES_NXDEMO_BPP == 32
#    define CONFIG_EXAMPLES_NXDEMO_DRAWCOLOR 0xFFFFFFFF
#  elif CONFIG_EXAMPLES_NXDEMO_BPP == 16
#    define CONFIG_EXAMPLES_NXDEMO_DRAWCOLOR 0xFFFF
#  elif CONFIG_EXAMPLES_NXDEMO_BPP < 8
#    define CONFIG_EXAMPLES_NXDEMO_DRAWCOLOR 0xFF
#  else
#    define CONFIG_EXAMPLES_NXDEMO_DRAWCOLOR 'F'
#  endif
#endif

/* Multi-user NX support */

#ifdef CONFIG_DISABLE_MQUEUE
#  error "The multi-threaded example requires MQ support (CONFIG_DISABLE_MQUEUE=n)"
#endif
#ifdef CONFIG_DISABLE_SIGNALS
#  error "This example requires signal support (CONFIG_DISABLE_SIGNALS=n)"
#endif
#ifdef CONFIG_DISABLE_PTHREAD
#  error "This example requires pthread support (CONFIG_DISABLE_PTHREAD=n)"
#endif
#ifndef CONFIG_NX_BLOCKING
#  error "This example depends on CONFIG_NX_BLOCKING"
#endif
#ifndef CONFIG_EXAMPLES_NXDEMO_LISTENER_STACKSIZE
#  define CONFIG_EXAMPLES_NXDEMO_LISTENER_STACKSIZE 2048
#endif
#ifndef CONFIG_EXAMPLES_NXDEMO_LISTENERPRIO
#  define CONFIG_EXAMPLES_NXDEMO_LISTENERPRIO 100
#endif
#ifndef CONFIG_EXAMPLES_NXDEMO_CLIENTPRIO
#  define CONFIG_EXAMPLES_NXDEMO_CLIENTPRIO 100
#endif
#ifndef CONFIG_EXAMPLES_NXDEMO_SERVERPRIO
#  define CONFIG_EXAMPLES_NXDEMO_SERVERPRIO 120
#endif
#ifndef CONFIG_EXAMPLES_NXDEMO_NOTIFYSIGNO
#  define CONFIG_EXAMPLES_NXDEMO_NOTIFYSIGNO 4
#endif

/* Image Information ********************************************************/

#define IMAGE_HEIGHT       160  /* Number of rows in the raw image */
#define IMAGE_WIDTH        160  /* Number of columns in the raw image */

#if defined(CONFIG_EXAMPLES_NXIMAGE_XSCALEp5)
#  define SCALED_WIDTH     80   /* Number of columns in the scaled image */
#elif defined(CONFIG_EXAMPLES_NXIMAGE_XSCALE1p5)
#  define SCALED_WIDTH     240  /* Number of columns in the scaled image */
#elif defined(CONFIG_EXAMPLES_NXIMAGE_XSCALE2p0)
#  define SCALED_WIDTH     320  /* Number of columns in the scaled image */
#else
#  define SCALED_WIDTH     160  /* Number of columns in the scaled image */
#endif

#if defined(CONFIG_EXAMPLES_NXIMAGE_YSCALEp5)
#  define SCALED_HEIGHT    80   /* Number of rows in the scaled image */
#elif defined(CONFIG_EXAMPLES_NXIMAGE_YSCALE1p5)
#  define SCALED_HEIGHT    240  /* Number of rows in the scaled image */
#elif defined(CONFIG_EXAMPLES_NXIMAGE_YSCALE2p0)
#  define SCALED_HEIGHT    320  /* Number of rows in the scaled image */
#else
#  define SCALED_HEIGHT    160  /* Number of rows in the scaled image */
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum exitcode_e
{
  NXEXIT_SUCCESS = 0,
  NXEXIT_EXTINITIALIZE,
  NXEXIT_FBINITIALIZE,
  NXEXIT_FBGETVPLANE,
  NXEXIT_LCDINITIALIZE,
  NXEXIT_LCDGETDEV,
  NXEXIT_NXOPEN,
  NXEXIT_FONTOPEN,
  NXEXIT_NXREQUESTBKGD,
  NXEXIT_NXSETBGCOLOR
};

/* Describes one cached glyph bitmap */

struct nxdemo_glyph_s
{
  uint8_t code;                        /* Character code */
  uint8_t height;                      /* Height of this glyph (in rows) */
  uint8_t width;                       /* Width of this glyph (in pixels) */
  uint8_t stride;                      /* Width of the glyph row (in bytes) */
  uint8_t usecnt;                      /* Use count */
  FAR uint8_t *bitmap;                 /* Allocated bitmap memory */
};

/* Describes on character on the display */

struct nxdemo_bitmap_s
{
  uint8_t code;                        /* Character code */
  uint8_t flags;                       /* See BMFLAGS_* */
  struct nxgl_point_s pos;             /* Character position */
};

struct nxdemo_data_s
{
  /* The NX handles */

  NXHANDLE hnx;
  NXHANDLE hbkgd;
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

/* NXDEMO state data */

extern struct nxdemo_data_s g_nxdemo;

/* NX callback vtables */

extern const struct nx_callback_s g_nxdemocb;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* NX server/listener thread */

FAR void *nxdemo_listener(FAR void *arg);

/* Background window interfaces */

void nxdemo_hello(NXWINDOW hwnd);

#endif /* __APPS_EXAMPLES_NXDEMO_NXDEMO_H */
