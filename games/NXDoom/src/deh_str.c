/****************************************************************************
 * apps/games/NXDoom/src/deh_str.c
 *
 * SPDX-License-Identifier: GPLv2
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
 *
 * Parses Text substitution sections in dehacked files
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "deh_str.h"
#include "doomtype.h"
#include "m_misc.h"

#include "z_zone.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct
{
  char *from_text;
  char *to_text;
} deh_substitution_t;

typedef enum
{
  FORMAT_ARG_INVALID,
  FORMAT_ARG_INT,
  FORMAT_ARG_FLOAT,
  FORMAT_ARG_CHAR,
  FORMAT_ARG_STRING,
  FORMAT_ARG_PTR,
  FORMAT_ARG_SAVE_POS
} format_arg_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static deh_substitution_t **hash_table = NULL;
static int hash_table_entries;
static int hash_table_length = -1;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* This is the algorithm used by glib */

static unsigned int strhash(const char *s)
{
  const char *p = s;
  unsigned int h = *p;

  if (h)
    {
      for (p += 1; *p; p++)
        h = (h << 5) - h + *p;
    }

  return h;
}

static deh_substitution_t *substitution_for_string(const char *s)
{
  int entry;

  /* Fallback if we have not initialized the hash table yet */

  if (hash_table_length < 0) return NULL;

  entry = strhash(s) % hash_table_length;

  while (hash_table[entry] != NULL)
    {
      if (!strcmp(hash_table[entry]->from_text, s))
        {
          return hash_table[entry]; /* substitution found! */
        }

      entry = (entry + 1) % hash_table_length;
    }

  /* no substitution found */

  return NULL;
}

static void init_hash_table(void)
{
  /* init hash table */

  hash_table_entries = 0;
  hash_table_length = 16;
  hash_table = z_malloc(sizeof(deh_substitution_t *) * hash_table_length,
                        PU_STATIC, NULL);
  memset(hash_table, 0, sizeof(deh_substitution_t *) * hash_table_length);
}

static void deh_add_to_hashtable(deh_substitution_t *sub);

static void increase_hashtable(void)
{
  deh_substitution_t **old_table;
  int old_table_length;
  int i;

  /* save the old table */

  old_table = hash_table;
  old_table_length = hash_table_length;

  /* double the size */

  hash_table_length *= 2;
  hash_table = z_malloc(sizeof(deh_substitution_t *) * hash_table_length,
                        PU_STATIC, NULL);
  memset(hash_table, 0, sizeof(deh_substitution_t *) * hash_table_length);

  /* go through the old table and insert all the old entries */

  for (i = 0; i < old_table_length; ++i)
    {
      if (old_table[i] != NULL)
        {
          deh_add_to_hashtable(old_table[i]);
        }
    }

  /* free the old table */

  z_free(old_table);
}

static void deh_add_to_hashtable(deh_substitution_t *sub)
{
  int entry;

  /* if the hash table is more than 60% full, increase its size */

  if ((hash_table_entries * 10) / hash_table_length > 6)
    {
      increase_hashtable();
    }

  /* find where to insert it */

  entry = strhash(sub->from_text) % hash_table_length;

  while (hash_table[entry] != NULL)
    {
      entry = (entry + 1) % hash_table_length;
    }

  hash_table[entry] = sub;
  ++hash_table_entries;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void deh_add_string_replacement(const char *from_text, const char *to_text)
{
  deh_substitution_t *sub;
  size_t len;

  /* Initialize the hash table if this is the first time */

  if (hash_table_length < 0)
    {
      init_hash_table();
    }

  /* Check to see if there is an existing substitution already in place. */

  sub = substitution_for_string(from_text);

  if (sub != NULL)
    {
      z_free(sub->to_text);

      len = strlen(to_text) + 1;
      sub->to_text = z_malloc(len, PU_STATIC, NULL);
      memcpy(sub->to_text, to_text, len);
    }
  else
    {
      /* We need to allocate a new substitution. */

      sub = z_malloc(sizeof(*sub), PU_STATIC, 0);

      /* We need to create our own duplicates of the provided strings. */

      len = strlen(from_text) + 1;
      sub->from_text = z_malloc(len, PU_STATIC, NULL);
      memcpy(sub->from_text, from_text, len);

      len = strlen(to_text) + 1;
      sub->to_text = z_malloc(len, PU_STATIC, NULL);
      memcpy(sub->to_text, to_text, len);

      deh_add_to_hashtable(sub);
    }
}
