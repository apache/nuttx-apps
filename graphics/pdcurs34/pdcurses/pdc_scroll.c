/****************************************************************************
 * apps/graphics/pdcurses/pdc_scroll.c
 * Public Domain Curses
 * RCSID("$Id: scroll.c,v 1.36 2008/07/13 16:08:18 wmcbrine Exp $")
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
 *   Adapted by: Gregory Nutt <gnutt@nuttx.org>
 *
 * Adapted from the original public domain pdcurses by Gregory Nutt and
 * released as part of NuttX under the 3-clause BSD license:
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

/* Name: scroll
 *
 * Synopsis:
 *       int scroll(WINDOW *win);
 *       int scrl(int n);
 *       int wscrl(WINDOW *win, int n);
 *
 * Description:
 *       scroll() causes the window to scroll up one line.  This involves
 *       moving the lines in the window data structure.
 *
 *       With a positive n, scrl() and wscrl() scroll the window up n
 *       lines (line i + n becomes i); otherwise they scroll the window
 *       down n lines.

 *       For these functions to work, scrolling must be enabled via
 *       scrollok(). Note also that scrolling is not allowed if the
 *       supplied window is a pad.
 *
 * Return Value:
 *       All functions return OK on success and ERR on error.
 *
 * Portability                                X/Open    BSD    SYS V
 *       scroll                                  Y       Y       Y
 *       scrl                                    Y       -      4.0
 *       wscrl                                   Y       -      4.0
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "curspriv.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int wscrl(WINDOW *win, int n)
{
  chtype blank;
  chtype *temp;
  int dir;
  int start;
  int end;
  int l;
  int i;

  /* Check if window scrolls. Valid for window AND pad */

  if (!win || !win->_scroll || !n)
    {
      return ERR;
    }

  blank = win->_bkgd;

  if (n > 0)
    {
      start = win->_tmarg;
      end = win->_bmarg;
      dir = 1;
    }
  else
    {
      start = win->_bmarg;
      end = win->_tmarg;
      dir = -1;
    }

  for (l = 0; l < (n * dir); l++)
    {
      temp = win->_y[start];

      /* Re-arrange line pointers */

      for (i = start; i != end; i += dir)
        {
          win->_y[i] = win->_y[i + dir];
        }

      win->_y[end] = temp;

      /* Make a blank line */

      for (i = 0; i < win->_maxx; i++)
        {
          *temp++ = blank;
        }
    }

  touchline(win, win->_tmarg, win->_bmarg - win->_tmarg + 1);

  PDC_sync(win);
  return OK;
}

int scrl(int n)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("scrl() - called\n"));

  return wscrl(stdscr, n);
}

int scroll(WINDOW *win)
{
  PDC_LOG(("scroll() - called\n"));

  return wscrl(win, 1);
}
