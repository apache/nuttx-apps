/****************************************************************************
 * apps/examples/nxtext/nxtext_internal.h
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

#ifndef __APPS_EXAMPLES_NXTEXT_NXTEXT_INTERNAL_H
#define __APPS_EXAMPLES_NXTEXT_NXTEXT_INTERNAL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <semaphore.h>

#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxfonts.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

#ifndef CONFIG_NX
#  error "NX is not enabled (CONFIG_NX)"
#endif

#ifndef CONFIG_EXAMPLES_NXTEXT_VPLANE
#    define CONFIG_EXAMPLES_NXTEXT_VPLANE 0
#endif

/* Line spacing.  Space (in rows) between lines. */

#ifdef CONFIG_EXAMPLES_NXTEXT_LINESPACING
#  define LINE_SEPARATION CONFIG_EXAMPLES_NXTEXT_LINESPACING
#else
#  define LINE_SEPARATION 2
#endif

/* Pixel depth.  If none provided, pick the smallest enabled pixel depth */

#ifndef CONFIG_EXAMPLES_NXTEXT_BPP
#  if !defined(CONFIG_NX_DISABLE_1BPP)
#    define CONFIG_EXAMPLES_NXTEXT_BPP 1
#  elif !defined(CONFIG_NX_DISABLE_2BPP)
#    define CONFIG_EXAMPLES_NXTEXT_BPP 2
#  elif !defined(CONFIG_NX_DISABLE_4BPP)
#    define CONFIG_EXAMPLES_NXTEXT_BPP 4
#  elif !defined(CONFIG_NX_DISABLE_8BPP)
#    define CONFIG_EXAMPLES_NXTEXT_BPP 8
#  elif !defined(CONFIG_NX_DISABLE_16BPP)
#    define CONFIG_EXAMPLES_NXTEXT_BPP 16
#  elif !defined(CONFIG_NX_DISABLE_24BPP)
#    define CONFIG_EXAMPLES_NXTEXT_BPP 24
#  elif !defined(CONFIG_NX_DISABLE_32BPP)
#    define CONFIG_EXAMPLES_NXTEXT_BPP 32
#  else
#    error "No pixel depth provided"
#  endif
#endif

/* Background color */

#ifndef CONFIG_EXAMPLES_NXTEXT_BGCOLOR
#  if CONFIG_EXAMPLES_NXTEXT_BPP == 24 || CONFIG_EXAMPLES_NXTEXT_BPP == 32
#    define CONFIG_EXAMPLES_NXTEXT_BGCOLOR 0x007b68ee
#  elif CONFIG_EXAMPLES_NXTEXT_BPP == 16
#    define CONFIG_EXAMPLES_NXTEXT_BGCOLOR 0x7b5d
#  else
#    define CONFIG_EXAMPLES_NXTEXT_BGCOLOR ' '
# endif
#endif

/* Pop-up font ID */

#ifndef CONFIG_EXAMPLES_NXTEXT_PUFONTID
#  define CONFIG_EXAMPLES_NXTEXT_PUFONTID NXFONT_DEFAULT
#endif

/* Pop-up window color */

#ifndef CONFIG_EXAMPLES_NXTEXT_PUCOLOR
#  if CONFIG_EXAMPLES_NXTEXT_BPP == 24 || CONFIG_EXAMPLES_NXTEXT_BPP == 32
#    define CONFIG_EXAMPLES_NXTEXT_PUCOLOR 0x00dcdcdc
#  elif CONFIG_EXAMPLES_NXTEXT_BPP == 16
#    define CONFIG_EXAMPLES_NXTEXT_PUCOLOR 0xdefb
#  else
#    define CONFIG_EXAMPLES_NXTEXT_PUCOLOR '2'
# endif
#endif

/* Background font ID */

#ifndef CONFIG_EXAMPLES_NXTEXT_BGFONTID
#  define CONFIG_EXAMPLES_NXTEXT_BGFONTID NXFONT_DEFAULT
#endif

/* Background font color */

#ifndef CONFIG_EXAMPLES_NXTEXT_BGFONTCOLOR
#  if CONFIG_EXAMPLES_NXTEXT_BPP == 24 || CONFIG_EXAMPLES_NXTEXT_BPP == 32
#    define CONFIG_EXAMPLES_NXTEXT_BGFONTCOLOR 0x00000000
#  elif CONFIG_EXAMPLES_NXTEXT_BPP == 16
#    define CONFIG_EXAMPLES_NXTEXT_BGFONTCOLOR 0x0000
#  else
#    define CONFIG_EXAMPLES_NXTEXT_BGFONTCOLOR 'F'
#  endif
#endif

/* Pop-up font color */

#ifndef CONFIG_EXAMPLES_NXTEXT_PUFONTCOLOR
#  if CONFIG_EXAMPLES_NXTEXT_BPP == 24 || CONFIG_EXAMPLES_NXTEXT_BPP == 32
#    define CONFIG_EXAMPLES_NXTEXT_PUFONTCOLOR 0x00000000
#  elif CONFIG_EXAMPLES_NXTEXT_BPP == 16
#    define CONFIG_EXAMPLES_NXTEXT_PUFONTCOLOR 0x0000
#  else
#    define CONFIG_EXAMPLES_NXTEXT_PUFONTCOLOR 'F'
#  endif
#endif

/* Character caching */

#ifndef CONFIG_EXAMPLES_NXTEXT_BMCACHE
#  define CONFIG_EXAMPLES_NXTEXT_BMCACHE 128
#endif

/* Font glyph caching */

#ifndef CONFIG_EXAMPLES_NXTEXT_GLCACHE
#  define CONFIG_EXAMPLES_NXTEXT_GLCACHE 16
#endif

/* NX Server Configuration */

