/****************************************************************************
 * apps/graphics/pdcurses/pdc_inch.c
 * Public Domain Curses
 * RCSID("$Id: inch.c,v 1.33 2008/07/13 16:08:18 wmcbrine Exp $")
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

/* Name: inch
 *
 * Synopsis:
 *       chtype inch(void);
 *       chtype winch(WINDOW *win);
 *       chtype mvinch(int y, int x);
 *       chtype mvwinch(WINDOW *win, int y, int x);
 *
 *       int in_wch(cchar_t *wcval);
 *       int win_wch(WINDOW *win, cchar_t *wcval);
 *       int mvin_wch(int y, int x, cchar_t *wcval);
 *       int mvwin_wch(WINDOW *win, int y, int x, cchar_t *wcval);
 *
 * Description:
 *       The inch() functions retrieve the character and attribute from
 *       the current or specified window position, in the form of a
 *       chtype. If a NULL window is specified, (chtype)ERR is returned.
 *
 *       The in_wch() functions are the wide-character versions; instead
 *       of returning a chtype, they store a cchar_t at the address
 *       specified by wcval, and return OK or ERR. (No value is stored
 *       when ERR is returned.) Note that in PDCurses, chtype and cchar_t
 *       are the same.
 *
 * Portability                                X/Open    BSD    SYS V
 *       inch                                    Y       Y       Y
 *       winch                                   Y       Y       Y
 *       mvinch                                  Y       Y       Y
 *       mvwinch                                 Y       Y       Y
 *       in_wch                                  Y
 *       win_wch                                 Y
 *       mvin_wch                                Y
 *       mvwin_wch                               Y
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "curspriv.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

chtype winch(WINDOW *win)
{
  PDC_LOG(("winch() - called\n"));

  if (!win)
    {
      return (chtype) ERR;
    }

  return win->_y[win->_cury][win->_curx];
}

chtype inch(void)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("inch() - called\n"));

  return winch(stdscr);
}

chtype mvinch(int y, int x)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("mvinch() - called\n"));

  if (move(y, x) == ERR)
    {
      return (chtype)ERR;
    }

  return stdscr->_y[stdscr->_cury][stdscr->_curx];
}

chtype mvwinch(WINDOW *win, int y, int x)
{
  PDC_LOG(("mvwinch() - called\n"));

  if (wmove(win, y, x) == ERR)
    {
      return (chtype) ERR;
    }

  return win->_y[win->_cury][win->_curx];
}

#ifdef CONFIG_PDCURSES_WIDE
int win_wch(WINDOW *win, cchar_t *wcval)
{
  PDC_LOG(("win_wch() - called\n"));

  if (!win || !wcval)
    {
      return ERR;
    }

  *wcval = win->_y[win->_cury][win->_curx];

  return OK;
}

int in_wch(cchar_t *wcval)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("in_wch() - called\n"));

  return win_wch(stdscr, wcval);
}

int mvin_wch(int y, int x, cchar_t *wcval)
{
  PDC_LOG(("mvin_wch() - called\n"));

  if (!wcval || (move(y, x) == ERR))
    {
      return ERR;
    }

  *wcval = stdscr->_y[stdscr->_cury][stdscr->_curx];
  return OK;
}

int mvwin_wch(WINDOW *win, int y, int x, cchar_t *wcval)
{
  PDC_LOG(("mvwin_wch() - called\n"));

  if (!wcval || (wmove(win, y, x) == ERR))
    {
      return ERR;
    }

  *wcval = win->_y[win->_cury][win->_curx];
  return OK;
}
#endif
