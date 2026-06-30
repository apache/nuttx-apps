/****************************************************************************
 * apps/games/NXDoom/src/d_mode.c
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
 *   Functions and definitions relating to the game type and operational
 *   mode.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "d_mode.h"
#include "doomtype.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Valid game mode/mission combinations, with the number of
 * episodes/maps for each.
 */

struct modemission
{
  gamemission_t mission;
  game_mode_t mode;
  int episode;
  int map;
};

struct gameversion
{
  gamemission_t mission;
  game_version_t version;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct modemission g_valid_modes[] =
{
  {.mission = doom, .mode = shareware, .episode = 1, .map = 9},
  {.mission = doom, .mode = registered, .episode = 3, .map = 9},
  {.mission = doom, .mode = retail, .episode = 4, .map = 9},
  {.mission = doom2, .mode = commercial, .episode = 1, .map = 32},
};

/* Table of valid versions */

static struct gameversion g_valid_versions[] =
{
  {.mission = doom, .version = exe_doom_1_2},
  {.mission = doom, .version = exe_doom_1_5},
  {.mission = doom, .version = exe_doom_1_666},
  {.mission = doom, .version = exe_doom_1_7},
  {.mission = doom, .version = exe_doom_1_8},
  {.mission = doom, .version = exe_doom_1_9},
  {.mission = doom, .version = exe_hacx},
  {.mission = doom, .version = exe_ultimate},
  {.mission = doom, .version = exe_final},
  {.mission = doom, .version = exe_final2},
  {.mission = doom, .version = exe_chex},
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Check that a gamemode + gamemission received over the network is valid. */

boolean d_valid_game_mode(gamemission_t mission, game_mode_t mode)
{
  int i;

  for (i = 0; i < arrlen(g_valid_modes); ++i)
    {
      if (g_valid_modes[i].mode == mode &&
          g_valid_modes[i].mission == mission)
        {
          return true;
        }
    }

  return false;
}

boolean d_valid_episode_map(gamemission_t mission, game_mode_t mode,
                            int episode, int map)
{
  int i;

  /* Hacks for Heretic secret episodes */

  if (mission == heretic)
    {
      if (mode == retail && episode == 6)
        {
          return map >= 1 && map <= 3;
        }
      else if (mode == registered && episode == 4)
        {
          return map == 1;
        }
    }

  /* Find the table entry for this mission/mode combination. */

  for (i = 0; i < arrlen(g_valid_modes); ++i)
    {
      if (mission == g_valid_modes[i].mission &&
          mode == g_valid_modes[i].mode)
        {
          return episode >= 1 && episode <= g_valid_modes[i].episode &&
                 map >= 1 && map <= g_valid_modes[i].map;
        }
    }

  /* Unknown mode/mission combination */

  return false;
}

/* Get the number of valid episodes for the specified mission/mode. */

int d_get_num_episodes(gamemission_t mission, game_mode_t mode)
{
  int episode;

  episode = 1;

  while (d_valid_episode_map(mission, mode, episode, 1))
    {
      ++episode;
    }

  return episode - 1;
}

boolean d_valid_game_version(gamemission_t mission, game_version_t version)
{
  int i;

  /* All Doom variants can use the Doom versions. */

  if (mission == doom2 || mission == pack_plut || mission == pack_tnt ||
      mission == pack_hacx || mission == pack_chex)
    {
      mission = doom;
    }

  for (i = 0; i < arrlen(g_valid_versions); ++i)
    {
      if (g_valid_versions[i].mission == mission &&
          g_valid_versions[i].version == version)
        {
          return true;
        }
    }

  return false;
}

/* Does this mission type use ExMy form, rather than MAPxy form? */

boolean d_is_episode_map(gamemission_t mission)
{
  switch (mission)
    {
    case doom:
    case heretic:
    case pack_chex:
      return true;

    case none:
    case hexen:
    case doom2:
    case pack_hacx:
    case pack_tnt:
    case pack_plut:
    case strife:
    default:
      return false;
    }
}

const char *d_game_mission_string(gamemission_t mission)
{
  switch (mission)
    {
    case none:
    default:
      return "none";
    case doom:
      return "doom";
    case doom2:
      return "doom2";
    case pack_tnt:
      return "tnt";
    case pack_plut:
      return "plutonia";
    case pack_hacx:
      return "hacx";
    case pack_chex:
      return "chex";
    case heretic:
      return "heretic";
    case hexen:
      return "hexen";
    case strife:
      return "strife";
    }
}

const char *d_game_mode_string(game_mode_t mode)
{
  switch (mode)
    {
    case shareware:
      return "shareware";
    case registered:
      return "registered";
    case commercial:
      return "commercial";
    case retail:
      return "retail";
    case indetermined:
    default:
      return "unknown";
    }
}
