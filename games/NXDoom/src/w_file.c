/****************************************************************************
 * apps/games/NXDoom/src/w_file.c
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
 *  WAD I/O functions.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>

#include "config.h"

#include "doomtype.h"
#include "m_argv.h"

#include "w_file.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static wad_file_class_t *wad_file_classes[] =
{
  &stdc_wad_file,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

wad_file_t *w_open_file(const char *path)
{
  wad_file_t *result;
  int i;

  /* @category obscure
   *
   * Use the OS's virtual memory subsystem to map WAD files
   * directly into memory.
   */

  if (!m_check_parm("-mmap"))
    {
      return stdc_wad_file.open_file(path);
    }

  /* Try all classes in order until we find one that works */

  result = NULL;

  for (i = 0; i < arrlen(wad_file_classes); ++i)
    {
      result = wad_file_classes[i]->open_file(path);

      if (result != NULL)
        {
          break;
        }
    }

  return result;
}

void w_close_file(wad_file_t *wad)
{
  wad->file_class->close_file(wad);
}

size_t w_read(wad_file_t *wad, unsigned int offset, void *buffer,
              size_t buffer_len)
{
  return wad->file_class->read(wad, offset, buffer, buffer_len);
}
