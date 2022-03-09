/****************************************************************************
 * apps/graphics/pdcurses/pdc_bkgd.c
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

/* Name: bkgd
 *
 * Synopsis:
 *       int bkgd(chtype ch);
 *       void bkgdset(chtype ch);
 *       chtype getbkgd(WINDOW *win);
 *       int wbkgd(WINDOW *win, chtype ch);
 *       void wbkgdset(WINDOW *win, chtype ch);
 *
 *       int bkgrnd(const cchar_t *wch);
 *       void bkgrndset(const cchar_t *wch);
 *       int getbkgrnd(cchar_t *wch);
 *       int wbkgrnd(WINDOW *win, const cchar_t *wch);
 *       void wbkgrndset(WINDOW *win, const cchar_t *wch);
 *       int wgetbkgrnd(WINDOW *win, cchar_t *wch);
 *
 * Description:
 *       bkgdset() and wbkgdset() manipulate the background of a window.
 *       The background is a chtype consisting of any combination of
 *       attributes and a character; it is combined with each chtype
 *       added or inserted to the window by waddch() or winsch(). Only
 *       the attribute part is used to set the background of non-blank
 *       characters, while both character and attributes are used for
 *       blank positions.
 *
 *       bkgd() and wbkgd() not only change the background, but apply it
 *       immediately to every cell in the window.
 *
 *       The attributes that are defined with the attrset()/attron() set
 *       of functions take precedence over the background attributes if
 *       there is a conflict (e.g., different color pairs).
 *
 * Return Value:
 *       bkgd() and wbkgd() return OK, unless the window is NULL, in
 *       which case they return ERR.
 *
 * Portability                                X/Open    BSD    SYS V
 *       bkgd                                    Y       -      4.0
 *       bkgdset                                 Y       -      4.0
 *       getbkgd                                 Y
 *       wbkgd                                   Y       -      4.0
 *       wbkgdset                                Y       -      4.0
 *       bkgrnd                                  Y
 *       bkgrndset                               Y
 *       getbkgrnd                               Y
 *       wbkgrnd                                 Y
 *       wbkgrndset                              Y
 *       wgetbkgrnd                              Y
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "curspriv.h"

int wbkgd(WINDOW *win, chtype ch)
{
  int x;
  int y;
  chtype oldcolr;
  chtype oldch;
  chtype newcolr;
  chtype newch;
  chtype colr;
  chtype attr;
  chtype oldattr = 0;
  chtype newattr = 0;
  chtype *winptr;

  PDC_LOG(("wbkgd() - called\n"));

  if (!win)
    {
      return ERR;
    }

  if (win->_bkgd == ch)
    {
      return OK;
    }

  oldcolr = win->_bkgd & A_COLOR;
  if (oldcolr)
    {
      oldattr = (win->_bkgd & A_ATTRIBUTES) ^ oldcolr;
    }

  oldch = win->_bkgd & A_CHARTEXT;

  wbkgdset(win, ch);

  newcolr = win->_bkgd & A_COLOR;
  if (newcolr)
    {
      newattr = (win->_bkgd & A_ATTRIBUTES) ^ newcolr;
    }

  newch = win->_bkgd & A_CHARTEXT;

  /* What follows is what seems to occur in the System V implementation of
   * this routine.
   */

  for (y = 0; y < win->_maxy; y++)
    {
      for (x = 0; x < win->_maxx; x++)
        {
          winptr = win->_y[y] + x;

          ch = *winptr;

          /* Determine the colors and attributes of the character read from
           * the window.
           */

          colr = ch & A_COLOR;
          attr = ch & (A_ATTRIBUTES ^ A_COLOR);

          /* If the color is the same as the old background color, then make
           * it the new background color, otherwise leave it.
           */

          if (colr == oldcolr)
            {
              colr = newcolr;
            }

          /* Remove any attributes (non color) from the character that were
           * part of the old background, then combine the remaining ones with
           * the new background */

          attr ^= oldattr;
          attr |= newattr;

          /* Change character if it is there because it was the old background
           * character.
           */

          ch &= A_CHARTEXT;
          if (ch == oldch)
            {
              ch = newch;
            }

          ch |= (attr | colr);

          *winptr = ch;
        }
    }

  touchwin(win);
  PDC_sync(win);
  return OK;
}

int bkgd(chtype ch)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("bkgd() - called\n"));

  return wbkgd(stdscr, ch);
}

void wbkgdset(WINDOW *win, chtype ch)
{
  PDC_LOG(("wbkgdset() - called\n"));

  if (win)
    {
      if (!(ch & A_CHARTEXT))
        {
          ch |= ' ';
        }

      win->_bkgd = ch;
    }
}

void bkgdset(chtype ch)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("bkgdset() - called\n"));

  wbkgdset(stdscr, ch);
}

chtype getbkgd(WINDOW *win)
{
  PDC_LOG(("getbkgd() - called\n"));

  return win ? win->_bkgd : (chtype) ERR;
}

#ifdef CONFIG_PDCURSES_WIDE
int wbkgrnd(WINDOW *win, const cchar_t *wch)
{
  PDC_LOG(("wbkgrnd() - called\n"));

  return wch ? wbkgd(win, *wch) : ERR;
}

int bkgrnd(const cchar_t *wch)
{
  PDC_LOG(("bkgrnd() - called\n"));

  return wbkgrnd(stdscr, wch);
}

void wbkgrndset(WINDOW *win, const cchar_t *wch)
{
  PDC_LOG(("wbkgdset() - called\n"));

  if (wch)
    {
      wbkgdset(win, *wch);
    }
}

void bkgrndset(const cchar_t *wch)
{
  PDC_LOG(("bkgrndset() - called\n"));

  wbkgrndset(stdscr, wch);
}

int wgetbkgrnd(WINDOW *win, cchar_t *wch)
{
  PDC_LOG(("wgetbkgrnd() - called\n"));

  if (!win || !wch)
    {
      return ERR;
    }

  *wch = win->_bkgd;

  return OK;
}

int getbkgrnd(cchar_t *wch)
{
  PDC_LOG(("getbkgrnd() - called\n"));

  return wgetbkgrnd(stdscr, wch);
}
#endif
