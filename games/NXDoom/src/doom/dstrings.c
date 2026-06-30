/****************************************************************************
 * apps/games/NXDoom/src/doom/dstrings.c
 *
 * SPDX-License-Identifier: GPLv2
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
 * DESCRIPTION:
 *  Globally defined strings.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "dstrings.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

const char *doom1_endmsg[] =
{
  DOOM1_ENDMSG
};

const char *doom2_endmsg[] =
{
  DOOM2_ENDMSG
};

/* UNUSED messages included in the source release */

#if 0
char *endmsg[] =
{
  /* DOOM1 */

  QUITMSG,

  /* FinalDOOM? */

  "fuck you, pussy!\nget the fuck out!",
  "you quit and i'll jizz\nin your cystholes!",
  "if you leave, i'll make\nthe lord drink my jizz.",
  "hey, ron! can we say\n'fuck' in the game?",
  "i'd leave: this is just\nmore monsters and levels.\nwhat a load.",
  "suck it down, asshole!\nyou're a fucking wimp!",
  "don't quit now! we're \nstill spending your money!",

  /* Internal debug. Different style, too. */

  "THIS IS NO MESSAGE!\nPage intentionally left blank.",
};
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/
