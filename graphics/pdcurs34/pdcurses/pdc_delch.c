/****************************************************************************
 * apps/graphics/pdcurses/pdc_delch.c
 * Public Domain Curses
 * RCSID("$Id: delch.c,v 1.33 2008/07/13 16:08:18 wmcbrine Exp $")
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

/* Name: delch
 *
 * Synopsis:
 *       int delch(void);
 *       int wdelch(WINDOW *win);
 *       int mvdelch(int y, int x);
 *       int mvwdelch(WINDOW *win, int y, int x);
 *
 * Description:
 *       The character under the cursor in the window is deleted.  All
 *       characters to the right on the same line are moved to the left
 *       one position and the last character on the line is filled with
 *       a blank.  The cursor position does not change (after moving to
 *       y, x if coordinates are specified).
 *
 * Return Value:
 *       All functions return OK on success and ERR on error.
 *
 * Portability                                X/Open    BSD    SYS V
 *       delch                                   Y       Y       Y
 *       wdelch                                  Y       Y       Y
 *       mvdelch                                 Y       Y       Y
 *       mvwdelch                                Y       Y       Y
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <string.h>

#include "curspriv.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int wdelch(WINDOW *win)
{
  int x;
  int y;
  int maxx;
  chtype *temp1;

  PDC_LOG(("wdelch() - called\n"));

  if (!win)
    {
      return ERR;
    }

  y     = win->_cury;
  x     = win->_curx;
  maxx  = win->_maxx - 1;
  temp1 = &win->_y[y][x];

  memmove(temp1, temp1 + 1, (maxx - x) * sizeof(chtype));

  /* wrs (4/10/93) account for window background */

  win->_y[y][maxx] = win->_bkgd;

  win->_lastch[y] = maxx;

  if ((win->_firstch[y] == _NO_CHANGE) || (win->_firstch[y] > x))
    {
      win->_firstch[y] = x;
    }

  PDC_sync(win);

  return OK;
}

int delch(void)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("delch() - called\n"));

  return wdelch(stdscr);
}

int mvdelch(int y, int x)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("mvdelch() - called\n"));

  if (move(y, x) == ERR)
    {
      return ERR;
    }

  return wdelch(stdscr);
}

int mvwdelch(WINDOW *win, int y, int x)
{
  PDC_LOG(("mvwdelch() - called\n"));

  if (wmove(win, y, x) == ERR)
    {
      return ERR;
    }

  return wdelch(win);
}
