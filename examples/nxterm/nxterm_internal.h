/****************************************************************************
 * apps/examples/nxterm/nxterm_internal.h
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

#ifndef __APPS_EXAMPLES_NXTERM_NXTERM_INTERNAL_H
#define __APPS_EXAMPLES_NXTERM_NXTERM_INTERNAL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <semaphore.h>

#include <nuttx/video/rgbcolors.h>

#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxglib.h>
#include <nuttx/nx/nxtk.h>
#include <nuttx/nx/nxterm.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

/* Need NX graphics support */

#ifndef CONFIG_NX
#  error "NX is not enabled (CONFIG_NX=y)"
#endif

/* Can't do the NxTerm example if the NxTerm driver is not built */

#ifndef CONFIG_NXTERM
#  error "NxTerm is not enabled (CONFIG_NXTERM=y)"
#endif

/* If there is no NSH console, then why are we running this example? */

#ifndef CONFIG_NSH_CONSOLE
#  warning "Expected CONFIG_NSH_CONSOLE=y"
#endif

/* If not specified, assume that the hardware supports one video plane */

#if CONFIG_NX_NPLANES != 1
#  error "Only CONFIG_NX_NPLANES==1 supported"
#endif

/* Pixel depth.  If none provided, pick the smallest enabled pixel depth */

#if defined(CONFIG_EXAMPLES_NXTERM_BPP) && \
    CONFIG_EXAMPLES_NXTERM_BPP != 1 && \
    CONFIG_EXAMPLES_NXTERM_BPP != 2 && \
    CONFIG_EXAMPLES_NXTERM_BPP != 4 && \
    CONFIG_EXAMPLES_NXTERM_BPP != 8 && \
    CONFIG_EXAMPLES_NXTERM_BPP != 16 && \
    CONFIG_EXAMPLES_NXTERM_BPP != 32
#  error Invalid selection for CONFIG_EXAMPLES_NXTERM_BPP
#  undef CONFIG_EXAMPLES_NXTERM_BPP
#endif

#ifndef CONFIG_EXAMPLES_NXTERM_BPP
#  if !defined(CONFIG_NX_DISABLE_1BPP)
#    define CONFIG_EXAMPLES_NXTERM_BPP 1
#  elif !defined(CONFIG_NX_DISABLE_2BPP)
#    define CONFIG_EXAMPLES_NXTERM_BPP 2
#  elif !defined(CONFIG_NX_DISABLE_4BPP)
#    define CONFIG_EXAMPLES_NXTERM_BPP 4
#  elif !defined(CONFIG_NX_DISABLE_8BPP)
#    define CONFIG_EXAMPLES_NXTERM_BPP 8
#  elif !defined(CONFIG_NX_DISABLE_16BPP)
#    define CONFIG_EXAMPLES_NXTERM_BPP 16
#  elif !defined(CONFIG_NX_DISABLE_24BPP)
#    define CONFIG_EXAMPLES_NXTERM_BPP 24
#  elif !defined(CONFIG_NX_DISABLE_32BPP)
#    define CONFIG_EXAMPLES_NXTERM_BPP 32
#  else
#    error "No pixel depth provided"
#  endif
#endif

/* Background color (default is darker royal blue) */

#ifndef CONFIG_EXAMPLES_NXTERM_BGCOLOR
#  if CONFIG_EXAMPLES_NXTERM_BPP == 24 || CONFIG_EXAMPLES_NXTERM_BPP == 32
#    define CONFIG_EXAMPLES_NXTERM_BGCOLOR RGBTO24(39, 64, 139)
#  elif CONFIG_EXAMPLES_NXTERM_BPP == 16
#    define CONFIG_EXAMPLES_NXTERM_BGCOLOR RGBTO16(39, 64, 139)
#  else
#    define CONFIG_EXAMPLES_NXTERM_BGCOLOR RGBTO8(39, 64, 139)
# endif
#endif

/* Window color (lighter steel blue) */

#ifndef CONFIG_EXAMPLES_NXTERM_WCOLOR
#  if CONFIG_EXAMPLES_NXTERM_BPP == 24 || CONFIG_EXAMPLES_NXTERM_BPP == 32
#    define CONFIG_EXAMPLES_NXTERM_WCOLOR RGBTO24(202, 225, 255)
#  elif CONFIG_EXAMPLES_NXTERM_BPP == 16
#    define CONFIG_EXAMPLES_NXTERM_WCOLOR RGBTO16(202, 225, 255)
#  else
#    define CONFIG_EXAMPLES_NXTERM_WCOLOR RGBTO8(202, 225, 255)
# endif
#endif

/* Toolbar color (medium grey) */

