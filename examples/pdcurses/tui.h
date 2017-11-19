/****************************************************************************
 * apps/examples/pdcurses/tui.c
 * Textual User Interface
 *
 *   Author : P.J. Kunst <kunst@prl.philips.nl>
 *   Date   : 25-02-93
 *
 * $Id: tui.h,v 1.11 2008/07/14 12:35:23 wmcbrine Exp $
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

#ifndef __APPS_EXAMPLES_PDCURSES_TUI_H
#define __APPS_EXAMPLES_PDCURSES_TUI_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "graphics/curses.h"

/****************************************************************************
 * Pre-processor Defintiions
 ****************************************************************************/

#ifdef A_COLOR
#  define A_ATTR  (A_ATTRIBUTES ^ A_COLOR)  /* A_BLINK, A_REVERSE, A_BOLD */
#else
#  define A_ATTR  (A_ATTRIBUTES)            /* standard UNIX attributes */
#endif

#define MAXSTRLEN  256
#define KEY_ESC    0x1b     /* Escape */

#define editstr(s,f)           (weditstr(stdscr,s,f))
#define mveditstr(y,x,s,f)     (move(y,x)==ERR?ERR:editstr(s,f))
#define mvweditstr(w,y,x,s,f)  (wmove(w,y,x)==ERR?ERR:weditstr(w,s,f))

#define inputbox(l,c)          (winputbox(stdscr,l,c))
#define mvinputbox(y,x,l,c)    (move(y,x)==ERR?w:inputbox(l,c))
#define mvwinputbox(w,y,x,l,c) (wmove(w,y,x)==ERR?w:winputbox(w,l,c))

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef void (*FUNC)(void);

typedef struct
{
  char *name; /* item label */
  FUNC  func; /* (pointer to) function */
  char *desc; /* function description */
} menu;

/****************************************************************************
 * Public Functin Prototypes
 ****************************************************************************/

void    clsbody(void);
int     bodylen(void);
WINDOW *bodywin(void);

void    rmerror(void);
void    rmstatus(void);

void    titlemsg(char *msg);
void    bodymsg(char *msg);
void    errormsg(char *msg);
void    statusmsg(char *msg);

bool    keypressed(void);
int     getkey(void);
int     waitforkey(void);

void    tui_exit(void);
void    startmenu(menu *mp, char *title);
void    domenu(const menu *mp);

int     weditstr(WINDOW *win, char *buf, int field);
WINDOW *winputbox(WINDOW *win, int nlines, int ncols);
int     getstrings(const char *desc[], char *buf[], int field);

#endif /* __APPS_EXAMPLES_PDCURSES_TUI_H */
