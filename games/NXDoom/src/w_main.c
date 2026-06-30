/****************************************************************************
 * apps/games/NXDoom/src/w_main.c
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
 *     Common code to parse command line, identifying WAD files to load.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>

#include "config.h"
#include "d_iwad.h"
#include "i_glob.h"
#include "i_system.h"
#include "m_argv.h"
#include "w_main.h"
#include "w_merge.h"
#include "w_wad.h"
#include "z_zone.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct uniquelump
{
  gamemission_t mission;
  const char *lumpname;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Lump names that are unique to particular game types. This lets us check
 * the user is not trying to play with the wrong executable, eg.
 * chocolate-doom -iwad hexen.wad.
 */

static const struct uniquelump g_unique_lumps[] =
{
  {doom, "POSSA1"},
  {heretic, "IMPXA1"},
  {hexen, "ETTNA1"},
  {strife, "AGRDA1"},
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Parse the command line, merging WAD files that are sppecified.
 * Returns true if at least one file was added.
 */

boolean w_parse_command_line(void)
{
  boolean modifiedgame = false;
  int p;

  /* Merged PWADs are loaded first, because they are supposed to be
   * modified IWADs.
   */

  /* @arg <files>
   * @category mod
   *
   * Simulates the behavior of deutex's -merge option, merging a PWAD
   * into the main IWAD.  Multiple files may be specified.
   */

  p = m_check_parm_with_args("-merge", 1);

  if (p > 0)
    {
      for (p = p + 1; p < myargc && myargv[p][0] != '-'; ++p)
        {
          char *filename;

          modifiedgame = true;

          filename = d_try_find_wad_by_name(myargv[p]);

          printf(" merging %s\n", filename);
          w_merge_file(filename);
          free(filename);
        }
    }

  /* NWT-style merging: */

  /* NWT's -merge option: */

  /* @arg <files>
   * @category mod
   *
   * Simulates the behavior of NWT's -merge option.  Multiple files
   * may be specified.
   */

  p = m_check_parm_with_args("-nwtmerge", 1);

  if (p > 0)
    {
      for (p = p + 1; p < myargc && myargv[p][0] != '-'; ++p)
        {
          char *filename;

          modifiedgame = true;

          filename = d_try_find_wad_by_name(myargv[p]);

          printf(" performing NWT-style merge of %s\n", filename);
          w_nwt_dash_merge(filename);
          free(filename);
        }
    }

  /* Add flats */

  /* @arg <files>
   * @category mod
   *
   * Simulates the behavior of NWT's -af option, merging flats into
   * the main IWAD directory.  Multiple files may be specified.
   */

  p = m_check_parm_with_args("-af", 1);

  if (p > 0)
    {
      for (p = p + 1; p < myargc && myargv[p][0] != '-'; ++p)
        {
          char *filename;

          modifiedgame = true;

          filename = d_try_find_wad_by_name(myargv[p]);

          printf(" merging flats from %s\n", filename);
          w_nwt_merge_file(filename, W_NWT_MERGE_FLATS);
          free(filename);
        }
    }

  /* @arg <files>
   * @category mod
   *
   * Simulates the behavior of NWT's -as option, merging sprites
   * into the main IWAD directory.  Multiple files may be specified.
   */

  p = m_check_parm_with_args("-as", 1);

  if (p > 0)
    {
      for (p = p + 1; p < myargc && myargv[p][0] != '-'; ++p)
        {
          char *filename;

          modifiedgame = true;
          filename = d_try_find_wad_by_name(myargv[p]);

          printf(" merging sprites from %s\n", filename);
          w_nwt_merge_file(filename, W_NWT_MERGE_SPRITES);
          free(filename);
        }
    }

  /* @arg <files>
   * @category mod
   *
   * Equivalent to "-af <files> -as <files>".
   */

  p = m_check_parm_with_args("-aa", 1);

  if (p > 0)
    {
      for (p = p + 1; p < myargc && myargv[p][0] != '-'; ++p)
        {
          char *filename;

          modifiedgame = true;

          filename = d_try_find_wad_by_name(myargv[p]);

          printf(" merging sprites and flats from %s\n", filename);
          w_nwt_merge_file(filename,
                  W_NWT_MERGE_SPRITES | W_NWT_MERGE_FLATS);
          free(filename);
        }
    }

  /* @arg <files>
   * @vanilla
   *
   * Load the specified PWAD files.  Each succeeding argument is
   * treated as a PWAD file name until one starts with a dash or the
   * argument list is exhausted.
   */

  p = m_check_parm_with_args("-file", 1);
  if (p)
    {
      /* the params after p are wadfile/lump names,
       * until end of params or another - preceded param
       */

      modifiedgame = true; /* homebrew levels */
      while (++p != myargc && myargv[p][0] != '-')
        {
          char *filename;

          filename = d_try_find_wad_by_name(myargv[p]);

          printf(" adding %s\n", filename);
          w_add_file(filename);
          free(filename);
        }
    }

  return modifiedgame;
}

/* Load all WAD files from the given directory. */

void w_auto_load_wads(const char *path)
{
  glob_t *glob;
  const char *filename;

  glob = i_start_multi_glob(path, GLOB_FLAG_NOCASE | GLOB_FLAG_SORTED,
                            "*.wad", "*.lmp", NULL);
  for (; ; )
    {
      filename = i_next_glob(glob);
      if (filename == NULL)
        {
          break;
        }

      printf(" [autoload] merging %s\n", filename);
      w_merge_file(filename);
    }

  i_end_glob(glob);
}

void w_check_correct_iwad(gamemission_t mission)
{
  int i;
  lumpindex_t lumpnum;

  for (i = 0; i < arrlen(g_unique_lumps); ++i)
    {
      if (mission != g_unique_lumps[i].mission)
        {
          lumpnum = w_check_num_for_name(g_unique_lumps[i].lumpname);

          if (lumpnum >= 0)
            {
              i_error(
                  "\nYou are trying to use a %s IWAD file with "
                  "the %s%s binary.\nThis isn't going to work.\n"
                  "You probably want to use the %s%s binary.",
                  d_suggest_game_name(
                      g_unique_lumps[i].mission, indetermined),
                  PROGRAM_PREFIX, d_game_mission_string(mission),
                  PROGRAM_PREFIX,
                  d_game_mission_string(g_unique_lumps[i].mission));
            }
        }
    }
}
