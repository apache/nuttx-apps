/****************************************************************************
 * apps/graphics/nuttx/pdcsetsc.c
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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

#include <string.h>

#include "pdcnuttx.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: PDC_curs_set
 *
 * Description:
 *   Called from curs_set(). Changes the appearance of the cursor -- 0 turns
 *   it off, 1 is normal (the terminal's default, if applicable, as
 *   determined by SP->orig_cursor), and 2 is high visibility. The exact
 *   appearance of these modes is not specified.
 *
 ****************************************************************************/

int PDC_curs_set(int visibility)
{
  int ret;

  PDC_LOG(("PDC_curs_set() - called: visibility=%d\n", visibility));

  /* The return value is the previous visibility */

  ret = SP->visibility;

  /* Make sure that the new visibility is within range, then instantiate it. */

  if (visibility < 0)
    {
      visibility = 0;
    }
  else if (visibility > 2)
    {
      visibility = 2;
    }

  SP->visibility = visibility;

  /* Redraw the cursor of the visiblity has change.  For our purses 1 and 2
   * are currently treated the same.
   */

  if ((ret == 0 && visibility > 0) ||  /* From OFF to ON */
      (ret > 0  && visibility == 0))   /* From ON to OFF */
    {
      PDC_gotoyx(SP->cursrow, SP->curscol);
    }

  return ret;
}

/****************************************************************************
 * Name: PDC_set_title
 *
 * Description:
 *   PDC_set_title() sets the title of the window in which the curses
 *   program is running. This function may not do anything on some
 *   platforms. (Currently it only works in Win32 and X11.)
 *
 ****************************************************************************/

void PDC_set_title(const char *title)
{
  PDC_LOG(("PDC_set_title() - called:<%s>\n", title));
}

/****************************************************************************
 * Name: PDC_set_blink
 *
 * Description:
 *   PDC_set_blink() toggles whether the A_BLINK attribute sets an actual
 *   blink mode (true), or sets the background color to high intensity
 *   (false).  The default is platform-dependent (false in most cases).  It
 *   returns OK if it could set the state to match the given parameter,
 *   ERR otherwise. Current platforms also adjust the value of COLORS
 *   according to this function -- 16 for false, and 8 for true.
 *
 ****************************************************************************/

int PDC_set_blink(bool blinkon)
{
  COLORS = 16;
  return blinkon ? ERR : OK;
}
