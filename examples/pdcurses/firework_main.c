/****************************************************************************
 * apps/examples/pdcurses/firework_main.c
 * $Id: firework.c,v 1.25 2008/07/13 16:08:17 wmcbrine Exp $
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
 *   Adapted by: Gregory Nutt <gnutt@nuttx.org>
 *
 * Adapted from the original public domain pdcurses by Gregory Nutt and
 * released as part of NuttX under the 3-clause BSD license:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
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

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int firework_main(int argc, char *argv[])
#endif
{
  int start;
  int end;
  int row;
  int diff;
  int flag;
  int direction;
  int seed;
  int i;

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
