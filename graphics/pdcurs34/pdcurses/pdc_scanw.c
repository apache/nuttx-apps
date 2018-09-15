/****************************************************************************
 * apps/graphics/pdcurses/pdc_scanw.c
 * Public Domain Curses
 * RCSID("$Id: scanw.c,v 1.42 2008/07/14 12:22:13 wmcbrine Exp $")
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

/* Name: scanw
 *
 * Synopsis:
 *       int scanw(const char *fmt, ...);
 *       int wscanw(WINDOW *win, const char *fmt, ...);
 *       int mvscanw(int y, int x, const char *fmt, ...);
 *       int mvwscanw(WINDOW *win, int y, int x, const char *fmt, ...);
 *       int vwscanw(WINDOW *win, const char *fmt, va_list varglist);
 *       int vw_scanw(WINDOW *win, const char *fmt, va_list varglist);
 *
 * Description:
 *       These routines correspond to the standard C library's scanf()
 *       family. Each gets a string from the window via wgetnstr(), and
 *       uses the resulting line as input for the scan.
 *
 * Return Value:
 *       On successful completion, these functions return the number of
 *       items successfully matched.  Otherwise they return ERR.
 *
 * Portability                                X/Open    BSD    SYS V
 *       scanw                                   Y       Y       Y
 *       wscanw                                  Y       Y       Y
 *       mvscanw                                 Y       Y       Y
 *       mvwscanw                                Y       Y       Y
 *       vwscanw                                 Y       -      4.0
 *       vw_scanw                                Y
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <string.h>

#include "curspriv.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int vwscanw(WINDOW *win, const char *fmt, va_list varglist)
{
  char scanbuf[256];

  PDC_LOG(("vwscanw() - called\n"));

  if (wgetnstr(win, scanbuf, 255) == ERR)
    {
      return ERR;
    }

  return vsscanf(scanbuf, fmt, varglist);
}

int scanw(const char *fmt, ...)
{
  va_list args;
  int retval;

  PDC_LOG(("scanw() - called\n"));

  va_start(args, fmt);
  retval = vwscanw(stdscr, fmt, args);
  va_end(args);

  return retval;
}

int wscanw(WINDOW *win, const char *fmt, ...)
{
  va_list args;
  int retval;

  PDC_LOG(("wscanw() - called\n"));

  va_start(args, fmt);
  retval = vwscanw(win, fmt, args);
  va_end(args);

  return retval;
}

int mvscanw(int y, int x, const char *fmt, ...)
{
  va_list args;
  int retval;

  PDC_LOG(("mvscanw() - called\n"));

  if (move(y, x) == ERR)
    {
      return ERR;
    }

  va_start(args, fmt);
  retval = vwscanw(stdscr, fmt, args);
  va_end(args);

  return retval;
}

int mvwscanw(WINDOW *win, int y, int x, const char *fmt, ...)
{
  va_list args;
  int retval;

  PDC_LOG(("mvscanw() - called\n"));

  if (wmove(win, y, x) == ERR)
    {
      return ERR;
    }

  va_start(args, fmt);
  retval = vwscanw(win, fmt, args);
  va_end(args);

  return retval;
}

int vw_scanw(WINDOW *win, const char *fmt, va_list varglist)
{
  PDC_LOG(("vw_scanw() - called\n"));

  return vwscanw(win, fmt, varglist);
}
