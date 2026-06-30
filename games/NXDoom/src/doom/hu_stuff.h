/****************************************************************************
 * apps/games/NXDoom/src/doom/hu_stuff.h
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
 * DESCRIPTION: Head up display
 *
 ****************************************************************************/

#ifndef __HU_STUFF_H__
#define __HU_STUFF_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "d_event.h"
#include "v_patch.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Globally visible constants. */

#define HU_FONTSTART '!' /* the first font characters */
#define HU_FONTEND '_'   /* the last font characters */

/* Calculate # of glyphs in font. */

#define HU_FONTSIZE (HU_FONTEND - HU_FONTSTART + 1)

#define HU_BROADCAST 5

#define HU_MSGX 0
#define HU_MSGY 0
#define HU_MSGWIDTH 64 /* in characters */
#define HU_MSGHEIGHT 1 /* in lines */

#define HU_MSGTIMEOUT (4 * TICRATE)

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern const char *player_names[4];
extern char *chat_macros[10];

extern patch_t *hu_font[HU_FONTSIZE];

extern boolean message_dontfuckwithme;

extern boolean chat_on;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* HEADS UP TEXT */

void hu_init(void);
void hu_start(void);

boolean hu_responder(event_t *ev);

void hu_ticker(void);
void hu_drawer(void);
char hu_dequeue_chat_char(void);
void hu_erase(void);

#endif /* __HU_STUFF_H__ */
