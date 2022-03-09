/****************************************************************************
 * apps/examples/pdcurses/firework_main.c
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

#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>

#include "graphics/curses.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DELAYSIZE 200

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void myrefresh(void);
static void get_color(void);
static void explode(int, int);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static short color_table[] =
{
  COLOR_RED, COLOR_BLUE, COLOR_GREEN, COLOR_CYAN,
  COLOR_RED, COLOR_MAGENTA, COLOR_YELLOW, COLOR_WHITE
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void myrefresh(void)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif

  napms(DELAYSIZE);
  move(LINES - 1, COLS - 1);
  refresh();
}

static void get_color(void)
{
  chtype bold = (rand() % 2) ? A_BOLD : A_NORMAL;
  attrset(COLOR_PAIR(rand() % 8) | bold);
}

static void explode(int row, int col)
{
  erase();
  mvaddstr(row, col, "-");
  myrefresh();

  --col;

  get_color();
  mvaddstr(row - 1, col, " - ");
  mvaddstr(row, col, "-+-");
  mvaddstr(row + 1, col, " - ");
  myrefresh();

  --col;

  get_color();
  mvaddstr(row - 2, col, " --- ");
  mvaddstr(row - 1, col, "-+++-");
  mvaddstr(row, col, "-+#+-");
  mvaddstr(row + 1, col, "-+++-");
  mvaddstr(row + 2, col, " --- ");
  myrefresh();

  get_color();
  mvaddstr(row - 2, col, " +++ ");
  mvaddstr(row - 1, col, "++#++");
  mvaddstr(row, col, "+# #+");
  mvaddstr(row + 1, col, "++#++");
  mvaddstr(row + 2, col, " +++ ");
  myrefresh();

  get_color();
  mvaddstr(row - 2, col, "  #  ");
  mvaddstr(row - 1, col, "## ##");
  mvaddstr(row, col, "#   #");
  mvaddstr(row + 1, col, "## ##");
  mvaddstr(row + 2, col, "  #  ");
  myrefresh();

  get_color();
  mvaddstr(row - 2, col, " # # ");
  mvaddstr(row - 1, col, "#   #");
  mvaddstr(row, col, "     ");
  mvaddstr(row + 1, col, "#   #");
  mvaddstr(row + 2, col, " # # ");
  myrefresh();
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int start;
  int end;
  int row;
  int diff;
  int flag;
  int direction;
  int seed;
  int i;
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif

  traceon();
  initscr();
  nodelay(stdscr, true);
  noecho();

  if (has_colors())
    {
      start_color();
    }

  for (i = 0; i < 8; i++)
    {
      init_pair(i, color_table[i], COLOR_BLACK);
    }

  seed = time((time_t *) 0);
  srand(seed);
  flag = 0;

  while (getch() == ERR)        /* loop until a key is hit */
    {
      do
        {
          start = rand() % (COLS - 3);
          end = rand() % (COLS - 3);
          start = (start < 2) ? 2 : start;
          end = (end < 2) ? 2 : end;
          direction = (start > end) ? -1 : 1;
          diff = abs(start - end);
        }
      while (diff < 2 || diff >= LINES - 2);

      attrset(A_NORMAL);

      for (row = 0; row < diff; row++)
        {
          mvaddstr(LINES - row, row * direction + start,
                   (direction < 0) ? "\\" : "/");

          if (flag++)
            {
              myrefresh();
              erase();
              flag = 0;
            }
        }

      if (flag++)
        {
          myrefresh();
          flag = 0;
        }

      explode(LINES - row, diff * direction + start);
      erase();
      myrefresh();
    }

  endwin();
  return 0;
}
