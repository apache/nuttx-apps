/****************************************************************************
 * apps/graphics/pdcurses/pdc_inch.c
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
