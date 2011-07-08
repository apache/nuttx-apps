/****************************************************************************
 * examples/nxtext/nxtext_bkgd.c
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

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void nxbg_redraw(NXWINDOW hwnd, FAR const struct nxgl_rect_s *rect,
                        bool morem, FAR void *arg);
static void nxbg_position(NXWINDOW hwnd, FAR const struct nxgl_size_s *size,
                          FAR const struct nxgl_point_s *pos,
                          FAR const struct nxgl_rect_s *bounds,
                          FAR void *arg);
#ifdef CONFIG_NX_MOUSE
static void nxbg_mousein(NXWINDOW hwnd, FAR const struct nxgl_point_s *pos,
                         uint8_t buttons, FAR void *arg);
#endif

#ifdef CONFIG_NX_KBD
static void nxbg_kbdin(NXWINDOW hwnd, uint8_t nch, FAR const uint8_t *ch,
                       FAR void *arg);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct nxtext_state_s  g_bgstate;
static struct nxtext_bitmap_s g_bgbm[CONFIG_EXAMPLES_NXTEXT_BMCACHE];
static struct nxtext_glyph_s  g_bgglyph[CONFIG_EXAMPLES_NXTEXT_GLCACHE];

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Background window call table */

const struct nx_callback_s g_bgcb =
{
  nxbg_redraw,   /* redraw */
  nxbg_position  /* position */
#ifdef CONFIG_NX_MOUSE
  , nxbg_mousein /* mousein */
#endif
#ifdef CONFIG_NX_KBD
  , nxbg_kbdin   /* my kbdin */
#endif
};

/* Background window handle */

NXHANDLE g_bgwnd;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxbg_fillwindow
 ****************************************************************************/

static inline void nxbg_fillwindow(NXWINDOW hwnd,
                                   FAR const struct nxgl_rect_s *rect,
                                   FAR struct nxtext_state_s *st)
{
  int ret;
  int i;

  ret = nx_fill(hwnd, rect, st->wcolor);
  if (ret < 0)
    {
      message("nxbg_fillwindow: nx_fill failed: %d\n", errno);
    }

  /* Fill each character on the display (Only the characters within rect
   * will actually be redrawn).
   */

#ifdef CONFIG_NX_KBD
  nxtext_home(st);
  for (i = 0; i < st->nchars; i++)
    {
      nxtext_fillchar(hwnd, rect, &st->bm[i]);
    }
#endif
}

/****************************************************************************
 * Name: nxbg_redraw
 ****************************************************************************/

static void nxbg_redraw(NXWINDOW hwnd, FAR const struct nxgl_rect_s *rect,
                        bool more, FAR void *arg)
{
  FAR struct nxtext_state_s *st = (FAR struct nxtext_state_s *)arg;
  message("nxbg_redraw: hwnd=%p rect={(%d,%d),(%d,%d)} more=%s\n",
          hwnd, rect->pt1.x, rect->pt1.y, rect->pt2.x, rect->pt2.y,
          more ? "true" : "false");

  nxbg_fillwindow(hwnd, rect, st);
}

/****************************************************************************
 * Name: nxbg_position
 ****************************************************************************/

static void nxbg_position(NXWINDOW hwnd, FAR const struct nxgl_size_s *size,
                          FAR const struct nxgl_point_s *pos,
                          FAR const struct nxgl_rect_s *bounds,
                          FAR void *arg)
{
  FAR struct nxtext_state_s *st = (FAR struct nxtext_state_s *)arg;

  /* Report the position */

  message("nxbg_position: hwnd=%p size=(%d,%d) pos=(%d,%d) bounds={(%d,%d),(%d,%d)}\n",
          hwnd, size->w, size->h, pos->x, pos->y,
          bounds->pt1.x, bounds->pt1.y, bounds->pt2.x, bounds->pt2.y);

  /* Have we picked off the window bounds yet? */

  if (!b_haveresolution)
    {
      /* Save the background window handle */

      g_bgwnd = hwnd;

      /* Save the background window size */

      st->wsize.w = size->w;
      st->wsize.h = size->h;

      /* Save the window limits (these should be the same for all places and all windows */

      g_xres = bounds->pt2.x;
      g_yres = bounds->pt2.y;

      b_haveresolution = true;
      sem_post(&g_semevent);
      message("nxbg_position: Have xres=%d yres=%d\n", g_xres, g_yres);
    }
}

/****************************************************************************
 * Name: nxbg_mousein
 ****************************************************************************/

#ifdef CONFIG_NX_MOUSE
static void nxbg_mousein(NXWINDOW hwnd, FAR const struct nxgl_point_s *pos,
                         uint8_t buttons, FAR void *arg)
{
  message("nxbg_mousein: hwnd=%p pos=(%d,%d) button=%02x\n",
          hwnd,  pos->x, pos->y, buttons);
}
#endif

/****************************************************************************
 * Name: nxbg_kbdin
 ****************************************************************************/

