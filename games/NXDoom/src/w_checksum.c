/****************************************************************************
 * apps/games/NXDoom/src/w_checksum.c
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
 *   Generate a checksum of the WAD directory.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "i_system.h"
#include "m_misc.h"
#include "sha1.h"
#include "w_checksum.h"
#include "w_wad.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static wad_file_t **g_open_wadfiles = NULL;
static int g_num_open_wadfiles = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int get_file_number(wad_file_t *handle)
{
  int i;
  int result;

  for (i = 0; i < g_num_open_wadfiles; ++i)
    {
      if (g_open_wadfiles[i] == handle)
        {
          return i;
        }
    }

  /* Not found in list.  This is a new file we haven't seen yet.
   * Allocate another slot for this file.
   */

  g_open_wadfiles = i_realloc(g_open_wadfiles,
          sizeof(wad_file_t *) * (g_num_open_wadfiles + 1));
  g_open_wadfiles[g_num_open_wadfiles] = handle;

  result = g_num_open_wadfiles;
  ++g_num_open_wadfiles;

  return result;
}

static void checksum_add_lump(SHA1_CTX *sha1_context, lumpinfo_t *lump)
{
  char buf[9];
  int fileno;

  fileno = get_file_number(lump->wad_file);

  m_str_copy(buf, lump->name, sizeof(buf));
  sha1_updatestring(sha1_context, buf);
  sha1_updateint32(sha1_context, fileno);
  sha1_updateint32(sha1_context, lump->position);
  sha1_updateint32(sha1_context, lump->size);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void w_checksum(sha1_digest_t digest)
{
  SHA1_CTX sha1_context;
  unsigned int i;

  sha1init(&sha1_context);

  g_num_open_wadfiles = 0;

  /* Go through each entry in the WAD directory, adding information
   * about each entry to the SHA1 hash.
   */

  for (i = 0; i < numlumps; ++i)
    {
      checksum_add_lump(&sha1_context, lumpinfo[i]);
    }

  sha1final(digest, &sha1_context);
}
