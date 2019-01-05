/****************************************************************************
 * apps/graphics/pdcurses/pdc_move.c
 * Public Domain Curses
 * RCSID("$Id: move.c,v 1.28 2008/07/13 16:08:18 wmcbrine Exp $")
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

/* Name: move
 *
 * Synopsis:
 *       int move(int y, int x);
 *       int wmove(WINDOW *win, int y, int x);
 *
 * Description:
 *       The cursor associated with the window is moved to the given
 *       location.  This does not move the physical cursor of the
 *       terminal until refresh() is called.  The position specified is
 *       relative to the upper left corner of the window, which is (0,0).
 *
 * Return Value:
 *       All functions return OK on success and ERR on error.
 *
 * Portability                                X/Open    BSD    SYS V
 *       move                                    Y       Y       Y
 *       wmove                                   Y       Y       Y
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "curspriv.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int move(int y, int x)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("move() - called: y=%d x=%d\n", y, x));

  if (!stdscr || x < 0 || y < 0 || x >= stdscr->_maxx || y >= stdscr->_maxy)
    {
      return ERR;
    }

  stdscr->_curx = x;
  stdscr->_cury = y;
  return OK;
}

int wmove(WINDOW *win, int y, int x)
{
  PDC_LOG(("wmove() - called: y=%d x=%d\n", y, x));

  if (!win || x < 0 || y < 0 || x >= win->_maxx || y >= win->_maxy)
    {
      return ERR;
    }

  win->_curx = x;
  win->_cury = y;
  return OK;
}
