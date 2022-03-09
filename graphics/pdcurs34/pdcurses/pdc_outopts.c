/****************************************************************************
 * apps/graphics/pdcurses/pdc_outopts.c
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

/* Name: outopts
 *
 * Synopsis:
 *       int clearok(WINDOW *win, bool bf);
 *       int idlok(WINDOW *win, bool bf);
 *       void idcok(WINDOW *win, bool bf);
 *       void immedok(WINDOW *win, bool bf);
 *       int leaveok(WINDOW *win, bool bf);
 *       int setscrreg(int top, int bot);
 *       int wsetscrreg(WINDOW *win, int top, int bot);
 *       int scrollok(WINDOW *win, bool bf);
 *
 *       int raw_output(bool bf);
 *
 * Description:
 *       With clearok(), if bf is true, the next call to wrefresh() with
 *       this window will clear the screen completely and redraw the
 *       entire screen.
 *
 *       immedok(), called with a second argument of true, causes an
 *       automatic wrefresh() every time a change is made to the
 *       specified window.
 *
 *       Normally, the hardware cursor is left at the location of the
 *       window being refreshed.  leaveok() allows the cursor to be
 *       left wherever the update happens to leave it.  It's useful
 *       for applications where the cursor is not used, since it reduces
 *       the need for cursor motions.  If possible, the cursor is made
 *       invisible when this option is enabled.
 *
 *       wsetscrreg() sets a scrolling region in a window; "top" and
 *       "bot" are the line numbers for the top and bottom margins. If
 *       this option and scrollok() are enabled, any attempt to move off
 *       the bottom margin will cause all lines in the scrolling region
 *       to scroll up one line. setscrreg() is the stdscr version.
 *
 *       idlok() and idcok() do nothing in PDCurses, but are provided for
 *       compatibility with other curses implementations.
 *
 *       raw_output() enables the output of raw characters using the
 *       standard *add* and *ins* curses functions (that is, it disables
 *       translation of control characters).
 *
 * Return Value:
 *       All functions return OK on success and ERR on error.
 *
 * Portability                                X/Open    BSD    SYS V
 *       clearok                                 Y       Y       Y
 *       idlok                                   Y       Y       Y
 *       idcok                                   Y       -      4.0
 *       immedok                                 Y       -      4.0
 *       leaveok                                 Y       Y       Y
 *       setscrreg                               Y       Y       Y
 *       wsetscrreg                              Y       Y       Y
 *       scrollok                                Y       Y       Y
 *       raw_output                              -       -       -
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "curspriv.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int clearok(WINDOW *win, bool bf)
{
  PDC_LOG(("clearok() - called\n"));

  if (!win)
    {
      return ERR;
    }

  win->_clear = bf;
  return OK;
}

int idlok(WINDOW *win, bool bf)
{
  PDC_LOG(("idlok() - called\n"));

  return OK;
}

void idcok(WINDOW *win, bool bf)
{
  PDC_LOG(("idcok() - called\n"));
}

void immedok(WINDOW *win, bool bf)
{
  PDC_LOG(("immedok() - called\n"));

  if (win)
    {
      win->_immed = bf;
    }
}

int leaveok(WINDOW *win, bool bf)
{
  PDC_LOG(("leaveok() - called\n"));

  if (!win)
    {
      return ERR;
    }

  win->_leaveit = bf;
  curs_set(!bf);
  return OK;
}

int setscrreg(int top, int bottom)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("setscrreg() - called: top %d bottom %d\n", top, bottom));

  return wsetscrreg(stdscr, top, bottom);
}

int wsetscrreg(WINDOW *win, int top, int bottom)
{
  PDC_LOG(("wsetscrreg() - called: top %d bottom %d\n", top, bottom));

  if (win && 0 <= top && top <= win->_cury &&
      win->_cury <= bottom && bottom < win->_maxy)
    {
      win->_tmarg = top;
      win->_bmarg = bottom;

      return OK;
    }
  else
    {
      return ERR;
    }
}

int scrollok(WINDOW *win, bool bf)
{
  PDC_LOG(("scrollok() - called\n"));

  if (!win)
    {
      return ERR;
    }

  win->_scroll = bf;
  return OK;
}

int raw_output(bool bf)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("raw_output() - called\n"));

  SP->raw_out = bf;
  return OK;
}
