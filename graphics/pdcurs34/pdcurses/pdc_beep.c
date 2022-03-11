/****************************************************************************
 * apps/graphics/pdcurses/pdc_beep.c
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

/* Name: beep
 *
 * Synopsis:
 *       int beep(void);
 *       int flash(void);
 *
 * Description:
 *       beep() sounds the audible bell on the terminal, if possible;
 *       if not, it calls flash().
 *
 *       flash() "flashes" the screen, by inverting the foreground and
 *       background of every cell, pausing, and then restoring the
 *       original attributes.
 *
 * Return Value:
 *       These functions return OK.
 *
 * Portability                                X/Open    BSD    SYS V
 *       beep                                    Y       Y       Y
 *       flash                                   Y       Y       Y
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "curspriv.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int beep(void)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("beep() - called\n"));

  if (SP->audible)
    {
      PDC_beep();
    }
  else
    {
      flash();
    }

  return OK;
}

int flash(void)
{
  int x;
  int y;
  int z;
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif

  PDC_LOG(("flash() - called\n"));

  /* Reverse each cell; wait; restore the screen */

  for (z = 0; z < 2; z++)
    {
      for (y = 0; y < LINES; y++)
        {
          for (x = 0; x < COLS; x++)
            {
              curscr->_y[y][x] ^= A_REVERSE;
            }
        }

      wrefresh(curscr);

      if (!z)
        {
          napms(50);
        }
    }

  return OK;
}
