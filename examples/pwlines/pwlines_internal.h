/****************************************************************************
 * apps/examples/pwlines/pwlines_internal.h
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

#ifndef __APPS_EXAMPLES_PWLINES_PWLINES_INTERNAL_H
#define __APPS_EXAMPLES_PWLINES_PWLINES_INTERNAL_H

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
#include <nuttx/video/rgbcolors.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

/* Required NX Server Settings */

#ifndef CONFIG_NX
#  error "NX is not enabled (CONFIG_NX)"
#endif

#ifdef CONFIG_DISABLE_MQUEUE
#  error "The multi-threaded example requires MQ support (CONFIG_DISABLE_MQUEUE=n)"
#endif

#ifdef CONFIG_DISABLE_PTHREAD
#  error "This example requires pthread support (CONFIG_DISABLE_PTHREAD=n)"
#endif

#ifndef CONFIG_NX_BLOCKING
#  error "This example depends on CONFIG_NX_BLOCKING"
#endif

/* Task priorities */

#if CONFIG_EXAMPLES_PWLINES_CLIENT_PRIO >= CONFIG_EXAMPLES_PWLINES_LISTENER_PRIO || \
    CONFIG_EXAMPLES_PWLINES_CLIENT_PRIO >= CONFIG_NXSTART_SERVERPRIO
#  warning Client priority must be lower than both the listener and server priorities
#endif

#if CONFIG_EXAMPLES_PWLINES_LISTENER_PRIO >= CONFIG_NXSTART_SERVERPRIO
#  warning Listener priority must be lower than the server priority
#endif

/* Default colors */

#ifndef CONFIG_EXAMPLES_PWLINES_BGCOLOR
#  if CONFIG_EXAMPLES_PWLINES_BPP == 24 || CONFIG_EXAMPLES_PWLINES_BPP == 32
#    define CONFIG_EXAMPLES_PWLINES_BGCOLOR 0x007b68ee
#  elif CONFIG_EXAMPLES_PWLINES_BPP == 16
#    define CONFIG_EXAMPLES_PWLINES_BGCOLOR 0x7b5d
#  else
#    define CONFIG_EXAMPLES_PWLINES_BGCOLOR ' '
# endif
#endif

#ifndef CONFIG_EXAMPLES_PWLINES_COLOR1
#  if CONFIG_EXAMPLES_PWLINES_BPP == 24 || CONFIG_EXAMPLES_PWLINES_BPP == 32
#    define CONFIG_EXAMPLES_PWLINES_COLOR1 RGB24_DARKGREEN
#  elif CONFIG_EXAMPLES_PWLINES_BPP == 16
#    define CONFIG_EXAMPLES_PWLINES_COLOR1 RGB16_DARKGREEN
#  else
#    define CONFIG_EXAMPLES_PWLINES_COLOR1 RGB8_DARKGREEN
# endif
#endif

#ifndef CONFIG_EXAMPLES_PWLINES_COLOR2
#  if CONFIG_EXAMPLES_PWLINES_BPP == 24 || CONFIG_EXAMPLES_PWLINES_BPP == 32
#    define CONFIG_EXAMPLES_PWLINES_COLOR2 RGB24_GREEN
#  elif CONFIG_EXAMPLES_PWLINES_BPP == 16
#    define CONFIG_EXAMPLES_PWLINES_COLOR2 RGB16_GREEN
#  else
#    define CONFIG_EXAMPLES_PWLINES_COLOR2 RGB8_GREEN
# endif
#endif

#ifndef CONFIG_EXAMPLES_PWLINES_COLOR3
#  if CONFIG_EXAMPLES_PWLINES_BPP == 24 || CONFIG_EXAMPLES_PWLINES_BPP == 32
#    define CONFIG_EXAMPLES_PWLINES_COLOR3 RGB24_LIGHTGREEN
#  elif CONFIG_EXAMPLES_PWLINES_BPP == 16
#    define CONFIG_EXAMPLES_PWLINES_COLOR3 RGB16_LIGHTGREEN
#  else
#    define CONFIG_EXAMPLES_PWLINES_COLOR3 RGB8_LIGHTGREEN
# endif
#endif

#ifndef CONFIG_EXAMPLES_PWLINES_BORDERWIDTH
#  define CONFIG_EXAMPLES_PWLINES_BORDERWIDTH 16
#endif

