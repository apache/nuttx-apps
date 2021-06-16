/****************************************************************************
 * apps/examples/nxhello/nxhello.h
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
