/****************************************************************************
 * examples/nxtext/nxtext_popup.c
 *
 *   Copyright (C) 2011 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <debug.h>
#include <errno.h>

#include <nuttx/nx.h>
#include <nuttx/nxfonts.h>

#include "nxtext_internal.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/

#define NBM_CACHE    8
#define NGLYPH_CACHE 8

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void nxpu_redraw(NXWINDOW hwnd, FAR const struct nxgl_rect_s *rect,
                        bool morem, FAR void *arg);
static void nxpu_position(NXWINDOW hwnd, FAR const struct nxgl_size_s *size,
                          FAR const struct nxgl_point_s *pos,
                          FAR const struct nxgl_rect_s *bounds,
                          FAR void *arg);
#ifdef CONFIG_NX_MOUSE
static void nxpu_mousein(NXWINDOW hwnd, FAR const struct nxgl_point_s *pos,
                         uint8_t buttons, FAR void *arg);
#endif

#ifdef CONFIG_NX_KBD
static void nxpu_kbdin(NXWINDOW hwnd, uint8_t nch, FAR const uint8_t *ch,
                       FAR void *arg);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct nx_callback_s g_pucb =
{
  nxpu_redraw,   /* redraw */
  nxpu_position  /* position */
#ifdef CONFIG_NX_MOUSE
  , nxpu_mousein /* mousein */
#endif
#ifdef CONFIG_NX_KBD
  , nxpu_kbdin   /* my kbdin */
#endif
};

static struct nxtext_state_s g_pustate;
#ifdef CONFIG_NX_KBD
static struct nxtext_bitmap_s  g_pubm[NBM_CACHE];
static struct nxtext_glyph_s g_puglyph[NGLYPH_CACHE];
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxpu_setsize
 ****************************************************************************/

static inline int nxpu_setsize(NXWINDOW hwnd, FAR struct nxgl_size_s *size)
{
  int ret = nx_setsize(hwnd, size);
  if (ret < 0)
    {
      message("user_start: nx_setsize failed: %d\n", errno);
      g_exitcode = NXEXIT_NXSETSIZE;
    }
  return ret;
}

/****************************************************************************
 * Name: nxpu_setposition
 ****************************************************************************/

static inline int nxpu_setposition(NXWINDOW hwnd, FAR struct nxgl_point_s *pos)
{
  int ret = nx_setposition(hwnd, pos);
  if (ret < 0)
    {
      message("user_start: nx_setposition failed: %d\n", errno);
      g_exitcode = NXEXIT_NXSETPOSITION;
    }
  return ret;
}

/****************************************************************************
 * Name: nxpu_fillwindow
 ****************************************************************************/

static inline void nxpu_fillwindow(NXWINDOW hwnd,
                                   FAR const struct nxgl_rect_s *rect,
                                   FAR struct nxtext_state_s *st)
{
  int ret;
  int i;

  ret = nx_fill(hwnd, rect, st->wcolor);
  if (ret < 0)
    {
      message("nxpu_fillwindow: nx_fill failed: %d\n", errno);
    }

  /* Fill each character on the display (Only the characters within rect
   * will actually be redrawn).
   */

  nxtext_home(st);
  for (i = 0; i < st->nchars; i++)
    {
      nxtext_fillchar(hwnd, rect, &st->bm[i]);
    }
}

/****************************************************************************
 * Name: nxpu_redraw
 ****************************************************************************/

static void nxpu_redraw(NXWINDOW hwnd, FAR const struct nxgl_rect_s *rect,
                        bool more, FAR void *arg)
{
  FAR struct nxtext_state_s *st = (FAR struct nxtext_state_s *)arg;
  message("nxpu_redraw: hwnd=%p rect={(%d,%d),(%d,%d)} more=%s\n",
          hwnd, rect->pt1.x, rect->pt1.y, rect->pt2.x, rect->pt2.y,
          more ? "true" : "false");

  nxpu_fillwindow(hwnd, rect, st);
}

/****************************************************************************
 * Name: nxpu_position
 ****************************************************************************/

static void nxpu_position(NXWINDOW hwnd, FAR const struct nxgl_size_s *size,
                          FAR const struct nxgl_point_s *pos,
                          FAR const struct nxgl_rect_s *bounds,
                          FAR void *arg)
{
  FAR struct nxtext_state_s *st = (FAR struct nxtext_state_s *)arg;

  /* Report the position */

  message("nxpu_position: hwnd=%p size=(%d,%d) pos=(%d,%d) bounds={(%d,%d),(%d,%d)}\n",
          hwnd, size->w, size->h, pos->x, pos->y,
          bounds->pt1.x, bounds->pt1.y, bounds->pt2.x, bounds->pt2.y);

  /* Save the window size */

  st->wsize.w = size->w;
  st->wsize.h = size->h;
}

