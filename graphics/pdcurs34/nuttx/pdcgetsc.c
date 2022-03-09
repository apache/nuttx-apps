/****************************************************************************
 * apps/graphics/nuttx/pdcgetsc.c
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

#ifdef CONFIG_SYSTEM_TERMCURSES
#include <nuttx/termcurses.h>
#endif

#include <sys/ioctl.h>
#include <assert.h>

#include "pdcnuttx.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: PDC_get_columns_term
 *
 * Description:
 *   Returns the size of the screen in columns. It's used in resize_term()
 *   to set the new value of COLS. (Some existing implementations also call
 *   it internally from PDC_scr_open(), but this is not required.)
 *
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_TERMCURSES
int PDC_get_columns_term(FAR SCREEN *sp)
{
  FAR struct pdc_termscreen_s *termscreen = (FAR struct pdc_termscreen_s *)sp;
  FAR struct pdc_termstate_s  *termstate = &termscreen->termstate;
  struct winsize winsz;
  int ret;

  /* Call termcurses function to get window size */

  ret =termcurses_getwinsize(termstate->tcurs, &winsz);
  if (ret == OK)
    {
      return winsz.ws_col;
    }

  return ret;
}
#endif /* CONFIG_SYSTEM_TERMCURSES */

/****************************************************************************
 * Name: PDC_get_rows
 *
 * Description:
 *  Returns the size of the screen in rows. It's used in resize_term() to
 *  set the new value of LINES. (Some existing implementations also call it
 *  internally from PDC_scr_open(), but this is not required.)
 *
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_TERMCURSES
int PDC_get_rows_term(FAR SCREEN *sp)
{
  FAR struct pdc_termscreen_s *termscreen = (FAR struct pdc_termscreen_s *)sp;
  FAR struct pdc_termstate_s  *termstate = &termscreen->termstate;
  struct winsize winsz;
  int ret;

  /* Call termcurses function to get window size */

  ret =termcurses_getwinsize(termstate->tcurs, &winsz);
  if (ret == OK)
    {
      return winsz.ws_row;
    }

  return ret;
}
#endif /* CONFIG_SYSTEM_TERMCURSES */

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
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  FAR struct pdc_fbscreen_s *fbscreen = (FAR struct pdc_fbscreen_s *)SP;
  FAR struct pdc_fbstate_s *fbstate;

#ifdef CONFIG_SYSTEM_TERMCURSES
  if (!graphic_screen)
    {
       /* Call termcurses version of PDC_get_rows */

       return PDC_get_rows_term(SP);
    }
#endif

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
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  FAR struct pdc_fbscreen_s *fbscreen = (FAR struct pdc_fbscreen_s *)SP;
  FAR struct pdc_fbstate_s *fbstate;

  PDC_LOG(("PDC_get_columns() - called\n"));

#ifdef CONFIG_SYSTEM_TERMCURSES
  if (!graphic_screen)
    {
       /* Call termcurses version of PDC_get_cols */

       return PDC_get_columns_term(SP);
    }
#endif

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