#ifndef CONFIG_EXAMPLES_NXTERM_TBCOLOR
#  if CONFIG_EXAMPLES_NXTERM_BPP == 24 || CONFIG_EXAMPLES_NXTERM_BPP == 32
#    define CONFIG_EXAMPLES_NXTERM_TBCOLOR RGBTO24(188, 188, 188)
#  elif CONFIG_EXAMPLES_NXTERM_BPP == 16
#    define CONFIG_EXAMPLES_NXTERM_TBCOLOR RGBTO16(188, 188, 188)
#  else
#    define CONFIG_EXAMPLES_NXTERM_TBCOLOR RGBTO8(188, 188, 188)
#  endif
#endif

/* Font ID */

#ifndef CONFIG_EXAMPLES_NXTERM_FONTID
#  define CONFIG_EXAMPLES_NXTERM_FONTID NXFONT_DEFAULT
#endif

/* Font color */

#ifndef CONFIG_EXAMPLES_NXTERM_FONTCOLOR
#  if CONFIG_EXAMPLES_NXTERM_BPP == 24 || CONFIG_EXAMPLES_NXTERM_BPP == 32
#    define CONFIG_EXAMPLES_NXTERM_FONTCOLOR RGBTO24(0, 0, 0)
#  elif CONFIG_EXAMPLES_NXTERM_BPP == 16
#    define CONFIG_EXAMPLES_NXTERM_FONTCOLOR RGBTO16(0, 0, 0)
#  else
#    define CONFIG_EXAMPLES_NXTERM_FONTCOLOR RGBTO8(0, 0, 0)
#  endif
#endif

/* Height of the toolbar */

#ifndef CONFIG_EXAMPLES_NXTERM_TOOLBAR_HEIGHT
#  define CONFIG_EXAMPLES_NXTERM_TOOLBAR_HEIGHT 16
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
#ifndef CONFIG_EXAMPLES_NXTERM_STACKSIZE
#  define CONFIG_EXAMPLES_NXTERM_STACKSIZE 2048
#endif
#ifndef CONFIG_EXAMPLES_NXTERM_LISTENERPRIO
#  define CONFIG_EXAMPLES_NXTERM_LISTENERPRIO 100
#endif
#ifndef CONFIG_EXAMPLES_NXTERM_CLIENTPRIO
#  define CONFIG_EXAMPLES_NXTERM_CLIENTPRIO 100
#endif
#ifndef CONFIG_EXAMPLES_NXTERM_SERVERPRIO
#  define CONFIG_EXAMPLES_NXTERM_SERVERPRIO 120
#endif
#ifndef CONFIG_EXAMPLES_NXTERM_NOTIFYSIGNO
#  define CONFIG_EXAMPLES_NXTERM_NOTIFYSIGNO 4
#endif

/* NX Console Device */

#ifndef CONFIG_EXAMPLES_NXTERM_MINOR
#  define CONFIG_EXAMPLES_NXTERM_MINOR 0
#endif

#ifndef CONFIG_EXAMPLES_NXTERM_DEVNAME
#  define CONFIG_EXAMPLES_NXTERM_DEVNAME "/dev/nxterm0"
#endif

/* NxTerm task */

#ifndef CONFIG_EXAMPLES_NXTERM_PRIO
#  define CONFIG_EXAMPLES_NXTERM_PRIO SCHED_PRIORITY_DEFAULT
#endif

#ifndef CONFIG_EXAMPLES_NXTERM_STACKSIZE
#  define CONFIG_EXAMPLES_NXTERM_STACKSIZE 2048
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* All example global variables are retained in a structure to minimize
 * the chance of name collisions.
 */

struct nxterm_state_s
{
  volatile bool          haveres;   /* True: Have screen resolution */
  volatile bool          connected; /* True: Connected to server */
  sem_t                  eventsem;  /* Control waiting for display events */
  pid_t                  pid;       /* Console task ID */
  NXHANDLE               hnx;       /* The connection handler */
  NXTKWINDOW             hwnd;      /* The window */
  NXTERM                 hdrvr;     /* The console driver */
  struct nxterm_window_s wndo;      /* Describes the window */
  nxgl_coord_t           xres;      /* Screen X resolution */
  nxgl_coord_t           yres;      /* Screen Y resolution */
  struct nxgl_size_s     wsize;     /* Window size */
  struct nxgl_point_s    wpos;      /* Window position */
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* All example global variables are retained in a structure to minimize
 * the chance of name collisions.
 */

extern struct nxterm_state_s g_nxterm_vars;

/* NX callback vtables */

extern const struct nx_callback_s g_nxtermcb;
extern const struct nx_callback_s g_nxtoolcb;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Server thread support */

FAR void *nxterm_listener(FAR void *arg);

#endif /* __APPS_EXAMPLES_NXTERM_NXTERM_INTERNAL_H */
