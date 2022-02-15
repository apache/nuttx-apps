/****************************************************************************
 * apps/examples/nx/nx_internal.h
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

#ifndef __APPS_EXAMPLES_NX_NX_INTERNAL_H
#define __APPS_EXAMPLES_NX_NX_INTERNAL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <semaphore.h>

#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxtk.h>
#include <nuttx/nx/nxfonts.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

#ifndef CONFIG_NX
#  error "NX is not enabled (CONFIG_NX)"
#endif

#ifndef CONFIG_EXAMPLES_NX_VPLANE
#    define CONFIG_EXAMPLES_NX_VPLANE 0
#endif

#ifndef CONFIG_EXAMPLES_NX_BPP
#  define CONFIG_EXAMPLES_NX_BPP 32
#endif

#ifndef CONFIG_EXAMPLES_NX_BGCOLOR
#  if CONFIG_EXAMPLES_NX_BPP == 24 || CONFIG_EXAMPLES_NX_BPP == 32
#    define CONFIG_EXAMPLES_NX_BGCOLOR 0x007b68ee
#  elif CONFIG_EXAMPLES_NX_BPP == 16
#    define CONFIG_EXAMPLES_NX_BGCOLOR 0x7b5d
#  else
#    define CONFIG_EXAMPLES_NX_BGCOLOR ' '
# endif
#endif

#ifndef CONFIG_EXAMPLES_NX_COLOR1
#  if CONFIG_EXAMPLES_NX_BPP == 24 || CONFIG_EXAMPLES_NX_BPP == 32
#    define CONFIG_EXAMPLES_NX_COLOR1 0x00e6e6fa
#  elif CONFIG_EXAMPLES_NX_BPP == 16
#    define CONFIG_EXAMPLES_NX_COLOR1 0xe73f
#  else
#    define CONFIG_EXAMPLES_NX_COLOR1 '1'
# endif
#endif

#ifndef CONFIG_EXAMPLES_NX_COLOR2
#  if CONFIG_EXAMPLES_NX_BPP == 24 || CONFIG_EXAMPLES_NX_BPP == 32
#    define CONFIG_EXAMPLES_NX_COLOR2 0x00dcdcdc
#  elif CONFIG_EXAMPLES_NX_BPP == 16
#    define CONFIG_EXAMPLES_NX_COLOR2 0xdefb
#  else
#    define CONFIG_EXAMPLES_NX_COLOR2 '2'
# endif
#endif

#ifndef CONFIG_EXAMPLES_NX_TBCOLOR
#  if CONFIG_EXAMPLES_NX_BPP == 24 || CONFIG_EXAMPLES_NX_BPP == 32
#    define CONFIG_EXAMPLES_NX_TBCOLOR 0x00a9a9a9
#  elif CONFIG_EXAMPLES_NX_BPP == 16
#    define CONFIG_EXAMPLES_NX_TBCOLOR 0xad55
#  else
#    define CONFIG_EXAMPLES_NX_TBCOLOR 'T'
#  endif
#endif

#ifndef CONFIG_EXAMPLES_NX_FONTID
#  define CONFIG_EXAMPLES_NX_FONTID NXFONT_DEFAULT
#endif

#ifndef CONFIG_EXAMPLES_NX_FONTCOLOR
#  if CONFIG_EXAMPLES_NX_BPP == 24 || CONFIG_EXAMPLES_NX_BPP == 32
#    define CONFIG_EXAMPLES_NX_FONTCOLOR 0x00000000
#  elif CONFIG_EXAMPLES_NX_BPP == 16
#    define CONFIG_EXAMPLES_NX_FONTCOLOR 0x0000
#  else
#    define CONFIG_EXAMPLES_NX_FONTCOLOR 'F'
#  endif
#endif

#ifndef CONFIG_EXAMPLES_NX_TOOLBAR_HEIGHT
#  define CONFIG_EXAMPLES_NX_TOOLBAR_HEIGHT 16
#endif

/* NX Server Options */

#ifdef CONFIG_DISABLE_MQUEUE
#  error "The multi-threaded example requires MQ support (CONFIG_DISABLE_MQUEUE=n)"
#endif
#ifdef CONFIG_DISABLE_PTHREAD
#  error "This example requires pthread support (CONFIG_DISABLE_PTHREAD=n)"
#endif
#ifndef CONFIG_NX_BLOCKING
#  error "This example depends on CONFIG_NX_BLOCKING"
#endif
#ifndef CONFIG_EXAMPLES_NX_STACKSIZE
#  define CONFIG_EXAMPLES_NX_STACKSIZE 2048
#endif
#ifndef CONFIG_EXAMPLES_NX_LISTENERPRIO
#  define CONFIG_EXAMPLES_NX_LISTENERPRIO 100
#endif
#ifndef CONFIG_EXAMPLES_NX_CLIENTPRIO
#  define CONFIG_EXAMPLES_NX_CLIENTPRIO 100
#endif
#ifndef CONFIG_EXAMPLES_NX_SERVERPRIO
#  define CONFIG_EXAMPLES_NX_SERVERPRIO 120
#endif
#ifndef CONFIG_EXAMPLES_NX_NOTIFYSIGNO
#  define CONFIG_EXAMPLES_NX_NOTIFYSIGNO 4
#endif

