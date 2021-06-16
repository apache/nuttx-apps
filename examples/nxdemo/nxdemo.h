/****************************************************************************
 * apps/examples/nxdemo/nxdemo.h
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
