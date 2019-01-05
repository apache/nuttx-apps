/****************************************************************************
 * apps/graphics/pdcurses/pdc_getyx.c
 * Public Domain Curses
 * RCSID("$Id: getyx.c,v 1.29 2008/07/15 17:13:26 wmcbrine Exp $")
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

/* Name: getyx
 *
 * Synopsis:
 *       void getyx(WINDOW *win, int y, int x);
 *       void getparyx(WINDOW *win, int y, int x);
 *       void getbegyx(WINDOW *win, int y, int x);
 *       void getmaxyx(WINDOW *win, int y, int x);
 *
 *       void getsyx(int y, int x);
 *       int setsyx(int y, int x);
 *
 *       int getbegy(WINDOW *win);
 *       int getbegx(WINDOW *win);
 *       int getcury(WINDOW *win);
 *       int getcurx(WINDOW *win);
 *       int getpary(WINDOW *win);
 *       int getparx(WINDOW *win);
 *       int getmaxy(WINDOW *win);
 *       int getmaxx(WINDOW *win);
 *
 * Description:
 *       The getyx() macro (defined in curses.h -- the prototypes here
 *       are merely illustrative) puts the current cursor position of the
 *       specified window into y and x. getbegyx() and getmaxyx() return
 *       the starting coordinates and size of the specified window,
 *       respectively. getparyx() returns the starting coordinates of the
 *       parent's window, if the specified window is a subwindow;
 *       otherwise it sets y and x to -1. These are all macros.
 *
 *       getsyx() gets the coordinates of the virtual screen cursor, and
 *       stores them in y and x. If leaveok() is true, it returns -1, -1.
 *       If lines have been removed with ripoffline(), then getsyx()
 *       includes these lines in its count; so, the returned y and x
 *       values should only be used with setsyx().
 *
 *       setsyx() sets the virtual screen cursor to the y, x coordinates.
 *       If y, x are -1, -1, leaveok() is set true.
 *
 *       getsyx() and setsyx() are meant to be used by a library routine
 *       that manipulates curses windows without altering the position of
 *       the cursor. Note that getsyx() is defined only as a macro.
 *
 *       getbegy(), getbegx(), getcurx(), getcury(), getmaxy(),
 *       getmaxx(), getpary(), and getparx() return the appropriate
 *       coordinate or size values, or ERR in the case of a NULL window.
 *
 * Portability                                X/Open    BSD    SYS V
 *       getyx                                   Y       Y       Y
 *       getparyx                                -       -      4.0
 *       getbegyx                                -       -      3.0
 *       getmaxyx                                -       -      3.0
 *       getsyx                                  -       -      3.0
 *       setsyx                                  -       -      3.0
 *       getbegy                                 -       -       -
 *       getbegx                                 -       -       -
 *       getcury                                 -       -       -
 *       getcurx                                 -       -       -
 *       getpary                                 -       -       -
 *       getparx                                 -       -       -
 *       getmaxy                                 -       -       -
 *       getmaxx                                 -       -       -
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "curspriv.h"

/****************************************************************************
 * Publid Functions
 ****************************************************************************/

int getbegy(WINDOW *win)
{
  PDC_LOG(("getbegy() - called\n"));

  return win ? win->_begy : ERR;
}

int getbegx(WINDOW *win)
{
  PDC_LOG(("getbegx() - called\n"));

  return win ? win->_begx : ERR;
}

int getcury(WINDOW *win)
{
  PDC_LOG(("getcury() - called\n"));

  return win ? win->_cury : ERR;
}

int getcurx(WINDOW *win)
{
  PDC_LOG(("getcurx() - called\n"));

  return win ? win->_curx : ERR;
}

int getpary(WINDOW *win)
{
  PDC_LOG(("getpary() - called\n"));

  return win ? win->_pary : ERR;
}

int getparx(WINDOW *win)
{
  PDC_LOG(("getparx() - called\n"));

  return win ? win->_parx : ERR;
}

int getmaxy(WINDOW *win)
{
  PDC_LOG(("getmaxy() - called\n"));

  return win ? win->_maxy : ERR;
}

int getmaxx(WINDOW *win)
{
  PDC_LOG(("getmaxx() - called\n"));

  return win ? win->_maxx : ERR;
}

int setsyx(int y, int x)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("setsyx() - called\n"));

  if (y == -1 && x == -1)
    {
      curscr->_leaveit = true;
      return OK;
    }
  else
    {
      curscr->_leaveit = false;
      return wmove(curscr, y, x);
    }
}
