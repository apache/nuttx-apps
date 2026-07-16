/****************************************************************************
 * apps/games/NXDoom/src/d_iwad.c
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
 * DESCRIPTION:
 *   Search for and locate an IWAD file, and initialize according
 *   to the IWAD type.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "d_iwad.h"
#include "deh_str.h"
#include "doomkeys.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_config.h"
#include "m_misc.h"
#include "w_wad.h"
#include "z_zone.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Array of locations to search for IWAD files
 *
 * "128 IWAD search directories should be enough for anybody".
 */

#define MAX_IWAD_DIRS 128

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const iwad_t g_iwads[] =
{
  {"doom2.wad", doom2, commercial, "Doom II"},
  {"plutonia.wad", pack_plut, commercial, "Final Doom: Plutonia Experiment"},
  {"tnt.wad", pack_tnt, commercial, "Final Doom: TNT: Evilution"},
  {"doom.wad", doom, retail, "Doom"},
  {"doom1.wad", doom, shareware, "Doom Shareware"},
  {"doom2f.wad", doom2, commercial, "Doom II: L'Enfer sur Terre"},
  {"chex.wad", pack_chex, retail, "Chex Quest"},
  {"hacx.wad", pack_hacx, commercial, "Hacx"},
  {"freedoom2.wad", doom2, commercial, "Freedoom: Phase 2"},
  {"freedoom1.wad", doom, retail, "Freedoom: Phase 1"},
  {"freedm.wad", doom2, commercial, "FreeDM"},
  {"heretic.wad", heretic, retail, "Heretic"},
  {"heretic1.wad", heretic, shareware, "Heretic Shareware"},
  {"hexen.wad", hexen, commercial, "Hexen"},
  {"strife1.wad", strife, commercial, "Strife"},
};

static boolean iwad_dirs_built = false;
static const char *iwad_dirs[MAX_IWAD_DIRS];
static int num_iwad_dirs = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void add_iwad_dir(const char *dir)
{
  if (num_iwad_dirs < MAX_IWAD_DIRS)
    {
      iwad_dirs[num_iwad_dirs] = dir;
      ++num_iwad_dirs;
    }
}

/* Returns true if the specified path is a path to a file
 * of the specified name.
 */

static boolean dir_is_file(const char *path, const char *filename)
{
  return strchr(path, DIR_SEPARATOR) != NULL &&
         !strcasecmp(m_base_name(path), filename);
}

/* Check if the specified directory contains the specified IWAD
 * file, returning the full path to the IWAD if found, or NULL
 * if not found.
 */

static char *check_dir_has_iwad(const char *dir, const char *iwadname)
{
  char *filename;
  char *probe;

  /* As a special case, the "directory" may refer directly to an
   * IWAD file if the path comes from DOOMWADDIR or DOOMWADPATH.
   */

  probe = m_file_case_exists(dir);
  if (dir_is_file(dir, iwadname) && probe != NULL)
    {
      return probe;
    }

  /* Construct the full path to the IWAD if it is located in
   * this directory, and check if it exists.
   */

  if (!strcmp(dir, "."))
    {
      filename = m_string_duplicate(iwadname);
    }
  else
    {
      filename = m_string_join(dir, DIR_SEPARATOR_S, iwadname, NULL);
    }

  free(probe);
  probe = m_file_case_exists(filename);
  free(filename);

  if (probe != NULL)
    {
      return probe;
    }

  return NULL;
}

/* Search a directory to try to find an IWAD
 * Returns the location of the IWAD if found, otherwise NULL.
 */

static char *search_dir_for_iwad(const char *dir, int mask,
                                    gamemission_t *mission)
{
  char *filename;
  size_t i;

  for (i = 0; i < arrlen(g_iwads); ++i)
    {
      if (((1 << g_iwads[i].mission) & mask) == 0)
        {
          continue;
        }

      filename = check_dir_has_iwad(dir, (g_iwads[i].name));

      if (filename != NULL)
        {
          *mission = g_iwads[i].mission;

          return filename;
        }
    }

  return NULL;
}

