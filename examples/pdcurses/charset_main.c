/****************************************************************************
 * apps/examples/pdcurses/charset_main.c
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
 *   Adapted by: Gregory Nutt <gnutt@nuttx.org>
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
