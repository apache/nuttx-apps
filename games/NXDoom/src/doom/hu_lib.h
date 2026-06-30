/****************************************************************************
 * apps/games/NXDoom/src/doom/hu_lib.h
 *
 * SPDX-License-Identifer: GPLv2
 *
 * Copyright(C) 1993-1996 Id Software, Inc.
 * Copyright(C) 2005-2014 Simon Howard
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * DESCRIPTION: none
 *
 ****************************************************************************/

#ifndef __HULIB__
#define __HULIB__

/****************************************************************************
 * Included Files
 ****************************************************************************/

/* We are referring to patches. */

#include "r_defs.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* font stuff */

#define HU_CHARERASE KEY_BACKSPACE

#define HU_MAXLINES 4
#define HU_MAXLINELENGTH 80

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Typedefs of widgets */

/* Text Line widget
 *  (parent of Scrolling Text and Input Text widgets)
 */

typedef struct
{
  /* left-justified position of scrolling text window */

  int x;
  int y;

  patch_t **f;                  /* font */
  int sc;                       /* start character */
  char l[HU_MAXLINELENGTH + 1]; /* line of text */
  int len;                      /* current line length */

  /* whether this line needs to be updated */

  int needsupdate;
} hu_textline_t;

/* Scrolling Text window widget (child of Text Line widget) */

typedef struct
{
  hu_textline_t l[HU_MAXLINES]; /* text lines to draw */
  int h;                        /* height in lines */
  int cl;                       /* current line number */

  /* pointer to boolean stating whether to update window */

  boolean *on;
  boolean laston; /* last value of *->on. */
} hu_stext_t;

/* Input Text Line widget (child of Text Line widget) */

typedef struct
{
  hu_textline_t l; /* text line to input on */

  /* left margin past which I am not to delete characters */

  int lm;

  /* pointer to boolean stating whether to update window */

  boolean *on;
  boolean laston; /* last value of *->on; */
} hu_itext_t;

/* Widget creation, access, and update routines */

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* textline code */

/* clear a line of text */

void hu_lib_clear_text(hu_textline_t *t);

void hu_lib_init_text_line(hu_textline_t *t, int x, int y, patch_t **f,
                           int sc);

/* returns success */

boolean hu_lib_add_char_to_text_line(hu_textline_t *t, char ch);

/* returns success */

boolean hu_lib_del_char_from_text_line(hu_textline_t *t);

/* draws tline */

void hu_lib_draw_text_line(hu_textline_t *l, boolean drawcursor);

/* erases text line */

void hu_lib_erase_text_line(hu_textline_t *l);

/* Scrolling Text window widget routines */

void hu_lib_init_stext(hu_stext_t *s, int x, int y, int h, patch_t **font,
                       int startchar, boolean *on);

/* add a new line */

void hu_lib_add_line_to_stext(hu_stext_t *s);

/* ? */

void hu_lib_add_messsage_to_stext(hu_stext_t *s, const char *prefix,
                                  const char *msg);

/* draws stext */

void hu_lib_draw_stext(hu_stext_t *s);

/* erases all stext lines */

void hu_lib_erase_stext(hu_stext_t *s);

/* Input Text Line widget routines */

void hu_lib_init_itext(hu_itext_t *it, int x, int y, patch_t **font,
                       int startchar, boolean *on);

/* enforces left margin */

void hu_lib_del_char_from_itext(hu_itext_t *it);

/* resets line and left margin */

void hu_lib_reset_itext(hu_itext_t *it);

/* whether eaten */

boolean hu_lib_key_in_itext(hu_itext_t *it, unsigned char ch);

void hu_lib_draw_itext(hu_itext_t *it);

/* erases all itext lines */

void hu_lib_erase_itext(hu_itext_t *it);

#endif /* __HULIB__ */
