/****************************************************************************
 * apps/examples/pdcurses/tui.c
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
 * Public Function Prototypes
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
