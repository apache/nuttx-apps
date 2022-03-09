/****************************************************************************
 * apps/graphics/pdcurses/pdc_delch.c
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

/* Name: delch
 *
 * Synopsis:
 *       int delch(void);
 *       int wdelch(WINDOW *win);
 *       int mvdelch(int y, int x);
 *       int mvwdelch(WINDOW *win, int y, int x);
 *
 * Description:
 *       The character under the cursor in the window is deleted.  All
 *       characters to the right on the same line are moved to the left
 *       one position and the last character on the line is filled with
 *       a blank.  The cursor position does not change (after moving to
 *       y, x if coordinates are specified).
 *
 * Return Value:
 *       All functions return OK on success and ERR on error.
 *
 * Portability                                X/Open    BSD    SYS V
 *       delch                                   Y       Y       Y
 *       wdelch                                  Y       Y       Y
 *       mvdelch                                 Y       Y       Y
 *       mvwdelch                                Y       Y       Y
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <string.h>

#include "curspriv.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int wdelch(WINDOW *win)
{
  int x;
  int y;
  int maxx;
  chtype *temp1;

  PDC_LOG(("wdelch() - called\n"));

  if (!win)
    {
      return ERR;
    }

  y     = win->_cury;
  x     = win->_curx;
  maxx  = win->_maxx - 1;
  temp1 = &win->_y[y][x];

  memmove(temp1, temp1 + 1, (maxx - x) * sizeof(chtype));

  /* wrs (4/10/93) account for window background */

  win->_y[y][maxx] = win->_bkgd;

  win->_lastch[y] = maxx;

  if ((win->_firstch[y] == _NO_CHANGE) || (win->_firstch[y] > x))
    {
      win->_firstch[y] = x;
    }

  PDC_sync(win);

  return OK;
}

int delch(void)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("delch() - called\n"));

  return wdelch(stdscr);
}

int mvdelch(int y, int x)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("mvdelch() - called\n"));

  if (move(y, x) == ERR)
    {
      return ERR;
    }

  return wdelch(stdscr);
}

int mvwdelch(WINDOW *win, int y, int x)
{
  PDC_LOG(("mvwdelch() - called\n"));

  if (wmove(win, y, x) == ERR)
    {
      return ERR;
    }

  return wdelch(win);
}
