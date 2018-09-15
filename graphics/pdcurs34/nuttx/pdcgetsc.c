/****************************************************************************
 * apps/graphics/nuttx/pdcgetsc.c
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

#include "pdcnuttx.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: PDC_get_rows
 *
 * Description:
 *  Returns the size of the screen in rows. It's used in resize_term() to
 *  set the new value of LINES. (Some existing implementations also call it
 *  internally from PDC_scr_open(), but this is not required.)
 *
 ****************************************************************************/

int PDC_get_rows(void)
{
  FAR struct pdc_fbscreen_s *fbscreen = (FAR struct pdc_fbscreen_s *)SP;
  FAR struct pdc_fbstate_s *fbstate;

  PDC_LOG(("PDC_get_rows() - called\n"));

  DEBUGASSERT(fbscreen != NULL);
  fbstate = &fbscreen->fbstate;

  return fbstate->yres / fbstate->fheight;
}

/****************************************************************************
 * Name: PDC_get_columns
 *
 * Description:
 *   Returns the size of the screen in columns. It's used in resize_term()
 *   to set the new value of COLS. (Some existing implementations also call
 *   it internally from PDC_scr_open(), but this is not required.)
 *
 ****************************************************************************/

int PDC_get_columns(void)
{
  FAR struct pdc_fbscreen_s *fbscreen = (FAR struct pdc_fbscreen_s *)SP;
  FAR struct pdc_fbstate_s *fbstate;

  PDC_LOG(("PDC_get_columns() - called\n"));

  DEBUGASSERT(fbscreen != NULL);
  fbstate = &fbscreen->fbstate;

  return fbstate->xres / fbstate->fwidth;
}

/****************************************************************************
 * Name: PDC_get_cursor_mode
 *
 * Description:
 *   Returns the size/shape of the cursor. The format of the result is
 *   unspecified, except that it must be returned as an int. This function
 *   is called from initscr(), and the result is stored in SP->orig_cursor,
 *   which is used by PDC_curs_set() to determine the size/shape of the
 *   cursor in normal visibility mode (curs_set(1)).
 *
 ****************************************************************************/

int PDC_get_cursor_mode(void)
{
  PDC_LOG(("PDC_get_cursor_mode() - called\n"));
  return 0;
}
