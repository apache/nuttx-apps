/****************************************************************************
 * apps/games/NXDoom/src/deh_text.c
 *
 * SPDX-License-Identifer: GPLv2
 *
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
 * Parses Text substitution sections in dehacked files
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "doomtype.h"

#include "z_zone.h"

#include "deh_defs.h"
#include "deh_io.h"
#include "deh_main.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void deh_text_parse_line(deh_context_t *context, char *line,
                                void *tag);
static void *deh_text_start(deh_context_t *context, char *line);
static void *deh_text_start(deh_context_t *context, char *line);

/****************************************************************************
 * Public Data
 ****************************************************************************/

deh_section_t deh_section_text =
{
  "Text", NULL, deh_text_start, deh_text_parse_line, NULL, NULL,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Given a string length, find the maximum length of a string that can
 * replace it.
 */

static int txt_max_string_length(int len)
{
  /* Enough bytes for the string and the NUL terminator */

  len += 1;

  /* All strings in doom.exe are on 4-byte boundaries, so we may be able
   * to support a slightly longer string.
   * Extend up to the next 4-byte boundary
   */

  len += (4 - (len % 4)) % 4;

  /* Less one for the NUL terminator. */

  return len - 1;
}

static void *deh_text_start(deh_context_t *context, char *line)
{
  char *from_text;
  char *to_text;
  int fromlen;
  int to_len;
  int i;

  if (sscanf(line, "Text %i %i", &fromlen, &to_len) != 2)
    {
      deh_warning(context, "Parse error on section start");
      return NULL;
    }

  /* Only allow string replacements that are possible in Vanilla Doom.
   * Chocolate Doom is unforgiving!
   */

  if (!deh_allow_long_strings && to_len > txt_max_string_length(fromlen))
    {
      deh_error(context, "Replacement string is longer than the maximum "
                         "possible in doom.exe");
      return NULL;
    }

  from_text = malloc(fromlen + 1);
  to_text = malloc(to_len + 1);

  /* read in the "from" text */

  for (i = 0; i < fromlen; ++i)
    {
      from_text[i] = deh_get_char(context);
    }

  from_text[fromlen] = '\0';

  /* read in the "to" text */

  for (i = 0; i < to_len; ++i)
    {
      to_text[i] = deh_get_char(context);
    }

  to_text[to_len] = '\0';

  deh_add_string_replacement(from_text, to_text);

  free(from_text);
  free(to_text);

  return NULL;
}

static void deh_text_parse_line(deh_context_t *context, char *line,
        void *tag)
{
  /* not used */
}
