/****************************************************************************
 * apps/examples/pdcurses/charset_main.c
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
 * Included Files
 ****************************************************************************/

#include "graphics/curses.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FIRST_CH 0x20
#define LAST_CH  0x7e
#define NUM_CH   (LAST_CH - FIRST_CH + 1)

/****************************************************************************
 * Private Data
 ****************************************************************************/

static short color_table[] =
{
  COLOR_RED, COLOR_BLUE, COLOR_GREEN, COLOR_CYAN,
  COLOR_RED, COLOR_MAGENTA, COLOR_YELLOW, COLOR_WHITE
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  chtype ch;
  int lastch;
  int width;
  int height;
  int xoffset;
  int yoffset;
  int row;
  int col;
  int i;
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif

  /* Initialize */

  traceon();
  initscr();
  noecho();

  /* Setup colors */

  if (has_colors())
    {
      start_color();
    }

  for (i = 0; i < 8; i++)
    {
      init_pair(i, color_table[i], COLOR_BLACK);
    }

  /* Set up geometry */
  /* First get the maximum width */

  width = COLS - 2;
  if (width > NUM_CH)
    {
      width = NUM_CH;
    }

  /* Get the height associated with the maximum width */

  height = (NUM_CH + width - 1) / width;
  lastch = LAST_CH;

  if (height > LINES)
    {
      height = LINES;
      lastch = FIRST_CH + width * height - 1;
    }

  /* Reduce the width if it will give us a more balanced layout */

  while (height < (LINES / 2) && width > (COLS / 2))
    {
      int numch = lastch - FIRST_CH + 1;

      width >>= 1;
      height  = (numch + width - 1) / width;
    }

  /* Center the rectangle containing the font set */

  xoffset = (COLS - width) / 2;
  yoffset = (LINES - height) / 2;

  /* Now display the character set using that geometry */

  ch = FIRST_CH;
  for (row = yoffset; row < yoffset + height; row++)
    {
      for (col = xoffset; col < xoffset + width; col++)
        {
          mvaddch(row, col, ch);
          if (++ch > lastch)
            {
              break;
            }
        }
    }

  refresh();
  endwin();
  return 0;
}