#ifdef CONFIG_DISABLE_MQUEUE
#  error "The multi-threaded example requires MQ support (CONFIG_DISABLE_MQUEUE=n)"
#endif
#ifdef CONFIG_DISABLE_PTHREAD
#  error "This example requires pthread support (CONFIG_DISABLE_PTHREAD=n)"
#endif
#ifndef CONFIG_NX_BLOCKING
#  error "This example depends on CONFIG_NX_BLOCKING"
#endif
#ifndef CONFIG_EXAMPLES_NXTEXT_STACKSIZE
#  define CONFIG_EXAMPLES_NXTEXT_STACKSIZE 2048
#endif
#ifndef CONFIG_EXAMPLES_NXTEXT_LISTENERPRIO
#  define CONFIG_EXAMPLES_NXTEXT_LISTENERPRIO 100
#endif
#ifndef CONFIG_EXAMPLES_NXTEXT_CLIENTPRIO
#  define CONFIG_EXAMPLES_NXTEXT_CLIENTPRIO 100
#endif

/* Bitmap flags */

#define BMFLAGS_NOGLYPH (1 << 0) /* No glyph available, use space */

#define BM_ISSPACE(bm)  (((bm)->flags & BMFLAGS_NOGLYPH) != 0)

/* Sizes and maximums */

#define MAX_USECNT      255  /* Limit to range of a uint8_t */

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum exitcode_e
{
  NXEXIT_SUCCESS = 0,
  NXEXIT_SCHEDSETPARAM,
  NXEXIT_SERVERSTART,
  NXEXIT_PTHREADCREATE,
  NXEXIT_EXTINITIALIZE,
  NXEXIT_FBINITIALIZE,
  NXEXIT_FBGETVPLANE,
  NXEXIT_LCDINITIALIZE,
  NXEXIT_LCDGETDEV,
  NXEXIT_NXOPEN,
  NXEXIT_FONTOPEN,
  NXEXIT_NXREQUESTBKGD,
  NXEXIT_NXCONNECT,
  NXEXIT_NXSETBGCOLOR,
  NXEXIT_NXOPENWINDOW,
  NXEXIT_NXSETSIZE,
  NXEXIT_NXSETPOSITION,
  NXEXIT_NXCLOSEWINDOW,
  NXEXIT_LOSTSERVERCONN
};

/* Describes one cached glyph bitmap */

struct nxtext_glyph_s
{
  uint8_t code;                        /* Character code */
  uint8_t height;                      /* Height of this glyph (in rows) */
  uint8_t width;                       /* Width of this glyph (in pixels) */
  uint8_t stride;                      /* Width of the glyph row (in bytes) */
  uint8_t usecnt;                      /* Use count */
  FAR uint8_t *bitmap;                 /* Allocated bitmap memory */
};

/* Describes on character on the display */

struct nxtext_bitmap_s
{
  uint8_t code;                        /* Character code */
  uint8_t flags;                       /* See BMFLAGS_* */
  struct nxgl_point_s pos;             /* Character position */
};

/* Describes the state of one text display */

struct nxtext_state_s
{
  /* The following describe the window */

  nxgl_mxpixel_t wcolor[CONFIG_NX_NPLANES]; /* Window color */
  struct nxgl_size_s wsize;                 /* Window size */
  struct nxgl_point_s wpos;                 /* Window position */

  /* These characterize the font in use */

  nxgl_mxpixel_t fcolor[CONFIG_NX_NPLANES]; /* Font color */
  uint8_t fheight;                          /* Max height of a font in pixels */
  uint8_t fwidth;                           /* Max width of a font in pixels */
  uint8_t spwidth;                          /* The width of a space */
  struct nxgl_point_s fpos;                 /* Next display position */

  /* These describe all text already added to the display */

  uint8_t maxglyphs;                        /* Size of the glyph[] array */
  uint16_t maxchars;                        /* Size of the bm[] array */
  uint16_t nchars;                          /* Number of chars in the bm[] array */

  FAR struct nxtext_bitmap_s *bm;           /* List of characters on the display */
  FAR struct nxtext_glyph_s  *glyph;        /* Cache of rendered fonts in use */
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* The connection handler */

extern NXHANDLE g_hnx;

/* Background window handle */

extern NXHANDLE g_bgwnd;

/* The font handlse */

extern NXHANDLE g_bghfont;
extern NXHANDLE g_puhfont;

/* NX callback vtables */

extern const struct nx_callback_s g_nxtextcb;

/* The screen resolution */

extern nxgl_coord_t g_xres;
extern nxgl_coord_t g_yres;

extern bool b_haveresolution;
extern bool g_connected;
extern sem_t g_semevent;

extern int g_exitcode;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

FAR void *nxtext_listener(FAR void *arg);

/* Background window interfaces */

FAR struct nxtext_state_s *nxbg_getstate(void);
void nxbg_write(NXWINDOW hwnd, FAR const uint8_t *buffer, size_t buflen);

/* Pop-up window interfaces */

NXWINDOW nxpu_open(void);
int nxpu_close(NXWINDOW hwnd);

/* Generic text helpers */

void nxtext_home(FAR struct nxtext_state_s *st);
void nxtext_newline(FAR struct nxtext_state_s *st);
void nxtext_putc(NXWINDOW hwnd, FAR struct nxtext_state_s *st,
                 NXHANDLE hfont, uint8_t ch);
void nxtext_fillchar(NXWINDOW hwnd, FAR const struct nxgl_rect_s *rect,
                     FAR struct nxtext_state_s *st, NXHANDLE hfont,
                     FAR const struct nxtext_bitmap_s *bm);

#endif /* __APPS_EXAMPLES_NXTEXT_NXTEXT_INTERNAL_H */