#ifdef CONFIG_EXAMPLES_NX_RAWWINDOWS
#  define NXEGWINDOW NXWINDOW
#else
#  define NXEGWINDOW NXTKWINDOW
#endif

#define NXTK_MAXKBDCHARS 16

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum exitcode_e
{
  NXEXIT_SUCCESS = 0,
  NXEXIT_SIGPROCMASK,
  NXEXIT_SCHEDSETPARAM,
  NXEXIT_EVENTNOTIFY,
  NXEXIT_TASKCREATE,
  NXEXIT_PTHREADCREATE,
  NXEXIT_EXTINITIALIZE,
  NXEXIT_FBINITIALIZE,
  NXEXIT_FBGETVPLANE,
  NXEXIT_LCDINITIALIZE,
  NXEXIT_LCDGETDEV,
  NXEXIT_NXOPEN,
  NXEXIT_FONTOPEN,
  NXEXIT_NXOPENTOOLBAR,
  NXEXIT_NXCONNECT,
  NXEXIT_NXSETBGCOLOR,
  NXEXIT_NXOPENWINDOW,
  NXEXIT_NXSETSIZE,
  NXEXIT_NXSETPOSITION,
  NXEXIT_NXLOWER,
  NXEXIT_NXRAISE,
  NXEXIT_NXCLOSEWINDOW,
  NXEXIT_LOSTSERVERCONN
};

/* Describes one cached glyph bitmap */

struct nxeg_glyph_s
{
  uint8_t code;                        /* Character code */
  uint8_t height;                      /* Height of this glyph (in rows) */
  uint8_t width;                       /* Width of this glyph (in pixels) */
  uint8_t stride;                      /* Width of the glyph row (in bytes) */
  FAR uint8_t *bitmap;                 /* Allocated bitmap memory */
};

/* Describes on character on the display */

struct nxeg_bitmap_s
{
  struct nxgl_rect_s bounds;            /* Size/position of bitmap */
  FAR const struct nxeg_glyph_s *glyph; /* The cached glyph */
};

/* Describes the overall state of on one window */

struct nxeg_state_s
{
  uint8_t wnum;                            /* Window number */
  nxgl_mxpixel_t color[CONFIG_NX_NPLANES]; /* Window color */

#ifdef CONFIG_NX_KBD
  uint8_t height;                          /* Max height of a font in pixels */
  uint8_t width;                           /* Max width of a font in pixels */
  uint8_t spwidth;                         /* The width of a space */

  uint8_t nchars;                          /* Number of KBD chars received */
  uint8_t nglyphs;                         /* Number of glyphs cached */

  struct nxeg_bitmap_s bm[NXTK_MAXKBDCHARS];
  struct nxeg_glyph_s  glyph[NXTK_MAXKBDCHARS];
#endif
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* The connection handle */

extern NXHANDLE g_hnx;

/* NX callback vtables */

extern const struct nx_callback_s g_nxcb;
#ifndef CONFIG_EXAMPLES_NX_RAWWINDOWS
extern const struct nx_callback_s g_tbcb;
#endif

/* The font handle */

extern NXHANDLE g_fonthandle;

/* The screen resolution */

extern nxgl_coord_t g_xres;
extern nxgl_coord_t g_yres;

extern bool b_haveresolution;
extern bool g_connected;
extern sem_t g_semevent;

/* Colors used to fill window 1 & 2 */

extern nxgl_mxpixel_t g_color1[CONFIG_NX_NPLANES];
extern nxgl_mxpixel_t g_color2[CONFIG_NX_NPLANES];
#ifndef CONFIG_EXAMPLES_NX_RAWWINDOWS
extern nxgl_mxpixel_t g_tbcolor[CONFIG_NX_NPLANES];
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

FAR void *nx_listenerthread(FAR void *arg);

#ifdef CONFIG_NX_KBD
void nxeg_kbdin(NXWINDOW hwnd, uint8_t nch, const uint8_t *ch,
                FAR void *arg);
#ifndef CONFIG_EXAMPLES_NX_RAWWINDOWS
void nxeg_tbkbdin(NXWINDOW hwnd, uint8_t nch, const uint8_t *ch,
                  FAR void *arg);
#endif
void nxeg_filltext(NXWINDOW hwnd, FAR const struct nxgl_rect_s *rect,
                   FAR struct nxeg_state_s *st);
#endif

#endif /* __APPS_EXAMPLES_NX_NX_INTERNAL_H */
