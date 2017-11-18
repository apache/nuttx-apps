/****************************************************************************
 * apps/graphics/pdcurses/pdc_touch.c
 * Public Domain Curses
 * RCSID("$Id: touch.c,v 1.29 2008/07/13 16:08:18 wmcbrine Exp $")
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

/* Name: touch
 *
 * Synopsis:
 *       int touchwin(WINDOW *win);
 *       int touchline(WINDOW *win, int start, int count);
 *       int untouchwin(WINDOW *win);
 *       int wtouchln(WINDOW *win, int y, int n, int changed);
 *       bool is_linetouched(WINDOW *win, int line);
 *       bool is_wintouched(WINDOW *win);
 *
 * Description:
 *       touchwin() and touchline() throw away all information about
 *       which parts of the window have been touched, pretending that the
 *       entire window has been drawn on.  This is sometimes necessary
 *       when using overlapping windows, since a change to one window
 *       will affect the other window, but the records of which lines
 *       have been changed in the other window will not reflect the
 *       change.
 *
 *       untouchwin() marks all lines in the window as unchanged since
 *       the last call to wrefresh().
 *
 *       wtouchln() makes n lines in the window, starting at line y, look
 *       as if they have (changed == 1) or have not (changed == 0) been
 *       changed since the last call to wrefresh().
 *
 *       is_linetouched() returns true if the specified line in the
 *       specified window has been changed since the last call to
 *       wrefresh().
 *
 *       is_wintouched() returns true if the specified window
 *       has been changed since the last call to wrefresh().
 *
 * Return Value:
 *       All functions return OK on success and ERR on error except
 *       is_wintouched() and is_linetouched().
 *
 * Portability                                X/Open    BSD    SYS V
 *       touchwin                                Y       Y       Y
 *       touchline                               Y       -      3.0
 *       untouchwin                              Y       -      4.0
 *       wtouchln                                Y       Y       Y
 *       is_linetouched                          Y       -      4.0
 *       is_wintouched                           Y       -      4.0
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "curspriv.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int touchwin(WINDOW *win)
{
  int i;

  PDC_LOG(("touchwin() - called: Win=%x\n", win));

  if (!win)
    {
      return ERR;
    }

  for (i = 0; i < win->_maxy; i++)
    {
      win->_firstch[i] = 0;
      win->_lastch[i]  = win->_maxx - 1;
    }

  return OK;
}

int touchline(WINDOW *win, int start, int count)
{
  int i;

  PDC_LOG(("touchline() - called: win=%p start %d count %d\n",
           win, start, count));

  if (!win || start > win->_maxy || start + count > win->_maxy)
    {
      return ERR;
    }

  for (i = start; i < start + count; i++)
    {
      win->_firstch[i] = 0;
      win->_lastch[i]  = win->_maxx - 1;
    }

  return OK;
}

int untouchwin(WINDOW *win)
{
  int i;

  PDC_LOG(("untouchwin() - called: win=%p", win));

  if (!win)
    {
      return ERR;
    }

  for (i = 0; i < win->_maxy; i++)
    {
      win->_firstch[i] = _NO_CHANGE;
      win->_lastch[i]  = _NO_CHANGE;
    }

  return OK;
}

int wtouchln(WINDOW *win, int y, int n, int changed)
{
  int i;

  PDC_LOG(("wtouchln() - called: win=%p y=%d n=%d changed=%d\n",
           win, y, n, changed));

  if (!win || y > win->_maxy || y + n > win->_maxy)
    {
      return ERR;
    }

  for (i = y; i < y + n; i++)
    {
      if (changed)
        {
          win->_firstch[i] = 0;
          win->_lastch[i]  = win->_maxx - 1;
        }
      else
        {
          win->_firstch[i] = _NO_CHANGE;
          win->_lastch[i]  = _NO_CHANGE;
        }
    }

  return OK;
}

bool is_linetouched(WINDOW *win, int line)
{
  PDC_LOG(("is_linetouched() - called: win=%p line=%d\n", win, line));

  if (!win || line > win->_maxy || line < 0)
    {
      return false;
    }

  return (win->_firstch[line] != _NO_CHANGE) ? true : false;
}

bool is_wintouched(WINDOW *win)
{
  int i;

  PDC_LOG(("is_wintouched() - called: win=%p\n", win));

  if (win)
    {
      for (i = 0; i < win->_maxy; i++)
        {
          if (win->_firstch[i] != _NO_CHANGE)
            {
              return true;
            }
        }
    }

  return false;
}
