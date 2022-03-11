/****************************************************************************
 * apps/graphics/pdcurses/pdc_printw.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Adapted from the original public domain pdcurses by Gregory Nutt
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
