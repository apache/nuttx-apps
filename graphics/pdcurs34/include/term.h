/****************************************************************************
 * apps/graphics/pdcurses/term.h
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

#ifndef __APPS_GRAPHICS_PDCURS34_INCLUDE_TERM_H
#define __APPS_GRAPHICS_PDCURS34_INCLUDE_TERM_H

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
  FAR const char *_termname;
} TERMINAL;

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifndef CONFIG_PDCURSES_MULTITHREAD
EXTERN TERMINAL *cur_term;
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

FAR void *pdc_alloc_terminal_ctx(void);
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
