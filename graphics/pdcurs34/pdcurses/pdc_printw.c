/****************************************************************************
 * apps/graphics/pdcurses/pdc_printw.c
 * Public Domain Curses
 * RCSID("$Id: printw.c,v 1.40 2008/07/13 16:08:18 wmcbrine Exp $")
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

/* Name: printw
 *
 * Synopsis:
 *       int printw(const char *fmt, ...);
 *       int wprintw(WINDOW *win, const char *fmt, ...);
 *       int mvprintw(int y, int x, const char *fmt, ...);
 *       int mvwprintw(WINDOW *win, int y, int x, const char *fmt,...);
 *       int vwprintw(WINDOW *win, const char *fmt, va_list varglist);
 *       int vw_printw(WINDOW *win, const char *fmt, va_list varglist);
 *
 * Description:
 *       The printw() functions add a formatted string to the window at
 *       the current or specified cursor position. The format strings are
 *       the same as used in the standard C library's printf(). (printw()
 *       can be used as a drop-in replacement for printf().)
 *
 * Return Value:
 *       All functions return the number of characters printed, or
 *       ERR on error.
 *
 * Portability                                X/Open    BSD    SYS V
 *       printw                                  Y       Y       Y
 *       wprintw                                 Y       Y       Y
 *       mvprintw                                Y       Y       Y
 *       mvwprintw                               Y       Y       Y
 *       vwprintw                                Y       -      4.0
 *       vw_printw                               Y
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <string.h>

#include "curspriv.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int vwprintw(WINDOW *win, const char *fmt, va_list varglist)
{
  char printbuf[513];
  int len;

  PDC_LOG(("vwprintw() - called\n"));

  len = vsnprintf(printbuf, 512, fmt, varglist);
  return (waddstr(win, printbuf) == ERR) ? ERR : len;
}

int printw(const char *fmt, ...)
{
  va_list args;
  int retval;
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif

  PDC_LOG(("printw() - called\n"));

  va_start(args, fmt);
  retval = vwprintw(stdscr, fmt, args);
  va_end(args);

  return retval;
}

int wprintw(WINDOW *win, const char *fmt, ...)
{
  va_list args;
  int retval;

  PDC_LOG(("wprintw() - called\n"));

  va_start(args, fmt);
  retval = vwprintw(win, fmt, args);
  va_end(args);

  return retval;
}

int mvprintw(int y, int x, const char *fmt, ...)
{
  va_list args;
  int retval;
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif

  PDC_LOG(("mvprintw() - called\n"));

  if (move(y, x) == ERR)
    {
      return ERR;
    }

  va_start(args, fmt);
  retval = vwprintw(stdscr, fmt, args);
  va_end(args);

  return retval;
}

int mvwprintw(WINDOW *win, int y, int x, const char *fmt, ...)
{
  va_list args;
  int retval;

  PDC_LOG(("mvwprintw() - called\n"));

  if (wmove(win, y, x) == ERR)
    {
      return ERR;
    }

  va_start(args, fmt);
  retval = vwprintw(win, fmt, args);
  va_end(args);

  return retval;
}

int vw_printw(WINDOW *win, const char *fmt, va_list varglist)
{
  PDC_LOG(("vw_printw() - called\n"));

  return vwprintw(win, fmt, varglist);
}
