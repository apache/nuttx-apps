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
 *   Copyright (c) 2005 Free Software Foundation, Inc.
 *
 * Adapted from the original public domain pdcurses by Gregory Nutt
 ****************************************************************************/

/*
 *        @@@        @@@    @@@@@@@@@@     @@@@@@@@@@@    @@@@@@@@@@@@
 *        @@@        @@@   @@@@@@@@@@@@    @@@@@@@@@@@@   @@@@@@@@@@@@@
 *        @@@        @@@  @@@@      @@@@   @@@@           @@@@ @@@  @@@@
 *        @@@   @@   @@@  @@@        @@@   @@@            @@@  @@@   @@@
 *        @@@  @@@@  @@@  @@@        @@@   @@@            @@@  @@@   @@@
 *        @@@@ @@@@ @@@@  @@@        @@@   @@@            @@@  @@@   @@@
 *         @@@@@@@@@@@@   @@@@      @@@@   @@@            @@@  @@@   @@@
 *          @@@@  @@@@     @@@@@@@@@@@@    @@@            @@@  @@@   @@@
 *           @@    @@       @@@@@@@@@@     @@@            @@@  @@@   @@@
 *
 *                                Eric P. Scott
 *                         Caltech High Energy Physics
 *                                October, 1980
 *
 *                          Color by Eric S. Raymond
 *                                 July, 1995
 *
 * Options:
 *       -f                      fill screen with copies of 'WORM' at start.
 *       -l <n>                  set worm length
 *       -n <n>                  set number of worms
 *       -t                      make worms leave droppings
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "graphics/curses.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FLAVORS 7

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct worm
{
  int orientation, head;
  short *xpos, *ypos;
};

struct options
{
  int nopts;
  int opts[3];
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const chtype init_flavor[FLAVORS] =
{
  'O', '*', '#', '$', '%', '0', '@'
};

static const short xinc[] =
{
  1, 1, 1, 0, -1, -1, -1, 0
};

static const short yinc[] =
{
  -1, 0, 1, 1, 1, 0, -1, -1
};

static chtype flavor[FLAVORS];
static struct worm worm[40];
static const char *field;
static int length = 16;
static int number = 3;
static chtype trail = ' ';

static const struct options normal[8] =
{
  {
    3,
    {
      7, 0, 1
    }
  },
  {
    3,
    {
      0, 1, 2
    }
  },
  {
    3,
    {
      1, 2, 3
    }
  },
  {
    3,
    {
      2, 3, 4
    }
  },
  {
    3,
    {
      3, 4, 5
    }
  },
  {
    3,
    {
      4, 5, 6
    }
  },
  {
    3,
    {
      5, 6, 7
    }
  },
  {
    3,
    {
      6, 7, 0
    }
  }
};

static const struct options upper[8] =
{
  {
    1,
    {
      1, 0, 0
    }
  },
  {
    2,
    {
      1, 2, 0
    }
  },
  {
    0,
    {
      0, 0, 0
    }
  },
  {
    0,
    {
      0, 0, 0
    }
  },
  {
    0,
    {
      0, 0, 0
    }
  },
  {
    2,
    {
      4, 5, 0
    }
  },
  {
    1,
    {
      5, 0, 0
    }
  },
  {
    2,
    {
      1, 5, 0
    }
  }
};

static const struct options left[8] =
{
  {
    0,
    {
      0, 0, 0
    }
  },
  {
    0,
    {
      0, 0, 0
    }
  },
  {
    0,
    {
      0, 0, 0
    }
  },
  {
    2,
    {
      2, 3, 0
    }
  },
  {
    1,
    {
      3, 0, 0
    }
  },
  {
    2,
    {
      3, 7, 0
    }
  },
  {
    1,
    {
      7, 0, 0
    }
  },
  {
    2,
    {
      7, 0, 0
    }
  }
};

static const struct options right[8] =
{
  {
    1,
    {
      7, 0, 0
    }
  },
  {
    2,
    {
      3, 7, 0
    }
  },
  {
    1,
    {
      3, 0, 0
    }
  },
  {
    2,
    {
      3, 4, 0
    }
  },
  {
    0,
    {
      0, 0, 0
    }
  },
  {
    0,
    {
      0, 0, 0
    }
  },
  {
    0,
    {
      0, 0, 0
    }
  },
  {
    2,
    {
      6, 7, 0
    }
  }
};

static const struct options lower[8] =
{
  {
    0,
    {
      0, 0, 0
    }
  },
  {
    2,
    {
      0, 1, 0
    }
  },
  {
    1,
    {
      1, 0, 0
    }
  },
  {
    2,
    {
      1, 5, 0
    }
  },
  {
    1,
    {
      5, 0, 0
    }
  },
  {
    2,
    {
      5, 6, 0
    }
  },
  {
    0,
    {
      0, 0, 0
    }
  },
  {
    0,
    {
      0, 0, 0
    }
  }
};

static const struct options upleft[8] =
{
  {
    0,
    {
      0, 0, 0
    }
  },
  {
    0,
    {
      0, 0, 0
    }
  },
  {
    0,
    {
      0, 0, 0
    }
  },
  {
    0,
    {
      0, 0, 0
    }
  },
  {
    0,
    {
      0, 0, 0
    }
  },
  {
    1,
    {
      3, 0, 0
    }
  },
  {
    2,
    {
      1, 3, 0
    }
  },
  {
    1,
    {
      1, 0, 0
    }
  }
};

static const struct options upright[8] =
{
  {
    2,
    {
      3, 5, 0
    }
  },
  {
    1,
    {
      3, 0, 0
    }
  },
  {
    0,
    {
      0, 0, 0
    }
  },
  {
    0,
    {
      0, 0, 0
    }
  },
  {
    0,
    {
      0, 0, 0
    }
  },
  {
    0,
    {
      0, 0, 0
    }
  },
  {
    0,
    {
      0, 0, 0
    }
  },
  {
    1,
    {
      5, 0, 0
    }
  }
};

static const struct options lowleft[8] =
{
  {
    3,
    {
      7, 0, 1
    }
  },
  {
    0,
    {
      0, 0, 0
    }
  },
  {
    0,
    {
      0, 0, 0
    }
  },
  {
    1,
    {
      1, 0, 0
    }
  },
  {
    2,
    {
      1, 7, 0
    }
  },
  {
    1,
    {
      7, 0, 0
    }
  },
  {
    0,
    {
      0, 0, 0
    }
  },
  {
    0,
    {
      0, 0, 0
    }
  }
};

static const struct options lowright[8] =
{
  {
    0,
    {
      0, 0, 0
    }
  },
  {
    1,
    {
      7, 0, 0
    }
  },
  {
    2,
    {
      5, 7, 0
    }
  },
  {
    1,
    {
      5, 0, 0
    }
  },
  {
    0,
    {
     0, 0, 0
    }
  },
  {
    0,
    {
      0, 0, 0
    }
  },
  {
    0,
    {
      0, 0, 0
    }
  },
  {
    0,
    {
      0, 0, 0
    }
  }
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void cleanup(void)
{
  standend();
  refresh();
  curs_set(1);
  endwin();
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  const struct options *op;
  struct worm *w;
  short **ref;
  short *ip;
  int x;
  int y;
  int n;
  int h;
  int last;
  int bottom;
  int seed;
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif

  for (x = 1; x < argc; x++)
    {
      char *p = argv[x];

      if (*p == '-')
        {
          p++;
        }

      switch (*p)
        {
        case 'f':
          field = "WORM";
          break;

        case 'l':
          if (++x == argc)
            {
              goto usage;
            }

          if ((length = atoi(argv[x])) < 2 || length > 1024)
            {
              fprintf(stderr, "%s: Invalid length\n", *argv);
              return EXIT_FAILURE;
            }

          break;

        case 'n':
          if (++x == argc)
            {
              goto usage;
            }

          if ((number = atoi(argv[x])) < 1 || number > 40)
            {
              fprintf(stderr, "%s: Invalid number of worms\n", *argv);
              return EXIT_FAILURE;
            }

          break;

        case 't':
          trail = '.';
          break;

        default:
        usage:
          fprintf(stderr, "usage: %s [-field] [-length #] "
                  "[-number #] [-trail]\n", *argv);
          return EXIT_FAILURE;
        }
    }

  memcpy(flavor, init_flavor, FLAVORS * sizeof(chtype));

  traceon();
  initscr();
  seed = time((time_t *) 0);
  srand(seed);

  noecho();
  cbreak();
  nonl();
  keypad(stdscr, true);

  curs_set(0);

  bottom = LINES - 1;
  last   = COLS - 1;

#ifdef A_COLOR
  if (has_colors())
    {
      int bg = COLOR_BLACK;
      start_color();

#  if defined(NCURSES_VERSION) || (defined(PDC_BUILD) && PDC_BUILD > 3000)
      if (use_default_colors() == OK)
        {
          bg = -1;
        }
#  endif

#  define SET_COLOR(num, fg) \
        init_pair(num + 1, fg, bg); \
        flavor[num] |= COLOR_PAIR(num + 1) | A_BOLD

      SET_COLOR(0, COLOR_GREEN);
      SET_COLOR(1, COLOR_RED);
      SET_COLOR(2, COLOR_CYAN);
      SET_COLOR(3, COLOR_WHITE);
      SET_COLOR(4, COLOR_MAGENTA);
      SET_COLOR(5, COLOR_BLUE);
      SET_COLOR(6, COLOR_YELLOW);
    }
#endif

  ref = malloc(sizeof(short *) * LINES);

  for (y = 0; y < LINES; y++)
    {
      ref[y] = malloc(sizeof(short) * COLS);

      for (x = 0; x < COLS; x++)
        {
          ref[y][x] = 0;
        }
    }

#ifdef BADCORNER
  /* if addressing the lower right corner doesn't work in your curses */

  ref[bottom][last] = 1;
