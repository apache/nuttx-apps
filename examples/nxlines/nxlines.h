/****************************************************************************
 * examples/nxlines/nxlines.h
 *
 *   Copyright (C) 2011-2012, 2015, 2017 Gregory Nutt. All rights reserved.
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

#ifndef __APPS_EXAMPLES_NXLINES_NXLINES_H
#define __APPS_EXAMPLES_NXLINES_NXLINES_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <semaphore.h>

#include <nuttx/nx/nxglib.h>
#include <nuttx/nx/nx.h>
#include <nuttx/video/rgbcolors.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

#ifndef CONFIG_NX
#  error "NX is not enabled (CONFIG_NX)"
#endif

#ifndef CONFIG_EXAMPLES_NXLINES_BPP
#  define CONFIG_EXAMPLES_NXLINES_BPP 16
#endif

#ifndef CONFIG_EXAMPLES_NXLINES_BGCOLOR
#  if CONFIG_EXAMPLES_NXLINES_BPP == 24 || CONFIG_EXAMPLES_NXLINES_BPP == 32
#    define CONFIG_EXAMPLES_NXLINES_BGCOLOR RGB24_DARKGREEN
#  elif CONFIG_EXAMPLES_NXLINES_BPP == 16
#    define CONFIG_EXAMPLES_NXLINES_BGCOLOR RGB16_DARKGREEN
#  else
#    define CONFIG_EXAMPLES_NXLINES_BGCOLOR RGB8_DARKGREEN
#  endif
#endif

#ifndef CONFIG_EXAMPLES_NXLINES_LINEWIDTH
#  define CONFIG_EXAMPLES_NXLINES_LINEWIDTH 16
#endif

#ifndef CONFIG_EXAMPLES_NXLINES_LINECOLOR
#  if CONFIG_EXAMPLES_NXLINES_BPP == 24 || CONFIG_EXAMPLES_NXLINES_BPP == 32
#    define CONFIG_EXAMPLES_NXLINES_LINECOLOR RGB24_YELLOW
#  elif CONFIG_EXAMPLES_NXLINES_BPP == 16
#    define CONFIG_EXAMPLES_NXLINES_LINECOLOR RGB16_YELLOW
#  else
#    define CONFIG_EXAMPLES_NXLINES_LINECOLOR RGB8_YELLOW
#  endif
#endif

#ifndef CONFIG_EXAMPLES_NXLINES_BORDERWIDTH
#  define CONFIG_EXAMPLES_NXLINES_BORDERWIDTH 16
#endif

#ifndef CONFIG_EXAMPLES_NXLINES_BORDERCOLOR
#  if CONFIG_EXAMPLES_NXLINES_BPP == 24 || CONFIG_EXAMPLES_NXLINES_BPP == 32
#    define CONFIG_EXAMPLES_NXLINES_BORDERCOLOR RGB24_YELLOW
#  elif CONFIG_EXAMPLES_NXLINES_BPP == 16
#    define CONFIG_EXAMPLES_NXLINES_BORDERCOLOR RGB16_YELLOW
#  else
#    define CONFIG_EXAMPLES_NXLINES_BORDERCOLOR RGB8_YELLOW
#  endif
#endif

#ifndef CONFIG_EXAMPLES_NXLINES_CIRCLECOLOR
#  if CONFIG_EXAMPLES_NXLINES_BPP == 24 || CONFIG_EXAMPLES_NXLINES_BPP == 32
#    define CONFIG_EXAMPLES_NXLINES_CIRCLECOLOR RGB24_BEIGE
#  elif CONFIG_EXAMPLES_NXLINES_BPP == 16
#    define CONFIG_EXAMPLES_NXLINES_CIRCLECOLOR RGB16_BEIGE
#  else
#    define CONFIG_EXAMPLES_NXLINES_CIRCLECOLOR RGB8_YELLOW
#  endif
#endif

/* NX server support */

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
#ifndef CONFIG_EXAMPLES_NXLINES_LISTENER_STACKSIZE
#  define CONFIG_EXAMPLES_NXLINES_LISTENER_STACKSIZE 2048
#endif
#ifndef CONFIG_EXAMPLES_NXLINES_LISTENERPRIO
#  define CONFIG_EXAMPLES_NXLINES_LISTENERPRIO 100
#endif
#ifndef CONFIG_EXAMPLES_NXLINES_CLIENTPRIO
#  define CONFIG_EXAMPLES_NXLINES_CLIENTPRIO 100
#endif
#ifndef CONFIG_EXAMPLES_NXLINES_SERVERPRIO
#  define CONFIG_EXAMPLES_NXLINES_SERVERPRIO 120
#endif
#ifndef CONFIG_EXAMPLES_NXLINES_NOTIFYSIGNO
#  define CONFIG_EXAMPLES_NXLINES_NOTIFYSIGNO 4
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum exitcode_e
{
  NXEXIT_SUCCESS = 0,
  NXEXIT_INIT,
  NXEXIT_NXREQUESTBKGD,
  NXEXIT_NXSETBGCOLOR
};

struct nxlines_data_s
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

/* NXLINES state data */

extern struct nxlines_data_s g_nxlines;

/* NX callback vtables */

extern const struct nx_callback_s g_nxlinescb;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* NX event listener */

FAR void *nxlines_listener(FAR void *arg);

/* Background window interfaces */

void nxlines_test(NXWINDOW hwnd);

#endif /* __APPS_EXAMPLES_NXLINES_NXLINES_H */