/* When given an IWAD with the '-iwad' parameter,
 * attempt to identify it by its name.
 */

static gamemission_t identify_iwad_by_name(const char *name, int mask)
{
  size_t i;
  gamemission_t mission;

  name = m_base_name(name);
  mission = none;

  for (i = 0; i < arrlen(g_iwads); ++i)
    {
      /* Check if the filename is this IWAD name.
       * Only use supported missions:
       */

      if (((1 << g_iwads[i].mission) & mask) == 0) continue;

      /* Check if it ends in this IWAD name. */

      if (!strcasecmp(name, g_iwads[i].name))
        {
          mission = g_iwads[i].mission;
          break;
        }
    }

  return mission;
}

/* Add IWAD directories parsed from splitting a path string containing
 * paths separated by PATH_SEPARATOR. 'suffix' is a string to concatenate
 * to the end of the paths before adding them.
 */

static void add_iwad_path(const char *path, const char *suffix)
{
  char *left;
  char *p;
  char *dup_path;

  dup_path = m_string_duplicate(path);

  /* Split into individual dirs within the list. */

  left = dup_path;

  for (; ; )
    {
      p = strchr(left, PATH_SEPARATOR);
      if (p != NULL)
        {
          /* Break at the separator and use the left hand side
           * as another iwad dir
           */

          *p = '\0';

          add_iwad_dir(m_string_join(left, suffix, NULL));
          left = p + 1;
        }
      else
        {
          break;
        }
    }

  add_iwad_dir(m_string_join(left, suffix, NULL));

  free(dup_path);
}

/* Build a list of IWAD files */

