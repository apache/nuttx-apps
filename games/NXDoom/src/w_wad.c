/****************************************************************************
 * apps/games/NXDoom/src/w_wad.c
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
 *  Handles WAD file header, directory, lump I/O.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "doomtype.h"

#include "i_swap.h"
#include "i_system.h"
#include "i_video.h"
#include "m_misc.h"
#include "v_diskicon.h"
#include "z_zone.h"

#include "w_wad.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

begin_packed_struct struct wadinfo_t
{
  /* Should be "IWAD" or "PWAD". */

  char identification[4];
  int numlumps;
  int infotableofs;
} end_packed_struct;

typedef struct wadinfo_t wadinfo_t;

begin_packed_struct struct filelump_t
{
  int filepos;
  int size;
  char name[8];
} end_packed_struct;

typedef struct filelump_t filelump_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Hash table for fast lookups */

static lumpindex_t *lumphash;

/* Variables for the reload hack: filename of the PWAD to reload, and the
 * lumps from WADs before the reload file, so we can resent numlumps and
 * load the file again.
 */

static wad_file_t *reloadhandle = NULL;
static lumpinfo_t *reloadlumps = NULL;
static char *reloadname = NULL;
static int reloadlump = -1;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Location of each lump on disk. */

lumpinfo_t **lumpinfo;
unsigned int numlumps = 0;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Hash function used for lump names. */

unsigned int w_lump_name_hash(const char *s)
{
  /* This is the djb2 string hash function, modded to work on strings
   * that have a maximum length of 8.
   */

  unsigned int result = 5381;
  unsigned int i;

  for (i = 0; i < 8 && s[i] != '\0'; ++i)
    {
      result = ((result << 5) ^ result) ^ toupper(s[i]);
    }

  return result;
}

/* LUMP BASED ROUTINES. */

/* w_add_file
 *
 * All files are optional, but at least one file must be found (PWAD, if all
 * required lumps are present).
 *
 * Files with a .wad extension are wadlink files with multiple lumps.
 *
 * Other files are single lumps with the base filename for the lump name.
 */

wad_file_t *w_add_file(const char *filename)
{
  wadinfo_t header;
  lumpindex_t i;
  wad_file_t *wad_file;
  int length;
  int startlump;
  filelump_t *fileinfo;
  filelump_t *filerover;
  lumpinfo_t *filelumps;
  int numfilelumps;

  /* If the filename begins with a ~, it indicates that we should use the
   * reload hack.
   */

  if (filename[0] == '~')
    {
      if (reloadname != NULL)
        {
          i_error("Prefixing a WAD filename with '~' indicates that the "
                  "WAD should be reloaded\n"
                  "on each level restart, for use by level authors for "
                  "rapid development. You\n"
                  "can only reload one WAD file, and it must be the last "
                  "file in the -file list.");
        }

      reloadname = strdup(filename);
      reloadlump = numlumps;
      ++filename;
    }

  /* Open the file and add to directory */

  wad_file = w_open_file(filename);

  if (wad_file == NULL)
    {
      printf(" couldn't open %s\n", filename);
      return NULL;
    }

  if (strcasecmp(filename + strlen(filename) - 3, "wad"))
    {
      /* single lump file */

      /* fraggle: Swap the filepos and size here.  The WAD directory
       * parsing code expects a little-endian directory, so will swap
       * them back.  Effectively we're constructing a "fake WAD directory"
       * here, as it would appear on disk.
       */

      fileinfo = z_malloc(sizeof(filelump_t), PU_STATIC, 0);
      fileinfo->filepos = LONG(0);
      fileinfo->size = LONG(wad_file->length);

      /* Name the lump after the base of the filename (without the
       * extension).
       */

      m_extract_file_base(filename, fileinfo->name);
      numfilelumps = 1;
    }
  else
    {
      /* WAD file */

      w_read(wad_file, 0, &header, sizeof(header));

      if (strncmp(header.identification, "IWAD", 4))
        {
          /* Homebrew levels? */

          if (strncmp(header.identification, "PWAD", 4))
            {
              w_close_file(wad_file);
              i_error("Wad file %s doesn't have IWAD "
                      "or PWAD id\n",
                      filename);
            }

          /* ???modifiedgame = true; */
        }

      header.numlumps = LONG(header.numlumps);

      /* Vanilla Doom doesn't like WADs with more than 4046 lumps
       * https://www.doomworld.com/vb/post/1010985
       */

      if (!strncmp(header.identification, "PWAD", 4) &&
          header.numlumps > 4046)
        {
          w_close_file(wad_file);
          i_error("Error: Vanilla limit for lumps in a WAD is 4046, "
                  "PWAD %s has %d",
                  filename, header.numlumps);
        }

      header.infotableofs = LONG(header.infotableofs);
      length = header.numlumps * sizeof(filelump_t);
      fileinfo = z_malloc(length, PU_STATIC, 0);

      w_read(wad_file, header.infotableofs, fileinfo, length);
      numfilelumps = header.numlumps;
    }

  /* Increase size of numlumps array to accommodate the new file. */

  filelumps = calloc(numfilelumps, sizeof(lumpinfo_t));
  if (filelumps == NULL)
    {
      w_close_file(wad_file);
      i_error("Failed to allocate array for lumps from new file.");
    }

  startlump = numlumps;
  numlumps += numfilelumps;
  lumpinfo = i_realloc(lumpinfo, numlumps * sizeof(lumpinfo_t *));
  filerover = fileinfo;

  for (i = startlump; i < numlumps; ++i)
    {
      lumpinfo_t *lump_p = &filelumps[i - startlump];
      lump_p->wad_file = wad_file;
      lump_p->position = LONG(filerover->filepos);
      lump_p->size = LONG(filerover->size);
      lump_p->cache = NULL;
      strncpy(lump_p->name, filerover->name, 8);
      lumpinfo[i] = lump_p;

      ++filerover;
    }

  z_free(fileinfo);

  if (lumphash != NULL)
    {
      z_free(lumphash);
      lumphash = NULL;
    }

  /* If this is the reload file, we need to save some details about the
   * file so that we can close it later on when we do a reload.
   */

  if (reloadname)
    {
      reloadhandle = wad_file;
      reloadlumps = filelumps;
    }

  return wad_file;
}

