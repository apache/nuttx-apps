/****************************************************************************
 * apps/graphics/pdcurses/pdc_move.c
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

/* Name: move
 *
 * Synopsis:
 *       int move(int y, int x);
 *       int wmove(WINDOW *win, int y, int x);
 *
 * Description:
 *       The cursor associated with the window is moved to the given
 *       location.  This does not move the physical cursor of the
 *       terminal until refresh() is called.  The position specified is
 *       relative to the upper left corner of the window, which is (0,0).
 *
 * Return Value:
 *       All functions return OK on success and ERR on error.
 *
 * Portability                                X/Open    BSD    SYS V
 *       move                                    Y       Y       Y
 *       wmove                                   Y       Y       Y
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "curspriv.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int move(int y, int x)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("move() - called: y=%d x=%d\n", y, x));

  if (!stdscr || x < 0 || y < 0 || x >= stdscr->_maxx || y >= stdscr->_maxy)
    {
      return ERR;
    }

  stdscr->_curx = x;
  stdscr->_cury = y;
  return OK;
}

int wmove(WINDOW *win, int y, int x)
{
  PDC_LOG(("wmove() - called: y=%d x=%d\n", y, x));

  if (!win || x < 0 || y < 0 || x >= win->_maxx || y >= win->_maxy)
    {
      return ERR;
    }

  win->_curx = x;
  win->_cury = y;
  return OK;
}
