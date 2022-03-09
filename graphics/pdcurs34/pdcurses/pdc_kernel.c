/****************************************************************************
 * apps/graphics/pdcurses/pdc_kernel.c
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

/* Name: kernel
 *
 * Synopsis:
 *       int def_prog_mode(void);
 *       int def_shell_mode(void);
 *       int reset_prog_mode(void);
 *       int reset_shell_mode(void);
 *       int resetty(void);
 *       int savetty(void);
 *       int ripoffline(int line, int (*init)(WINDOW *, int));
 *       int curs_set(int visibility);
 *       int napms(int ms);
 *
 *       int draino(int ms);
 *       int resetterm(void);
 *       int fixterm(void);
 *       int saveterm(void);
 *
 * Description:
 *       def_prog_mode() and def_shell_mode() save the current terminal
 *       modes as the "program" (in curses) or "shell" (not in curses)
 *       state for use by the reset_prog_mode() and reset_shell_mode()
 *       functions.  This is done automatically by initscr().
 *
 *       reset_prog_mode() and reset_shell_mode() restore the terminal to
 *       "program" (in curses) or "shell" (not in curses) state.  These
 *       are done automatically by endwin() and doupdate() after an
 *       endwin(), so they would normally not be called before these
 *       functions.
 *
 *       savetty() and resetty() save and restore the state of the
 *       terminal modes. savetty() saves the current state in a buffer,
 *       and resetty() restores the state to what it was at the last call
 *       to savetty().
 *
 *       curs_set() alters the appearance of the cursor. A visibility of
 *       0 makes it disappear; 1 makes it appear "normal" (usually an
 *       underline) and 2 makes it "highly visible" (usually a block).
 *
 *       ripoffline() reduces the size of stdscr by one line.  If the
 *       "line" parameter is positive, the line is removed from the top
 *       of the screen; if negative, from the bottom. Up to 5 lines can
 *       be ripped off stdscr by calling ripoffline() repeatedly. The
 *       function argument, init, is called from within initscr() or
 *       newterm(), so ripoffline() must be called before either of these
 *       functions.  The init function receives a pointer to a one-line
 *       WINDOW, and the width of the window. Calling ripoffline() with a
 *       NULL init function pointer is an error.
 *
 *       napms() suspends the program for the specified number of
 *       milliseconds. draino() is an archaic equivalent.
 *
 *       resetterm(), fixterm() and saveterm() are archaic equivalents
 *       for reset_shell_mode(), reset_prog_mode() and def_prog_mode(),
 *       respectively.
 *
 * Return Value:
 *       All functions return OK on success and ERR on error, except
 *       curs_set(), which returns the previous visibility.
 *
 * Portability                                X/Open    BSD    SYS V
 *       def_prog_mode                           Y       Y       Y
 *       def_shell_mode                          Y       Y       Y
 *       reset_prog_mode                         Y       Y       Y
 *       reset_shell_mode                        Y       Y       Y
 *       resetty                                 Y       Y       Y
 *       savetty                                 Y       Y       Y
 *       ripoffline                              Y       -      3.0
 *       curs_set                                Y       -      3.0
 *       napms                                   Y       Y       Y
 *       draino                                  -
 *       resetterm                               -
 *       fixterm                                 -
 *       saveterm                                -
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <string.h>

#include "curspriv.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum
{
  PDC_SH_TTY, PDC_PR_TTY, PDC_SAVE_TTY
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifndef CONFIG_PDCURSES_MULTITHREAD
RIPPEDOFFLINE linesripped[5];
char linesrippedoff = 0;
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifndef CONFIG_PDCURSES_MULTITHREAD
static struct cttyset ctty[3];
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void _save_mode(int i)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  ctty[i].been_set = true;

  memcpy(&(ctty[i].saved), SP, sizeof(SCREEN));

  PDC_save_screen_mode(i);
}

static int _restore_mode(int i)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  if (ctty[i].been_set == true)
    {
      memcpy(SP, &(ctty[i].saved), sizeof(SCREEN));

      if (ctty[i].saved.raw_out)
        {
          raw();
        }

      PDC_restore_screen_mode(i);

      if ((LINES != ctty[i].saved.lines) || (COLS != ctty[i].saved.cols))
        {
          resize_term(ctty[i].saved.lines, ctty[i].saved.cols);
        }

      PDC_curs_set(ctty[i].saved.visibility);
      PDC_gotoyx(ctty[i].saved.cursrow, ctty[i].saved.curscol);
    }

  return ctty[i].been_set ? OK : ERR;
}

int def_prog_mode(void)
{
  PDC_LOG(("def_prog_mode() - called\n"));

  _save_mode(PDC_PR_TTY);
  return OK;
}

int def_shell_mode(void)
{
  PDC_LOG(("def_shell_mode() - called\n"));

  _save_mode(PDC_SH_TTY);
  return OK;
}

int reset_prog_mode(void)
{
  PDC_LOG(("reset_prog_mode() - called\n"));

  _restore_mode(PDC_PR_TTY);
  PDC_reset_prog_mode();
  return OK;
}

int reset_shell_mode(void)
{
  PDC_LOG(("reset_shell_mode() - called\n"));

  _restore_mode(PDC_SH_TTY);
  PDC_reset_shell_mode();
  return OK;
}

int resetty(void)
{
  PDC_LOG(("resetty() - called\n"));

  return _restore_mode(PDC_SAVE_TTY);
}

int savetty(void)
{
  PDC_LOG(("savetty() - called\n"));

  _save_mode(PDC_SAVE_TTY);
  return OK;
}

int curs_set(int visibility)
{
  int ret_vis;
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif

  PDC_LOG(("curs_set() - called: visibility=%d\n", visibility));

  if ((visibility < 0) || (visibility > 2))
    {
      return ERR;
    }

  ret_vis = PDC_curs_set(visibility);

  /* If the cursor is changing from invisible to visible, update its
   * position.
   */

  if (visibility && !ret_vis)
    {
      PDC_gotoyx(SP->cursrow, SP->curscol);
    }

  return ret_vis;
}

int napms(int ms)
{
  PDC_LOG(("napms() - called: ms=%d\n", ms));

  if (ms)
    {
      PDC_napms(ms);
    }

  return OK;
}

int ripoffline(int line, int (*init) (WINDOW *, int))
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("ripoffline() - called: line=%d\n", line));

  if (linesrippedoff < 5 && line && init)
    {
      linesripped[(int)linesrippedoff].line = line;
      linesripped[(int)linesrippedoff++].init = init;

      return OK;
    }

  return ERR;
}

int draino(int ms)
{
  PDC_LOG(("draino() - called\n"));

  return napms(ms);
}

int resetterm(void)
{
  PDC_LOG(("resetterm() - called\n"));

  return reset_shell_mode();
}

int fixterm(void)
{
  PDC_LOG(("fixterm() - called\n"));

  return reset_prog_mode();
}

int saveterm(void)
{
  PDC_LOG(("saveterm() - called\n"));

  return def_prog_mode();
}