#endif

  for (n = number, w = &worm[0]; --n >= 0; w++)
    {
      w->orientation = w->head = 0;

      if ((ip = malloc(sizeof(short) * (length + 1))) == NULL)
        {
          fprintf(stderr, "%s: out of memory\n", *argv);
          return EXIT_FAILURE;
        }

      w->xpos = ip;

      for (x = length; --x >= 0;)
        {
          *ip++ = -1;
        }

      if ((ip = malloc(sizeof(short) * (length + 1))) == NULL)
        {
          fprintf(stderr, "%s: out of memory\n", *argv);
          return EXIT_FAILURE;
        }

      w->ypos = ip;

      for (y = length; --y >= 0;)
        {
          *ip++ = -1;
        }
    }

  if (field)
    {
      const char *p = field;

      for (y = bottom; --y >= 0;)
        {
          for (x = COLS; --x >= 0;)
            {
              addch((chtype) (*p++));

              if (!*p)
                {
                  p = field;
                }
            }
        }
    }

  napms(12);
  refresh();
  nodelay(stdscr, true);

  for (;;)
    {
      int ch;

      if ((ch = getch()) > 0)
        {
#ifdef KEY_RESIZE
          if (ch == KEY_RESIZE)
            {
              resize_term(0, 0);
              erase();

              if (last != COLS - 1)
                {
                  for (y = 0; y <= bottom; y++)
                    {
                      ref[y] = realloc(ref[y], sizeof(short) * COLS);

                      for (x = last + 1; x < COLS; x++)
                        {
                          ref[y][x] = 0;
                        }
                    }

                  last = COLS - 1;
                }

              if (bottom != LINES - 1)
                {
                  for (y = LINES; y <= bottom; y++)
                    {
                      free(ref[y]);
                    }

                  ref = realloc(ref, sizeof(short *) * LINES);

                  for (y = bottom + 1; y < LINES; y++)
                    {
                      ref[y] = malloc(sizeof(short) * COLS);

                      for (x = 0; x < COLS; x++)
                        {
                          ref[y][x] = 0;
                        }
                    }

                  bottom = LINES - 1;
                }
            }

#endif /* KEY_RESIZE */

          /* Make it simple to put this into single-step mode, or resume normal
           * operation - T. Dickey
           */

          if (ch == 'q')
            {
              cleanup();
              return EXIT_SUCCESS;
            }
          else if (ch == 's')
            {
              nodelay(stdscr, false);
            }
          else if (ch == ' ')
            {
              nodelay(stdscr, true);
            }
        }

      for (n = 0, w = &worm[0]; n < number; n++, w++)
        {
          if ((x = w->xpos[h = w->head]) < 0)
            {
              move(y = w->ypos[h] = bottom, x = w->xpos[h] = 0);
              addch(flavor[n % FLAVORS]);
              ref[y][x]++;
            }
          else
            {
              y = w->ypos[h];
            }

          if (x > last)
            {
              x = last;
            }

          if (y > bottom)
            {
              y = bottom;
            }


          if (++h == length)
            {
              h = 0;
            }

          if (w->xpos[w->head = h] >= 0)
            {
              int x1 = w->xpos[h];
              int y1 = w->ypos[h];

              if (y1 < LINES && x1 < COLS && --ref[y1][x1] == 0)
                {
                  move(y1, x1);
                  addch(trail);
                }
            }

          op = &(x == 0 ? (y == 0 ? upleft :
                           (y == bottom ? lowleft : left)) :
                 (x == last ? (y == 0 ? upright :
                               (y == bottom ? lowright : right)) :
                  (y == 0 ? upper :
                   (y == bottom ? lower : normal))))[w->orientation];

          switch (op->nopts)
            {
            case 0:
              cleanup();
              return EXIT_SUCCESS;

            case 1:
              w->orientation = op->opts[0];
              break;

            default:
              w->orientation = op->opts[rand() % op->nopts];
              break;
            }

          move(y += yinc[w->orientation], x += xinc[w->orientation]);

          if (y < 0)
            {
              y = 0;
            }

          addch(flavor[n % FLAVORS]);
          ref[w->ypos[h] = y][w->xpos[h] = x]++;
        }

      napms(12);
      refresh();
    }
}
