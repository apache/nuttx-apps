/****************************************************************************
 * apps/examples/pdcurses/rain_main.c
 * $Id: rain.c,v 1.11 2008/07/13 16:08:17 wmcbrine Exp $
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
 *   Copyright (c) 2002 Free Software Foundation, Inc
 *   Adapted by: Gregory Nutt <gnutt@nuttx.org>
 *
 * Adapted from the original public domain pdcurses by Gregory Nutt and
 * released as part of NuttX under the origin ncurses license:
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, distribute with modifications, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the
 * sale, use or other dealings in this Software without prior written
 * authorization.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>
#include <time.h>

#include "graphics/curses.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int next_j(int j)
{
  if (j == 0)
    {
      j = 4;
    }
  else
    {
      --j;
    }

  if (has_colors())
    {
      int z = rand() % 3;
      chtype color = COLOR_PAIR(z);

      if (z)
        {
          color |= A_BOLD;
        }

      attrset(color);
    }

  return j;
}

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int rain_main(int argc, char *argv[])
#endif
{
  int x, y, j, r, c, seed;
  static int xpos[5], ypos[5];

  traceon();
  initscr();
  seed = time((time_t *) 0);
  srand(seed);

  if (has_colors())
    {
      int bg = COLOR_BLACK;

      start_color();

#if defined(NCURSES_VERSION) || (defined(PDC_BUILD) && PDC_BUILD > 3000)
      if (use_default_colors() == OK)
        {
          bg = -1;
        }
#endif

      init_pair(1, COLOR_BLUE, bg);
      init_pair(2, COLOR_CYAN, bg);
    }

  nl();
  noecho();
  curs_set(0);
  timeout(0);
  keypad(stdscr, true);

  r = LINES - 4;
  c = COLS - 4;

  for (j = 5; --j >= 0;)
    {
      xpos[j] = rand() % c + 2;
      ypos[j] = rand() % r + 2;
    }

  for (j = 0;;)
    {
      x = rand() % c + 2;
      y = rand() % r + 2;

      mvaddch(y, x, '.');

      mvaddch(ypos[j], xpos[j], 'o');

      j = next_j(j);
      mvaddch(ypos[j], xpos[j], 'O');

      j = next_j(j);
      mvaddch(ypos[j] - 1, xpos[j], '-');
      mvaddstr(ypos[j], xpos[j] - 1, "|.|");
      mvaddch(ypos[j] + 1, xpos[j], '-');

      j = next_j(j);
      mvaddch(ypos[j] - 2, xpos[j], '-');
      mvaddstr(ypos[j] - 1, xpos[j] - 1, "/ \\");
      mvaddstr(ypos[j], xpos[j] - 2, "| O |");
      mvaddstr(ypos[j] + 1, xpos[j] - 1, "\\ /");
      mvaddch(ypos[j] + 2, xpos[j], '-');

      j = next_j(j);
      mvaddch(ypos[j] - 2, xpos[j], ' ');
      mvaddstr(ypos[j] - 1, xpos[j] - 1, "   ");
      mvaddstr(ypos[j], xpos[j] - 2, "     ");
      mvaddstr(ypos[j] + 1, xpos[j] - 1, "   ");
      mvaddch(ypos[j] + 2, xpos[j], ' ');

      xpos[j] = x;
      ypos[j] = y;

      switch (getch())
        {
        case 'q':
        case 'Q':
          curs_set(1);
          endwin();
          return EXIT_SUCCESS;

        case 's':
          nodelay(stdscr, false);
          break;

        case ' ':
          nodelay(stdscr, true);
#ifdef KEY_RESIZE
          break;

        case KEY_RESIZE:
          resize_term(0, 0);
          erase();
          r = LINES - 4;
          c = COLS - 4;
          break;
#endif
        }

      napms(50);
    }
}