/* w_check_num_for_name
 * Returns -1 if name not found.
 */

lumpindex_t w_check_num_for_name(const char *name)
{
  lumpindex_t i;

  /* Do we have a hash table yet? */

  if (lumphash != NULL)
    {
      int hash;

      /* We do! Excellent. */

      hash = w_lump_name_hash(name) % numlumps;

      for (i = lumphash[hash]; i != -1; i = lumpinfo[i]->next)
        {
          if (!strncasecmp(lumpinfo[i]->name, name, 8))
            {
              return i;
            }
        }
    }
  else
    {
      /* We don't have a hash table generate yet. Linear search :-(
       *
       * scan backwards so patch lump files take precedence
       */

      for (i = numlumps - 1; i >= 0; --i)
        {
          if (!strncasecmp(lumpinfo[i]->name, name, 8))
            {
              return i;
            }
        }
    }

  /* TFB. Not found. */

  return -1;
}

/* w_get_num_for_name
 * Calls w_check_num_for_name, but bombs out if not found.
 */

lumpindex_t w_get_num_for_name(const char *name)
{
  lumpindex_t i;

  i = w_check_num_for_name(name);

  if (i < 0)
    {
      i_error("w_get_num_for_name: %s not found!", name);
    }

  return i;
}

/* w_lump_length
 * Returns the buffer size needed to load the given lump.
 */

int w_lump_length(lumpindex_t lump)
{
  if (lump >= numlumps)
    {
      i_error("w_lump_length: %i >= numlumps", lump);
    }

  return lumpinfo[lump]->size;
}

/* w_read_lump
 * Loads the lump into the given buffer,
 *  which must be >= w_lump_length().
 */

void w_read_lump(lumpindex_t lump, void *dest)
{
  int c;
  lumpinfo_t *l;

  if (lump >= numlumps)
    {
      i_error("w_read_lump: %i >= numlumps", lump);
    }

  l = lumpinfo[lump];

  v_begin_read(l->size);

  c = w_read(l->wad_file, l->position, dest, l->size);

  if (c < l->size)
    {
      i_error("w_read_lump: only read %i of %i on lump %i",
              c, l->size, lump);
    }
}

/* w_cache_lump_num
 *
 * Load a lump into memory and return a pointer to a buffer containing
 * the lump data.
 *
 * 'tag' is the type of zone memory buffer to allocate for the lump
 * (usually PU_STATIC or PU_CACHE).  If the lump is loaded as
 * PU_STATIC, it should be released back using w_release_lump_num
 * when no longer needed (do not use z_change_tag).
 */

void *w_cache_lump_num(lumpindex_t lumpnum, int tag)
{
  byte *result;
  lumpinfo_t *lump;

  if ((unsigned)lumpnum >= numlumps)
    {
      i_error("w_cache_lump_num: %i >= numlumps", lumpnum);
    }

  lump = lumpinfo[lumpnum];

  /* Get the pointer to return.  If the lump is in a memory-mapped
   * file, we can just return a pointer to within the memory-mapped
   * region.  If the lump is in an ordinary file, we may already
   * have it cached; otherwise, load it into memory.
   */

  if (lump->wad_file->mapped != NULL)
    {
      /* Memory mapped file, return from the mmapped region. */

      result = lump->wad_file->mapped + lump->position;
    }
  else if (lump->cache != NULL)
    {
      /* Already cached, so just switch the zone tag. */

      result = lump->cache;
      z_change_tag(lump->cache, tag);
    }
  else
    {
      /* Not yet loaded, so load it now */

      lump->cache = z_malloc(w_lump_length(lumpnum), tag, &lump->cache);
      w_read_lump(lumpnum, lump->cache);
      result = lump->cache;
    }

  return result;
}