#ifdef CONFIG_NX_KBD
static void nxbg_kbdin(NXWINDOW hwnd, uint8_t nch, FAR const uint8_t *ch,
                       FAR void *arg)
{
  message("nxbg_kbdin: hwnd=%p nch=%d\n", hwnd, nch);
  nxbg_write(hwnd, ch, nch);
}
#endif

/****************************************************************************
 * Name: nxbg_scroll
 ****************************************************************************/

static void nxbg_scroll(NXWINDOW hwnd, int lineheight)
{
  int i;
  int j;

  /* Scroll up until the next display position is in the window.  We may need
   * do this more than once (unlikely)
   */

  while (g_bgstate.pos.y >= g_bgstate.wsize.h - lineheight)
    {
      /* Adjust the vertical position of each character */

      for (i = 0; i < g_bgstate.nchars; )
        {
          FAR struct nxtext_bitmap_s *bm = &g_bgstate.bm[i];

          /* Has any part of this character scrolled off the screen? */

          if (bm->bounds.pt1.y < lineheight)
            {
              /* Yes... Delete the character by moving all of the data */

              for (j = i; j < g_bgstate.nchars-1; j++)
                {
                  memcpy(&g_bgstate.bm[j], &g_bgstate.bm[j+1], sizeof(struct nxtext_bitmap_s));
                }

              /* Decrement the number of cached characters (i is not incremented
               * in this case because it already points to the next charactger)
               */

              g_bgstate.nchars--;
            }

          /* No.. just decrement its vertical position (moving it "up" the
           * display by one line.
           */

          else
            {
              bm->bounds.pt1.y -= lineheight;
              bm->bounds.pt2.y -= lineheight;

              /* Increment to the next character */
 
              i++;
            }
        }

      /* And move the next display position up by one line as well */

      g_bgstate.pos.y -= lineheight;
    }

  /* Then re-draw the entire display */

  nxbg_refresh(hwnd);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxbg_getstate
 *
 * Description:
 *   Initialize the background window state structure.
 *
 ****************************************************************************/

FAR struct nxtext_state_s *nxbg_getstate(void)
{
  FAR const struct nx_font_s *fontset;

  /* Initialize the color (used for redrawing the window) */

  memset(&g_bgstate, 0, sizeof(struct nxtext_state_s));
  g_bgstate.wcolor[0] = CONFIG_EXAMPLES_NXTEXT_BGCOLOR;
  g_bgstate.fcolor[0] = CONFIG_EXAMPLES_NXTEXT_BGFONTCOLOR;

  /* Get information about the font set being used and save this in the
   * state structure
   */

  fontset             = nxf_getfontset();
  g_bgstate.fheight   = fontset->mxheight;
  g_bgstate.fwidth    = fontset->mxwidth;
  g_bgstate.spwidth   = fontset->spwidth;

  /* Set up the text caches */

  g_bgstate.maxchars  = CONFIG_EXAMPLES_NXTEXT_BMCACHE;
  g_bgstate.maxglyphs = CONFIG_EXAMPLES_NXTEXT_GLCACHE;
  g_bgstate.bm        = g_bgbm;
  g_bgstate.glyph     = g_bgglyph;

  /* Set the first display position */

  nxtext_home(&g_bgstate);
  return &g_bgstate;
}

/****************************************************************************
 * Name: nxbg_write
 *
 * Description:
 *   Put a sequence of bytes in the window.
 *
 ****************************************************************************/

void nxbg_write(NXWINDOW hwnd, FAR const uint8_t *buffer, size_t buflen)
{
  int lineheight = (g_bgstate.fheight + 2);

  while (buflen-- > 0)
    {
      /* Will another character fit on this line? */

      if (g_bgstate.pos.x + g_bgstate.fwidth > g_bgstate.wsize.w)
        {
          /* No.. move to the next line */

          nxtext_newline(&g_bgstate);

          /* If we were about to output a newline character, then don't */

          if (*buffer == '\n')
            {
              buffer++;
              continue;
            }
        }

      /* Check if we need to scroll up */

      if (g_bgstate.pos.y >= g_bgstate.wsize.h - lineheight)
        {
          nxbg_scroll(hwnd, lineheight);
        }

      /* Finally, we can output the character */

      nxtext_putc(hwnd, &g_bgstate, (uint8_t)*buffer++);
    }
}

/****************************************************************************
 * Name: nxbg_refresh
 *
 * Description:
 *   Re-draw the entire background.
 *
 ****************************************************************************/

void nxbg_refresh(NXWINDOW hwnd)
{
  struct nxgl_rect_s rect;

  rect.pt1.x = 0;
  rect.pt1.y = 0;
  rect.pt2.x = g_bgstate.wsize.w - 1;
  rect.pt2.y = g_bgstate.wsize.h - 1;
  nxbg_fillwindow(hwnd, &rect, &g_bgstate);
}



