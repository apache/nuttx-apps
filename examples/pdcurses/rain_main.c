/****************************************************************************
 * apps/examples/pdcurses/rain_main.c
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

int main(int argc, FAR char *argv[])
{
  int x, y, j, r, c, seed;
  static int xpos[5], ypos[5];
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif

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