/****************************************************************************
 * Name: nxpu_mousein
 ****************************************************************************/

#ifdef CONFIG_NX_MOUSE
static void nxpu_mousein(NXWINDOW hwnd, FAR const struct nxgl_point_s *pos,
                         uint8_t buttons, FAR void *arg)
{
  message("nxpu_mousein: hwnd=%p pos=(%d,%d) button=%02x\n",
          hwnd,  pos->x, pos->y, buttons);
}
#endif

/****************************************************************************
 * Name: nxpu_puts
 ****************************************************************************/

static inline void nxpu_puts(NXWINDOW hwnd, FAR struct nxtext_state_s *st,
                             uint8_t nch, FAR const uint8_t *ch)
{
  nxtext_home(st);
  while (nch--)
    {
      nxtext_putc(hwnd, st, *ch++);
    }
}

/****************************************************************************
 * Name: nxpu_kbdin
 ****************************************************************************/

#ifdef CONFIG_NX_KBD
static void nxpu_kbdin(NXWINDOW hwnd, uint8_t nch, FAR const uint8_t *ch,
                       FAR void *arg)
{
  FAR struct nxtext_state_s *st = (FAR struct nxtext_state_s *)arg;
  message("nxpu_kbdin: hwnd=%p nch=%d\n", hwnd, nch);
  nxpu_puts(hwnd, st, nch, ch);
}
#endif

/****************************************************************************
 * Name: nxpu_initstate
 ****************************************************************************/

static inline void nxpu_initstate(void)
{
#ifdef CONFIG_NX_KBD
  FAR const struct nx_font_s *fontset;
#endif

  /* Initialize the color (used for redrawing the window) */

  memset(&g_pustate, 0, sizeof(struct nxtext_state_s));
  g_pustate.wcolor[0] = CONFIG_EXAMPLES_NXTEXT_PUCOLOR;
  g_pustate.fcolor[0] = CONFIG_EXAMPLES_NXTEXT_PUFONTCOLOR;

  /* Get information about the font set being used and save this in the
   * state structure
   */

#ifdef CONFIG_NX_KBD
  fontset             = nxf_getfontset();
  g_pustate.fheight   = fontset->mxheight;
  g_pustate.fwidth    = fontset->mxwidth;
  g_pustate.spwidth   = fontset->spwidth;

  /* Set up the text caches */

  g_pustate.maxchars  = NBM_CACHE;
  g_pustate.maxglyphs = NGLYPH_CACHE;
  g_pustate.bm        = g_pubm;
  g_pustate.glyph     = g_puglyph;

  /* Set the first display position */

  nxtext_home(&g_pustate);
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxpu_open
 ****************************************************************************/

NXWINDOW nxpu_open(void)
{
  NXWINDOW hwnd;
  struct nxgl_size_s size;
  struct nxgl_point_s pt;
  int ret;

  /* Create a pop-up window */

  message("user_start: Create pop-up\n");
  nxpu_initstate();

  hwnd = nx_openwindow(g_hnx, &g_pucb, (FAR void *)&g_pustate);
  message("user_start: hwnd=%p\n", hwnd);

  if (!hwnd)
    {
      message("user_start: nx_openwindow failed: %d\n", errno);
      g_exitcode = NXEXIT_NXOPENWINDOW;
      goto errout_with_state;
    }

  /* Set the size of the pop-up window */

  size.w = g_xres / 4;
  size.h = g_yres / 4;

  message("user_start: Set pop-up size to (%d,%d)\n", size.w, size.h);
  ret = nxpu_setsize(hwnd, &size);
  if (ret < 0)
    {
      goto errout_with_hwnd;
    }

  /* Set a random position for pop-up */

  pt.x = ((uint32_t)(g_xres - size.w) * rand()) >> 16;
  pt.y = ((uint32_t)(g_yres - size.h) * rand()) >> 16;

  message("user_start: Set pop-up postion to (%d,%d)\n", pt.x, pt.y);
  ret = nxpu_setposition(hwnd, &pt);
  if (ret < 0)
    {
      goto errout_with_hwnd;
    }

  return hwnd;

errout_with_hwnd:
  (void)nx_closewindow(hwnd);

errout_with_state:
  return NULL;
}

/****************************************************************************
 * Name: nxpu_close
 ****************************************************************************/

int nxpu_close(NXWINDOW hwnd)
{
  int ret = nx_closewindow(hwnd);
  if (ret < 0)
    {
      message("nxpu_close: nx_closewindow failed: %d\n", errno);
      g_exitcode = NXEXIT_NXCLOSEWINDOW;
    }
  return ret;
}