static void buld_iwad_dir_list(void)
{
  char *env;

  if (iwad_dirs_built)
    {
      return;
    }

  /* Look in the current directory.  Doom always does this. */

  add_iwad_dir(".");

  /* Next check the directory where the executable is located. This might
   * be different from the current directory.
   */

  add_iwad_dir(m_dir_name(myargv[0]));

  /* Add the board's configured DOOM data directory.  Kconfig documents
   * CONFIG_GAMES_NXDOOM_PREFDIR as "Directory where DOOM WAD files are
   * stored", but until now it was only used for the config/save file
   * location -- nothing actually searched it for IWADs, forcing every
   * launch to rely on the current directory or DOOMWADDIR/DOOMWADPATH
   * being set by hand first.
   */

  add_iwad_dir(CONFIG_GAMES_NXDOOM_PREFDIR);

  /* Add DOOMWADDIR if it is in the environment */

  env = getenv("DOOMWADDIR");
  if (env != NULL)
    {
      add_iwad_dir(env);
    }

  /* Add dirs from DOOMWADPATH: */

  env = getenv("DOOMWADPATH");
  if (env != NULL)
    {
      add_iwad_path(env, "");
    }

  /* Don't run this function again. */

  iwad_dirs_built = true;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Searches WAD search paths for an WAD with a specific filename. */

char *d_find_wad_by_name(const char *name)
{
  char *path;
  char *probe;
  int i;

  /* Absolute path? */

  probe = m_file_case_exists(name);
  if (probe != NULL)
    {
      return probe;
    }

  buld_iwad_dir_list();

  /* Search through all IWAD paths for a file with the given name. */

  for (i = 0; i < num_iwad_dirs; ++i)
    {
      /* As a special case, if this is in DOOMWADDIR or DOOMWADPATH,
       * the "directory" may actually refer directly to an IWAD
       * file.
       */

      probe = m_file_case_exists(iwad_dirs[i]);
      if (dir_is_file(iwad_dirs[i], name) && probe != NULL)
        {
          return probe;
        }

      free(probe);

      /* Construct a string for the full path */

      path = m_string_join(iwad_dirs[i], DIR_SEPARATOR_S, name, NULL);

      probe = m_file_case_exists(path);
      if (probe != NULL)
        {
          return probe;
        }

      free(path);
    }

  /* File not found */

  return NULL;
}

/* d_try_find_wad_by_name
 *
 * Searches for a WAD by its filename, or returns a copy of the filename
 * if not found.
 */

char *d_try_find_wad_by_name(const char *filename)
{
  char *result;

  result = d_find_wad_by_name(filename);

  if (result != NULL)
    {
      return result;
    }
  else
    {
      return m_string_duplicate(filename);
    }
}

/* FindIWAD
 * Checks availability of IWAD files by name,
 * to determine whether registered/commercial features
 * should be executed (notably loading PWADs).
 */

char *d_find_iwad(int mask, gamemission_t *mission)
{
  char *result;
  const char *iwadfile;
  int iwadparm;
  int i;

  /* Check for the -iwad parameter
   *
   * Specify an IWAD file to use.
   *
   * @arg <file>
   *
   */

  iwadparm = m_check_parm_with_args("-iwad", 1);

  if (iwadparm)
    {
      /* Search through IWAD dirs for an IWAD with the given name. */

      iwadfile = myargv[iwadparm + 1];

      result = d_find_wad_by_name(iwadfile);

      if (result == NULL)
        {
          i_error("IWAD file '%s' not found!", iwadfile);
        }

      *mission = identify_iwad_by_name(result, mask);
    }
  else
    {
      /* Search through the list and look for an IWAD */

      result = NULL;

      buld_iwad_dir_list();

      for (i = 0; result == NULL && i < num_iwad_dirs; ++i)
        {
          result = search_dir_for_iwad(iwad_dirs[i], mask, mission);
        }
    }

  return result;
}

/* Find all IWADs in the IWAD search path matching the given mask. */

const iwad_t **d_find_all_iwads(int mask)
{
  const iwad_t **result;
  int result_len;
  char *filename;
  int i;

  result = malloc(sizeof(iwad_t *) * (arrlen(g_iwads) + 1));
  result_len = 0;

  /* Try to find all IWADs */

  for (i = 0; i < arrlen(g_iwads); ++i)
    {
      if (((1 << g_iwads[i].mission) & mask) == 0)
        {
          continue;
        }

      filename = d_find_wad_by_name(g_iwads[i].name);

      if (filename != NULL)
        {
          result[result_len] = &g_iwads[i];
          ++result_len;
        }
    }

  /* End of list */

  result[result_len] = NULL;

  return result;
}

/* Get the IWAD name used for savegames. */

const char *d_save_game_iwad_name(gamemission_t gamemission,
                               game_variant_t gamevariant)
{
  size_t i;

  /* Determine the IWAD name to use for savegames.
   * This determines the directory the savegame files get put into.
   *
   * Note that we match on gamemission rather than on IWAD name.
   * This ensures that doom1.wad and doom.wad saves are stored
   * in the same place.
   */

  if (gamevariant == freedoom)
    {
      if (gamemission == doom)
        {
          return "freedoom1.wad";
        }
      else if (gamemission == doom2)
        {
          return "freedoom2.wad";
        }
    }
  else if (gamevariant == freedm && gamemission == doom2)
    {
      return "freedm.wad";
    }

  for (i = 0; i < arrlen(g_iwads); ++i)
    {
      if (gamemission == g_iwads[i].mission)
        {
          return g_iwads[i].name;
        }
    }

  /* Default fallback: */

  return "unknown.wad";
}

const char *d_suggest_iwad_name(gamemission_t mission, game_mode_t mode)
{
  int i;

  for (i = 0; i < arrlen(g_iwads); ++i)
    {
      if (g_iwads[i].mission == mission && g_iwads[i].mode == mode)
        {
          return g_iwads[i].name;
        }
    }

  return "unknown.wad";
}

const char *d_suggest_game_name(gamemission_t mission, game_mode_t mode)
{
  int i;

  for (i = 0; i < arrlen(g_iwads); ++i)
    {
      if (g_iwads[i].mission == mission &&
          (mode == indetermined || g_iwads[i].mode == mode))
        {
          return g_iwads[i].description;
        }
    }

  return "Unknown game?";
}
