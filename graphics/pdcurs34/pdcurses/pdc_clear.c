/****************************************************************************
 * apps/graphics/pdcurses/pdc_clear.c
 * Public Domain Curses
 * RCSID("$Id: clear.c,v 1.35 2008/07/13 16:08:18 wmcbrine Exp $")
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

/* Name: clear
 *
 * Synopsis:
 *       int clear(void);
 *       int wclear(WINDOW *win);
 *       int erase(void);
 *       int werase(WINDOW *win);
 *       int clrtobot(void);
 *       int wclrtobot(WINDOW *win);
 *       int clrtoeol(void);
 *       int wclrtoeol(WINDOW *win);
 *
 * Description:
 *       erase() and werase() copy blanks (i.e. the background chtype) to
 *       every cell of the window.
 *
 *       clear() and wclear() are similar to erase() and werase(), but
 *       they also call clearok() to ensure that the the window is
 *       cleared on the next wrefresh().
 *
 *       clrtobot() and wclrtobot() clear the window from the current
 *       cursor position to the end of the window.
 *
 *       clrtoeol() and wclrtoeol() clear the window from the current
 *       cursor position to the end of the current line.
 *
 * Return Value:
 *       All functions return OK on success and ERR on error.
 *
 * Portability                                X/Open    BSD    SYS V
 *       clear                                   Y       Y       Y
 *       wclear                                  Y       Y       Y
 *       erase                                   Y       Y       Y
 *       werase                                  Y       Y       Y
 *       clrtobot                                Y       Y       Y
 *       wclrtobot                               Y       Y       Y
 *       clrtoeol                                Y       Y       Y
 *       wclrtoeol                               Y       Y       Y
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "curspriv.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int wclrtoeol(WINDOW *win)
{
  chtype blank;
  chtype *ptr;
  int x;
  int y;
  int minx;

  PDC_LOG(("wclrtoeol() - called: Row: %d Col: %d\n", win->_cury, win->_curx));

  if (!win)
    {
      return ERR;
    }

  y = win->_cury;
  x = win->_curx;

  /* wrs (4/10/93) account for window background */

  blank = win->_bkgd;

  for (minx = x, ptr = &win->_y[y][x]; minx < win->_maxx; minx++, ptr++)
    {
      *ptr = blank;
    }

  if (x < win->_firstch[y] || win->_firstch[y] == _NO_CHANGE)
    {
      win->_firstch[y] = x;
    }

  win->_lastch[y] = win->_maxx - 1;

  PDC_sync(win);
  return OK;
}

int clrtoeol(void)
{
  PDC_LOG(("clrtoeol() - called\n"));

  return wclrtoeol(stdscr);
}

int wclrtobot(WINDOW *win)
{
  int savey = win->_cury;
  int savex = win->_curx;

  PDC_LOG(("wclrtobot() - called\n"));

  if (!win)
    {
      return ERR;
    }

  /* should this involve scrolling region somehow ? */

  if (win->_cury + 1 < win->_maxy)
    {
      win->_curx = 0;
      win->_cury++;

      for (; win->_maxy > win->_cury; win->_cury++)
        {
          wclrtoeol(win);
        }

      win->_cury = savey;
      win->_curx = savex;
    }

  wclrtoeol(win);

  PDC_sync(win);
  return OK;
}

int clrtobot(void)
{
  PDC_LOG(("clrtobot() - called\n"));

  return wclrtobot(stdscr);
}

int werase(WINDOW *win)
{
  PDC_LOG(("werase() - called\n"));

  if (wmove(win, 0, 0) == ERR)
    {
      return ERR;
    }

  return wclrtobot(win);
}

int erase(void)
{
  PDC_LOG(("erase() - called\n"));

  return werase(stdscr);
}

int wclear(WINDOW *win)
{
  PDC_LOG(("wclear() - called\n"));

  if (!win)
    {
      return ERR;
    }

  win->_clear = true;
  return werase(win);
}

int clear(void)
{
  PDC_LOG(("clear() - called\n"));

  return wclear(stdscr);
}
