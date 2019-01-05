/****************************************************************************
 * apps/graphics/pdcurses/pdc_terminfo.c
 * Public Domain Curses
 * RCSID("$Id: terminfo.c,v 1.37 2008/07/21 12:29:20 wmcbrine Exp $")
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

/* Name: terminfo
 *
 * Synopsis:
 *       int mvcur(int oldrow, int oldcol, int newrow, int newcol);
 *       int vidattr(chtype attr);
 *       int vid_attr(attr_t attr, short color_pair, void *opt);
 *       int vidputs(chtype attr, int (*putfunc)(int));
 *       int vid_puts(attr_t attr, short color_pair, void *opt,
 *               int (*putfunc)(int));
 *
 *       int del_curterm(TERMINAL *);
 *       int putp(const char *);
 *       int restartterm(const char *, int, int *);
 *       TERMINAL *set_curterm(TERMINAL *);
 *       int setterm(const char *term);
 *       int setupterm(const char *, int, int *);
 *       int tgetent(char *, const char *);
 *       int tgetflag(const char *);
 *       int tgetnum(const char *);
 *       char *tgetstr(const char *, char **);
 *       char *tgoto(const char *, int, int);
 *       int tigetflag(const char *);
 *       int tigetnum(const char *);
 *       char *tigetstr(const char *);
 *       char *tparm(const char *,long, long, long, long, long, long,
 *               long, long, long);
 *       int tputs(const char *, int, int (*)(int));
 *
 * Description:
 *       mvcur() lets you move the physical cursor without updating any
 *       window cursor positions. It returns OK or ERR.
 *
 *       The rest of these functions are currently implemented as stubs,
 *       returning the appropriate errors and doing nothing else.
 *
 * Portability                                X/Open    BSD    SYS V
 *       mvcur                                   Y       Y       Y
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>
#include "term.h"
#include "curspriv.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifndef CONFIG_PDCURSES_MULTITHREAD
TERMINAL *cur_term = NULL;
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_PDCURSES_MULTITHREAD
void *pdc_alloc_term_ctx(void)
{
  return (void *) zalloc(sizeof(TERMINAL));
}
#endif

int mvcur(int oldrow, int oldcol, int newrow, int newcol)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("mvcur() - called: oldrow %d oldcol %d newrow %d newcol %d\n",
           oldrow, oldcol, newrow, newcol));

  if ((newrow >= LINES) || (newcol >= COLS) || (newrow < 0) || (newcol < 0))
    {
      return ERR;
    }

  PDC_gotoyx(newrow, newcol);
  SP->cursrow = newrow;
  SP->curscol = newcol;
  return OK;
}

int vidattr(chtype attr)
{
  PDC_LOG(("vidattr() - called: attr %d\n", attr));

  return ERR;
}

int vid_attr(attr_t attr, short color_pair, void *opt)
{
  PDC_LOG(("vid_attr() - called\n"));

  return ERR;
}

int vidputs(chtype attr, int (*putfunc) (int))
{
  PDC_LOG(("vidputs() - called: attr %d\n", attr));

  return ERR;
}

int vid_puts(attr_t attr, short color_pair, void *opt, int (*putfunc) (int))
{
  PDC_LOG(("vid_puts() - called\n"));

  return ERR;
}

int del_curterm(TERMINAL * oterm)
{
  PDC_LOG(("del_curterm() - called\n"));

  return ERR;
}

int putp(const char *str)
{
  PDC_LOG(("putp() - called: str %s\n", str));

  return ERR;
}

int restartterm(const char *term, int filedes, int *errret)
{
  PDC_LOG(("restartterm() - called\n"));

  if (errret)
    {
      *errret = -1;
    }

  return ERR;
}

TERMINAL *set_curterm(TERMINAL * nterm)
{
  PDC_LOG(("set_curterm() - called\n"));

  return (TERMINAL *) NULL;
}

int setterm(const char *term)
{
  PDC_LOG(("setterm() - called\n"));

  return ERR;
}

int setupterm(const char *term, int filedes, int *errret)
{
  PDC_LOG(("setupterm() - called\n"));

  if (errret)
    {
      *errret = -1;
    }
  else
    {
      fprintf(stderr, "There is no terminfo database\n");
    }

  return ERR;
}

int tgetent(char *bp, const char *name)
{
  PDC_LOG(("tgetent() - called: name %s\n", name));

  return ERR;
}

int tgetflag(const char *id)
{
  PDC_LOG(("tgetflag() - called: id %s\n", id));

  return ERR;
}

int tgetnum(const char *id)
{
  PDC_LOG(("tgetnum() - called: id %s\n", id));

  return ERR;
}

char *tgetstr(const char *id, char **area)
{
  PDC_LOG(("tgetstr() - called: id %s\n", id));

  return (char *)NULL;
}

char *tgoto(const char *cap, int col, int row)
{
  PDC_LOG(("tgoto() - called\n"));

  return (char *)NULL;
}

int tigetflag(const char *capname)
{
  PDC_LOG(("tigetflag() - called: capname %s\n", capname));

  return -1;
}

int tigetnum(const char *capname)
{
  PDC_LOG(("tigetnum() - called: capname %s\n", capname));

  return -2;
}

char *tigetstr(const char *capname)
{
  PDC_LOG(("tigetstr() - called: capname %s\n", capname));

  return (char *)(-1);
}

char *tparm(const char *cap, long p1, long p2, long p3, long p4,
            long p5, long p6, long p7, long p8, long p9)
{
  PDC_LOG(("tparm() - called: cap %s\n", cap));

  return (char *)NULL;
}

int tputs(const char *str, int affcnt, int (*putfunc) (int))
{
  PDC_LOG(("tputs() - called\n"));

  return ERR;
}
