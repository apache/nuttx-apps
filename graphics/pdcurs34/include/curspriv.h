/****************************************************************************
 * apps/graphics/pdcurses/curspriv.h
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

#ifndef __APPS_GRAPHICS_PDCURS34_INCLUDE_CURSPRIV_H
#define __APPS_GRAPHICS_PDCURS34_INCLUDE_CURSPRIV_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include "graphics/curses.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Window properties */

#define _SUBWIN    0x01  /* Window is a subwindow */
#define _PAD       0x10  /* X/Open Pad. */
#define _SUBPAD    0x20  /* X/Open subpad. */

/* Miscellaneous */

#define _NO_CHANGE -1    /* Flags line edge unchanged */

#define _ECHAR     0x08  /* Erase char       (^H) */
#define _DWCHAR    0x17  /* Delete Word char (^W) */
#define _DLCHAR    0x15  /* Delete Line char (^U) */

#ifdef CONFIG_PDCURSES_DEBUG
#  define PDC_LOG(x) if (pdc_trace_on) PDC_debug x
#  define RCSID(x) static const char *rcsid = x;
#else
#  define PDC_LOG(x)
#  define RCSID(x)
#endif

/* Internal macros for attributes */

#ifndef max
# define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
# define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define DIVROUND(num, divisor) ((num) + ((divisor) >> 1)) / (divisor)

#define PDC_CLICK_PERIOD 150  /* time to wait for a click, if
                                 not set by mouseinterval() */

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

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifndef CONFIG_PDCURSES_MULTITHREAD
EXTERN WINDOW *pdc_lastscr;
EXTERN bool pdc_trace_on;   /* Tracing flag */
EXTERN bool pdc_color_started;
EXTERN unsigned long pdc_key_modifiers;
EXTERN MOUSE_STATUS pdc_mouse_status;
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void    PDC_beep(void);
bool    PDC_can_change_color(void);
int     PDC_color_content(short, short *, short *, short *);
bool    PDC_check_key(void);
int     PDC_curs_set(int);
void    PDC_flushinp(void);
int     PDC_get_columns(void);
int     PDC_get_cursor_mode(void);
int     PDC_get_key(void);
int     PDC_get_rows(void);
void    PDC_gotoyx(int, int);
int     PDC_init_color(short, short, short, short);
void    PDC_init_pair(short, short, short);
int     PDC_modifiers_set(void);
int     PDC_mouse_set(void);
void    PDC_napms(int);
int     PDC_pair_content(short, short *, short *);
void    PDC_reset_prog_mode(void);
void    PDC_reset_shell_mode(void);
int     PDC_resize_screen(int, int);
void    PDC_restore_screen_mode(int);
void    PDC_save_screen_mode(int);
void    PDC_scr_close(void);
void    PDC_scr_free(void);
int     PDC_scr_open(int, char **);
void    PDC_set_keyboard_binary(bool);
void    PDC_transform_line(int, int, int, const chtype *);
const char *PDC_sysname(void);

/* Internal cross-module functions */

void    PDC_init_atrtab(void);
WINDOW *PDC_makelines(WINDOW *);
WINDOW *PDC_makenew(int, int, int, int);
int     PDC_mouse_in_slk(int, int);
void    PDC_slk_free(void);
void    PDC_slk_initialize(void);
void    PDC_sync(WINDOW *);

#ifdef CONFIG_PDCURSES_WIDE
int     PDC_mbtowc(wchar_t *, const char *, size_t);
size_t  PDC_mbstowcs(wchar_t *, const char *, size_t);
size_t  PDC_wcstombs(char *, const wchar_t *, size_t);
#endif

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __APPS_GRAPHICS_PDCURS34_INCLUDE_CURSPRIV_H*/
