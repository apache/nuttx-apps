/****************************************************************************
 * apps/graphics/pdcurses/pdc_deleteln.c
 * Public Domain Curses
 * RCSID("$Id: deleteln.c,v 1.35 2008/07/13 16:08:18 wmcbrine Exp $")
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

/* Name: deleteln
 *
 * Synopsis:
 *       int deleteln(void);
 *       int wdeleteln(WINDOW *win);
 *       int insdelln(int n);
 *       int winsdelln(WINDOW *win, int n);
 *       int insertln(void);
 *       int winsertln(WINDOW *win);
 *
 *       int mvdeleteln(int y, int x);
 *       int mvwdeleteln(WINDOW *win, int y, int x);
 *       int mvinsertln(int y, int x);
 *       int mvwinsertln(WINDOW *win, int y, int x);
 *
 * Description:
 *       With the deleteln() and wdeleteln() functions, the line under
 *       the cursor in the window is deleted.  All lines below the
 *       current line are moved up one line.  The bottom line of the
 *       window is cleared.  The cursor position does not change.
 *
 *       With the insertln() and winsertn() functions, a blank line is
 *       inserted above the current line and the bottom line is lost.
 *
 *       mvdeleteln(), mvwdeleteln(), mvinsertln() and mvwinsertln()
 *       allow moving the cursor and inserting/deleting in one call.
 *
 * Return Value:
 *       All functions return OK on success and ERR on error.
 *
 * Portability                                X/Open    BSD    SYS V
 *       deleteln                                Y       Y       Y
 *       wdeleteln                               Y       Y       Y
 *       mvdeleteln                              -       -       -
 *       mvwdeleteln                             -       -       -
 *       insdelln                                Y       -      4.0
 *       winsdelln                               Y       -      4.0
 *       insertln                                Y       Y       Y
 *       winsertln                               Y       Y       Y
 *       mvinsertln                              -       -       -
 *       mvwinsertln                             -       -       -
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "curspriv.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int wdeleteln(WINDOW *win)
{
  chtype blank;
  chtype *temp;
  chtype *ptr;
  int y;

  PDC_LOG(("wdeleteln() - called\n"));

  if (!win)
    {
      return ERR;
    }

  /* wrs (4/10/93) account for window background */

  blank = win->_bkgd;

  temp = win->_y[win->_cury];

  for (y = win->_cury; y < win->_bmarg; y++)
    {
      win->_y[y] = win->_y[y + 1];
      win->_firstch[y] = 0;
      win->_lastch[y] = win->_maxx - 1;
    }

  for (ptr = temp; (ptr - temp < win->_maxx); ptr++)
    {
      *ptr = blank;             /* make a blank line */
    }

  if (win->_cury <= win->_bmarg)
    {
      win->_firstch[win->_bmarg] = 0;
      win->_lastch[win->_bmarg] = win->_maxx - 1;
      win->_y[win->_bmarg] = temp;
    }

  return OK;
}

int deleteln(void)
{
  PDC_LOG(("deleteln() - called\n"));

  return wdeleteln(stdscr);
}

int mvdeleteln(int y, int x)
{
  PDC_LOG(("mvdeleteln() - called\n"));

  if (move(y, x) == ERR)
    {
      return ERR;
    }

  return wdeleteln(stdscr);
}

int mvwdeleteln(WINDOW *win, int y, int x)
{
  PDC_LOG(("mvwdeleteln() - called\n"));

  if (wmove(win, y, x) == ERR)
    {
      return ERR;
    }

  return wdeleteln(win);
}

int winsdelln(WINDOW *win, int n)
{
  int i;

  PDC_LOG(("winsdelln() - called\n"));

  if (!win)
    {
      return ERR;
    }

  if (n > 0)
    {
      for (i = 0; i < n; i++)
        {
          if (winsertln(win) == ERR)
            {
              return ERR;
            }
        }
    }
  else if (n < 0)
    {
      n = -n;
      for (i = 0; i < n; i++)
        {
          if (wdeleteln(win) == ERR)
            {
              return ERR;
            }
        }
    }

  return OK;
}

int insdelln(int n)
{
  PDC_LOG(("insdelln() - called\n"));

  return winsdelln(stdscr, n);
}

int winsertln(WINDOW *win)
{
  chtype blank, *temp, *end;
  int y;

  PDC_LOG(("winsertln() - called\n"));

  if (!win)
    {
      return ERR;
    }

  /* wrs (4/10/93) account for window background */

  blank = win->_bkgd;

  temp = win->_y[win->_maxy - 1];

  for (y = win->_maxy - 1; y > win->_cury; y--)
    {
      win->_y[y] = win->_y[y - 1];
      win->_firstch[y] = 0;
      win->_lastch[y] = win->_maxx - 1;
    }

  win->_y[win->_cury] = temp;

  for (end = &temp[win->_maxx - 1]; temp <= end; temp++)
    {
      *temp = blank;
    }

  win->_firstch[win->_cury] = 0;
  win->_lastch[win->_cury] = win->_maxx - 1;

  return OK;
}

int insertln(void)
{
  PDC_LOG(("insertln() - called\n"));

  return winsertln(stdscr);
}

int mvinsertln(int y, int x)
{
  PDC_LOG(("mvinsertln() - called\n"));

  if (move(y, x) == ERR)
    {
      return ERR;
    }

  return winsertln(stdscr);
}

int mvwinsertln(WINDOW *win, int y, int x)
{
  PDC_LOG(("mvwinsertln() - called\n"));

  if (wmove(win, y, x) == ERR)
    {
      return ERR;
    }

  return winsertln(win);
}
