/****************************************************************************
 * apps/graphics/nuttx/pdcdisp.c
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
 * Public Data
 ****************************************************************************/

/* A port of PDCurses must provide acs_map[], a 128-element array of chtypes,
 * with values laid out based on the Alternate Character Set of the VT100
 * (see curses.h).  PDC_transform_line() must use this table; when it
 * encounters a chtype with the A_ALTCHARSET flag set, and an A_CHARTEXT
 * value in the range 0-127, it must render it using the A_CHARTEXT portion
 * of the corresponding value from this table, instead of the original
 * value.  Also, values may be read from this table by apps, and passed
 * through functions such as waddch(), which does no special processing on
 * control characters (0-31 and 127) when the A_ALTCHARSET flag is set.
 * Thus, any control characters used in acs_map[] should also have the
 * A_ALTCHARSET flag set. Implementations should provide suitable values
 * for all the ACS_ macros defined in curses.h; other values in the table
 * should be filled with their own indices (e.g., acs_map['E'] == 'E'). The
 * table can be either hardwired, or filled by PDC_scr_open(). Existing
 * ports define it in pdcdisp.c, but this is not required.
 */

chtype acs_map[128];

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: PDC_gotoyx
 *
 * Description:
 *   Move the physical cursor (as opposed to the logical cursor affected by
 *   wmove()) to the given location. T his is called mainly from doupdate().
 *   In general, this function need not compare the old location with the
 *   new one, and should just move the cursor unconditionally.
 *
 ****************************************************************************/

/* Position hardware cursor at (y, x) */

void PDC_gotoyx(int row, int col)
{
  chtype ch;
  int oldrow;
  int oldcol;

  PDC_LOG(("PDC_gotoyx() - called: row %d col %d\n", row, col));

  if (SP->mono)
    {
      return;
    }

  /* Clear the old cursor */

  oldrow = SP->cursrow;
  oldcol = SP->curscol;

  PDC_transform_line(oldrow, oldcol, 1, curscr->_y[oldrow] + oldcol);

  if (SP->visibility == 0)
    {
      return;
    }

  /* Draw a new cursor by overprinting the existing character in reverse,
   * either the full cell (when visibility == 2) or the lowest quarter of
   * it (when visibility == 1)
   */

  ch = curscr->_y[row][col] ^ A_REVERSE;

  PDC_transform_line(row, col, 1, &ch);
}

/****************************************************************************
 * Name: PDC_transform_line
 *
 * Description:
 *   The core output routine.  It takes len chtype entities from srcp (a
 *   pointer into curscr) and renders them to the physical screen at line
 *   lineno, column x.  It must also translate characters 0-127 via acs_map[],
 *   if they're flagged with A_ALTCHARSET in the attribute portion of the
 *   chtype.
 *
 ****************************************************************************/

void PDC_transform_line(int lineno, int x, int len, const chtype *srcp)
{
  FAR struct pdc_fbscreen_s *fbscreen = (FAR struct pdc_fbscreen_s *)SP;
  FAR struct pdc_fbstate_s *fbstate;
  chtype ch;
  short fg;
  short bg;
  bool bold;
  int i;

  PDC_LOG(("PDC_transform_line() - called: lineno=%d x=%d len=%d\n",
           lineno, x, len));

  DEBUGASSERT(fbscreen != NULL);
  fbstate = &fbscreen->fbstate;

  /* Set up the start position for the transfer */
#warning Missing logic

  /* Add each character to the framebuffer at the current position,
   * incrementing the horizontal position after each character.
   */

  for (i = 0; i < len; i++)
    {
      ch = srcp[i];

      /* Get the forground and background colors of the font */

      PDC_pair_content(PAIR_NUMBER(ch), &fg, &bg);

      /* Handle attributes */

      bold = false;
      if (!SP->mono)
        {
          /* REVISIT:  Only bold and reverse attributed handled */
          /* Bold will select the bold font.
          */

          bold = ((ch & A_BOLD) != 0);

          /* Swap the foreground and background colors if reversed */

          if ((ch & A_REVERSE) != 0)
            {
              short tmp = fg;
              fg = bg;
              bg = tmp;
            }
        }

#ifdef CONFIG_PDCURSESCHTYPE_LONG
      /* Translate characters 0-127 via acs_map[], if they're flagged with
       * A_ALTCHARSET in the attribute portion of the chtype.
       */

      if (ch & A_ALTCHARSET && !(ch & 0xff80))
        {
          ch = (ch & (A_ATTRIBUTES ^ A_ALTCHARSET)) | acs_map[ch & 0x7f];
        }
#endif

      /* Rend the font glyph into the framebuffer */
#warning Missing logic

      /* Apply more attributes */

      if ((ch & (A_UNDERLINE | A_LEFTLINE | A_RIGHTLINE)) != 0)
        {
#warning Missing logic
        }

      /* Increment the X position by the font width */
#warning Missing logic
    }
}
