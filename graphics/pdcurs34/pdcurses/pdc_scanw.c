/****************************************************************************
 * apps/graphics/pdcurses/pdc_scanw.c
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
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif

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
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif

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
