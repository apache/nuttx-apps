/****************************************************************************
 * apps/graphics/pdcurses/term.h
 * Public Domain Curses
 * PDCurses doesn't operate with terminfo, but we need these functions for
 * compatibility, to allow some things (notably, interface libraries for
 * other languages) to be compiled. Anyone who tries to actually _use_
 * them will be disappointed, since they only return ERR.
 * $Id: term.h,v 1.16 2008/07/13 16:08:16 wmcbrine Exp $
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

#ifndef __APPS_GRAPHICS_PDCURS34_INCLUDE_TERM_H
#define __APPS_GRAPHICS_PDCURS34_INCLUDE_TERM_H 1

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "graphics/curses.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

#if defined(__cplusplus)
extern "C"
{
#  define EXTERN extern "C"
#else
#  define EXTERN extern
#endif

typedef struct
{
  const char *_termname;
} TERMINAL;

/****************************************************************************
 * Public Data
 ****************************************************************************/

EXTERN TERMINAL *cur_term;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int     del_curterm(TERMINAL *);
int     putp(const char *);
int     restartterm(const char *, int, int *);
TERMINAL *set_curterm(TERMINAL *);
int     setterm(const char *);
int     setupterm(const char *, int, int *);
int     tgetent(char *, const char *);
int     tgetflag(const char *);
int     tgetnum(const char *);
char   *tgetstr(const char *, char **);
char   *tgoto(const char *, int, int);
int     tigetflag(const char *);
int     tigetnum(const char *);
char   *tigetstr(const char *);
char   *tparm(const char *, long, long, long, long, long,
              long, long, long, long);
int     tputs(const char *, int, int (*)(int));

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __APPS_GRAPHICS_PDCURS34_INCLUDE_TERM_H */