/* w_cache_lump_name */

void *w_cache_lump_name(const char *name, int tag)
{
  return w_cache_lump_num(w_get_num_for_name(name), tag);
}

/* Release a lump back to the cache, so that it can be reused later
 * without having to read from disk again, or alternatively, discarded
 * if we run out of memory.
 *
 * Back in Vanilla Doom, this was just done using z_change_tag
 * directly, but now that we have WAD mmap, things are a bit more
 * complicated ...
 */

void w_release_lump_num(lumpindex_t lumpnum)
{
  lumpinfo_t *lump;

  if ((unsigned)lumpnum >= numlumps)
    {
      i_error("w_release_lump_num: %i >= numlumps", lumpnum);
    }

  lump = lumpinfo[lumpnum];

  if (lump->wad_file->mapped != NULL)
    {
      /* Memory-mapped file, so nothing needs to be done here. */
    }
  else
    {
      z_change_tag(lump->cache, PU_CACHE);
    }
}

void w_release_lump_name(const char *name)
{
  w_release_lump_num(w_get_num_for_name(name));
}

#if 0

/* w_profile */

int info[2500][10];
int profilecount;

void w_profile(void)
{
  int i;
  memblock_t *block;
  void *ptr;
  char ch;
  FILE *f;
  int j;
  char name[9];

  for (i = 0; i < numlumps; i++)
    {
      ptr = lumpinfo[i].cache;
      if (!ptr)
        {
          ch = ' ';
          continue;
        }
      else
        {
          block = (memblock_t *)((byte *)ptr - sizeof(memblock_t));
          if (block->tag < PU_PURGELEVEL)
            ch = 'S';
          else
            ch = 'P';
        }

      info[i][profilecount] = ch;
    }

  profilecount++;

  f = fopen("waddump.txt", "w");
  name[8] = 0;

  for (i = 0; i < numlumps; i++)
    {
      memcpy(name, lumpinfo[i].name, 8);

      for (j = 0; j < 8; j++)
        {
          if (!name[j])
            {
              break;
            }
        }

      for (; j < 8; j++)
        {
          name[j] = ' ';
        }

      fprintf(f, "%s ", name);

      for (j = 0; j < profilecount; j++)
        {
          fprintf(f, "    %c", info[i][j]);
        }

      fprintf(f, "\n");
    }

  fclose(f);
}
#endif

/* Generate a hash table for fast lookups */

void w_generate_hash_table(void)
{
  lumpindex_t i;

  /* Free the old hash table, if there is one: */

  if (lumphash != NULL)
    {
      z_free(lumphash);
    }

  /* Generate hash table */

  if (numlumps > 0)
    {
      lumphash = z_malloc(sizeof(lumpindex_t) * numlumps, PU_STATIC, NULL);

      for (i = 0; i < numlumps; ++i)
        {
          lumphash[i] = -1;
        }

      for (i = 0; i < numlumps; ++i)
        {
          unsigned int hash;

          hash = w_lump_name_hash(lumpinfo[i]->name) % numlumps;

          /* Hook into the hash table */

          lumpinfo[i]->next = lumphash[hash];
          lumphash[hash] = i;
        }
    }

  /* All done! */
}

/* The Doom reload hack. The idea here is that if you give a WAD file to
 * -file prefixed with the ~ hack, that WAD file will be reloaded each time a
 * new level is loaded. This lets you use a level editor in parallel and make
 * incremental changes to the level you're working on without having to
 * restart the game after every change. But: the reload feature is a fragile
 * hack...
 */

void w_reload(void)
{
  char *filename;
  lumpindex_t i;

  if (reloadname == NULL)
    {
      return;
    }

  /* We must free any lumps being cached from the PWAD we're about to reload:
   */

  for (i = reloadlump; i < numlumps; ++i)
    {
      if (lumpinfo[i]->cache != NULL)
        {
          z_free(lumpinfo[i]->cache);
        }
    }

  /* Reset numlumps to remove the reload WAD file: */

  numlumps = reloadlump;

  /* Now reload the WAD file. */

  filename = reloadname;

  w_close_file(reloadhandle);
  free(reloadlumps);

  reloadname = NULL;
  reloadlump = -1;
  reloadhandle = NULL;
  w_add_file(filename);
  free(filename);

  /* The WAD directory has changed, so we have to regenerate the
   * fast lookup hashtable:
   */

  w_generate_hash_table();
}

const char *w_wad_name_for_lump(const lumpinfo_t *lump)
{
  return m_base_name(lump->wad_file->path);
}

boolean w_is_iwad_lump(const lumpinfo_t *lump)
{
  return lump->wad_file == lumpinfo[0]->wad_file;
}
