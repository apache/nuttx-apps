/****************************************************************************
 * apps/games/NXDoom/src/doom/deh_ptr.c
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
 * Parses Action Pointer entries in dehacked files
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "doomtype.h"
#include "info.h"

#include "deh_defs.h"
#include "deh_io.h"
#include "deh_main.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void deh_pointer_init(void);
static void *deh_pointer_start(deh_context_t *context, char *line);
static void deh_pointer_parse_line(deh_context_t *context, char *line,
                                 void *tag);
static void deh_pointer_sha1_sum(SHA1_CTX *context);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static actionf_t codeptrs[NUMSTATES];

/****************************************************************************
 * Public Data
 ****************************************************************************/

deh_section_t deh_section_pointer =
{
  "Pointer",
  deh_pointer_init,
  deh_pointer_start,
  deh_pointer_parse_line,
  NULL,
  deh_pointer_sha1_sum,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int code_pointer_index(actionf_t *ptr)
{
  int i;

  for (i = 0; i < NUMSTATES; ++i)
    {
      if (!memcmp(&codeptrs[i], ptr, sizeof(actionf_t)))
        {
          return i;
        }
    }

  return -1;
}

static void deh_pointer_init(void)
{
  int i;

  /* Initialize list of dehacked pointers */

  for (i = 0; i < NUMSTATES; ++i)
    codeptrs[i] = states[i].action;
}

static void *deh_pointer_start(deh_context_t *context, char *line)
{
  int frame_number = 0;

  /* FIXME: can the third argument here be something other than "Frame"
   * or are we ok?
   */

  if (sscanf(line, "Pointer %*i (%*s %i)", &frame_number) != 1)
    {
      deh_warning(context, "Parse error on section start");
      return NULL;
    }

  if (frame_number < 0 || frame_number >= NUMSTATES)
    {
      deh_warning(context, "Invalid frame number: %i", frame_number);
      return NULL;
    }

  return &states[frame_number];
}

static void deh_pointer_parse_line(deh_context_t *context, char *line,
                                 void *tag)
{
  state_t *state;
  char *variable_name;
  char *value;
  int ivalue;

  if (tag == NULL) return;

  state = (state_t *)tag;

  /* Parse the assignment */

  if (!deh_parse_assignment(line, &variable_name, &value))
    {
      /* Failed to parse */

      deh_warning(context, "Failed to parse assignment");
      return;
    }

  /* all values are integers */

  ivalue = atoi(value);

  /* set the appropriate field */

  if (!strcasecmp(variable_name, "Codep frame"))
    {
      if (ivalue < 0 || ivalue >= NUMSTATES)
        {
          deh_warning(context, "Invalid state '%i'", ivalue);
        }
      else
        {
          state->action = codeptrs[ivalue];
        }
    }
  else
    {
      deh_warning(context, "Unknown variable name '%s'", variable_name);
    }
}

static void deh_pointer_sha1_sum(SHA1_CTX *context)
{
  int i;

  for (i = 0; i < NUMSTATES; ++i)
    {
      sha1_updateint32(context, code_pointer_index(&states[i].action));
    }
}
