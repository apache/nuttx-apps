/****************************************************************************
 * apps/games/NXDoom/src/i_sdlmusic.c
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
 *  System interface for music.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "doomtype.h"
#include "memio.h"
#include "mus2mid.h"

#include "deh_str.h"
#include "gusconf.h"
#include "i_sound.h"
#include "i_swap.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_config.h"
#include "m_misc.h"
#include "sha1.h"
#include "w_wad.h"
#include "z_zone.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static char *temp_timidity_cfg = NULL;

/* putenv requires a non-const string whose lifetime is the whole program
 * so can't use a string directly, have to do this silliness
 */

static char sdl_mixer_disable_fluidsynth[] =
                                "SDL_MIXER_DISABLE_FLUIDSYNTH=1";

/****************************************************************************
 * Public Data
 ****************************************************************************/

char *timidity_cfg_path = "";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* If the temp_timidity_cfg config variable is set, generate a "wrapper"
 * config file for Timidity to point to the actual config file. This
 * is needed to inject a "dir" command so that the patches are read
 * relative to the actual config file.
 */

static boolean write_wrapper_timidity_config(char *write_path)
{
  char *path;
  FILE *fstream;

  if (!strcmp(timidity_cfg_path, ""))
    {
      return false;
    }

  fstream = fopen(write_path, "w");

  if (fstream == NULL)
    {
      return false;
    }

  path = m_dir_name(timidity_cfg_path);
  fprintf(fstream, "dir %s\n", path);
  free(path);

  fprintf(fstream, "source %s\n", timidity_cfg_path);
  fclose(fstream);

  return true;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void i_init_timidity_config(void)
{
  char *env_string;
  boolean success;

  temp_timidity_cfg = m_temp_file("timidity.cfg");

  if (snd_musicdevice == SNDDEVICE_GUS)
    {
      success = gus_write_config(temp_timidity_cfg);
    }
  else
    {
      success = write_wrapper_timidity_config(temp_timidity_cfg);
    }

  /* Set the TIMIDITY_CFG environment variable to point to the temporary
   * config file.
   */

  if (success)
    {
      env_string = m_string_join("TIMIDITY_CFG=", temp_timidity_cfg, NULL);
      putenv(env_string);

      /* env_string deliberately not freed; see putenv manpage
       * If we're explicitly configured to use Timidity (either through
       * timidity_cfg_path or GUS mode), then disable Fluidsynth, because
       * SDL_mixer considers Fluidsynth a higher priority than Timidity and
       * therefore can end up circumventing Timidity entirely.
       */

      putenv(sdl_mixer_disable_fluidsynth);
    }
  else
    {
      free(temp_timidity_cfg);
      temp_timidity_cfg = NULL;
    }
}