#ifndef CONFIG_EXAMPLES_PWLINES_BORDERCOLOR
#  if CONFIG_EXAMPLES_PWLINES_BPP == 24 || CONFIG_EXAMPLES_PWLINES_BPP == 32
#    define CONFIG_EXAMPLES_PWLINES_BORDERCOLOR RGB24_YELLOW
#  elif CONFIG_EXAMPLES_PWLINES_BPP == 16
#    define CONFIG_EXAMPLES_PWLINES_BORDERCOLOR RGB16_YELLOW
#  else
#    define CONFIG_EXAMPLES_PWLINES_BORDERCOLOR RGB8_YELLOW
#  endif
#endif

#ifndef CONFIG_EXAMPLES_PWLINES_FACECOLOR
#  if CONFIG_EXAMPLES_PWLINES_BPP == 24 || CONFIG_EXAMPLES_PWLINES_BPP == 32
#    define CONFIG_EXAMPLES_PWLINES_FACECOLOR RGB24_BEIGE
#  elif CONFIG_EXAMPLES_PWLINES_BPP == 16
#    define CONFIG_EXAMPLES_PWLINES_FACECOLOR RGB16_BEIGE
#  else
#    define CONFIG_EXAMPLES_PWLINES_FACECOLOR RGB8_BEIGE
#  endif
#endif

#ifndef CONFIG_EXAMPLES_PWLINES_LINEWIDTH
#  define CONFIG_EXAMPLES_PWLINES_LINEWIDTH 16
#endif

#ifndef CONFIG_EXAMPLES_PWLINES_LINECOLOR
#  if CONFIG_EXAMPLES_PWLINES_BPP == 24 || CONFIG_EXAMPLES_PWLINES_BPP == 32
#    define CONFIG_EXAMPLES_PWLINES_LINECOLOR RGB24_GOLD
#  elif CONFIG_EXAMPLES_PWLINES_BPP == 16
#    define CONFIG_EXAMPLES_PWLINES_LINECOLOR RGB16_GOLD
#  else
#    define CONFIG_EXAMPLES_PWLINES_LINECOLOR RGB8_GOLD
#  endif
#endif

/* Helpers */

#ifndef MIN
#  define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#  define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Describes the unique state of one window */

struct pwlines_window_s
{
  NXTKWINDOW hwnd;                         /* Window handle */
  nxgl_mxpixel_t color[CONFIG_NX_NPLANES]; /* Window color */
  struct nxgl_size_s size;                 /* Drawable window size */
  struct nxgl_point_s center;              /* Circle center position */
  struct nxgl_vector_s previous;           /* Previous line vector */
  nxgl_coord_t radius;                     /* Internal, drawable radius */
  b16_t angle;                             /* Current line angle */
  b16_t xmax;                              /* Max X position */
  b16_t ymax;                              /* Max Y position */
  b16_t xpos;                              /* Current X position */
  b16_t ypos;                              /* Current Y position */
  b16_t deltax;                            /* Current X speed */
  b16_t deltay;                            /* Current Y speed */
};

/* Describes the overall state of the example */

struct pwlines_state_s
{
  /* NX server */

  volatile bool haveres;                   /* True:  Have screen resolution */
  volatile bool connected;                 /* True:  Connected to server */
  sem_t semevent;                          /* Event wait semaphore */
  NXHANDLE hnx;                            /* Connection handle */

  /* Graphics hardware */

  nxgl_coord_t xres;                       /* Horizontal resolution */
  nxgl_coord_t yres;                       /* Vertical resolution */

  /* Image properties */

  nxgl_mxpixel_t facecolor[CONFIG_NX_NPLANES];   /* Color of circle center region */
  nxgl_mxpixel_t bordercolor[CONFIG_NX_NPLANES]; /* Color of circle border */
  nxgl_mxpixel_t linecolor[CONFIG_NX_NPLANES];   /* Color of rotating line */

  /* Window-specific state */

  struct pwlines_window_s wndo[3];
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* NX callback vtables */

extern const struct nx_callback_s g_pwlines_wncb;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

FAR void *pwlines_listener(FAR void *arg);
void pwlines_circle(FAR struct pwlines_state_s *st);
void pwlines_update(FAR struct pwlines_state_s *st);
bool pwlines_motion(FAR struct pwlines_state_s *st);

#endif /* __APPS_EXAMPLES_PWLINES_PWLINES_INTERNAL_H */
