/****************************************************************************
 * apps/graphics/pdcurses/pdc_scroll.c
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

/* Name: scroll
 *
 * Synopsis:
 *       int scroll(WINDOW *win);
 *       int scrl(int n);
 *       int wscrl(WINDOW *win, int n);
 *
 * Description:
 *       scroll() causes the window to scroll up one line.  This involves
 *       moving the lines in the window data structure.
 *
 *       With a positive n, scrl() and wscrl() scroll the window up n
 *       lines (line i + n becomes i); otherwise they scroll the window
 *       down n lines.

 *       For these functions to work, scrolling must be enabled via
 *       scrollok(). Note also that scrolling is not allowed if the
 *       supplied window is a pad.
 *
 * Return Value:
 *       All functions return OK on success and ERR on error.
 *
 * Portability                                X/Open    BSD    SYS V
 *       scroll                                  Y       Y       Y
 *       scrl                                    Y       -      4.0
 *       wscrl                                   Y       -      4.0
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "curspriv.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int wscrl(WINDOW *win, int n)
{
  chtype blank;
  chtype *temp;
  int dir;
  int start;
  int end;
  int l;
  int i;

  /* Check if window scrolls. Valid for window AND pad */

  if (!win || !win->_scroll || !n)
    {
      return ERR;
    }

  blank = win->_bkgd;

  if (n > 0)
    {
      start = win->_tmarg;
      end = win->_bmarg;
      dir = 1;
    }
  else
    {
      start = win->_bmarg;
      end = win->_tmarg;
      dir = -1;
    }

  for (l = 0; l < (n * dir); l++)
    {
      temp = win->_y[start];

      /* Re-arrange line pointers */

      for (i = start; i != end; i += dir)
        {
          win->_y[i] = win->_y[i + dir];
        }

      win->_y[end] = temp;

      /* Make a blank line */

      for (i = 0; i < win->_maxx; i++)
        {
          *temp++ = blank;
        }
    }

  touchline(win, win->_tmarg, win->_bmarg - win->_tmarg + 1);

  PDC_sync(win);
  return OK;
}

int scrl(int n)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("scrl() - called\n"));

  return wscrl(stdscr, n);
}

int scroll(WINDOW *win)
{
  PDC_LOG(("scroll() - called\n"));

  return wscrl(win, 1);
}
