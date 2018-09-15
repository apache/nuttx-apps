/****************************************************************************
 * apps/graphics/pdcurses/pdc_beep.c
 * Public Domain Curses
 * RCSID("$Id: beep.c,v 1.34 2008/07/13 16:08:17 wmcbrine Exp $")
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
