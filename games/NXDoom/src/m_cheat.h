/****************************************************************************
 * apps/games/NXDoom/src/m_cheat.h
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
 * DESCRIPTION:
 *  Cheat code checking.
 *
 ****************************************************************************/

#ifndef __M_CHEAT__
#define __M_CHEAT__

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* CHEAT SEQUENCE PACKAGE */

/* declaring a cheat */

#define CHEAT(value, parameters)                                             \
  {value, sizeof(value) - 1, parameters, 0, 0, ""}

#define MAX_CHEAT_LEN 25
#define MAX_CHEAT_PARAMS 5

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef struct
{
  /* settings for this cheat */

  char sequence[MAX_CHEAT_LEN];
  size_t sequence_len;
  int parameter_chars;

  /* state used during the game */

  size_t chars_read;
  int param_chars_read;
  char parameter_buf[MAX_CHEAT_PARAMS];
} cheatseq_t;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int cht_check_cheat(cheatseq_t *cht, char key);

void ch_get_param(cheatseq_t *cht, char *buffer);

#endif /* __M_CHEAT__ */
