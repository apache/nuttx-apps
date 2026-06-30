/****************************************************************************
 * apps/games/NXDoom/src/w_file_stdc.c
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
 *  WAD I/O functions.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>

#include "m_misc.h"
#include "w_file.h"
#include "z_zone.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct
{
  wad_file_t wad;
  FILE *fstream;
} stdc_wad_file_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static wad_file_t *w_stdc_openfile(const char *path);
static void w_stdc_closefile(wad_file_t *wad);
static size_t w_stdc_read(wad_file_t *wad, unsigned int offset, void *buffer,
                          size_t buffer_len);

/****************************************************************************
 * Public Data
 ****************************************************************************/

wad_file_class_t stdc_wad_file =
{
  w_stdc_openfile,
  w_stdc_closefile,
  w_stdc_read,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static wad_file_t *w_stdc_openfile(const char *path)
{
  stdc_wad_file_t *result;
  FILE *fstream;

  fstream = fopen(path, "rb");

  if (fstream == NULL)
    {
      return NULL;
    }

  /* Create a new stdc_wad_file_t to hold the file handle. */

  result = z_malloc(sizeof(stdc_wad_file_t), PU_STATIC, 0);
  result->wad.file_class = &stdc_wad_file;
  result->wad.mapped = NULL;
  result->wad.length = m_file_length(fstream);
  result->wad.path = m_string_duplicate(path);
  result->fstream = fstream;

  return &result->wad;
}

static void w_stdc_closefile(wad_file_t *wad)
{
  stdc_wad_file_t *stdc_wad;

  stdc_wad = (stdc_wad_file_t *)wad;

  fclose(stdc_wad->fstream);
  z_free(stdc_wad);
}

/* Read data from the specified position in the file into the
 * provided buffer.  Returns the number of bytes read.
 */

static size_t w_stdc_read(wad_file_t *wad, unsigned int offset, void *buffer,
                          size_t buffer_len)
{
  stdc_wad_file_t *stdc_wad;
  size_t result;

  stdc_wad = (stdc_wad_file_t *)wad;

  /* Jump to the specified position in the file. */

  fseek(stdc_wad->fstream, offset, SEEK_SET);

  /* Read into the buffer. */

  result = fread(buffer, 1, buffer_len, stdc_wad->fstream);

  return result;
}
