/****************************************************************************
 * apps/games/NXDoom/src/i_pcsound.c
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
 *  System interface for PC speaker sound.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <string.h>

#include "doomtype.h"

#include "deh_str.h"
#include "i_sound.h"
#include "m_misc.h"
#include "w_wad.h"
#include "z_zone.h"

#include "pcsound.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TIMER_FREQ 1193181 /* Hz */

/****************************************************************************
 * Private Data
 ****************************************************************************/

static boolean g_pcs_initialized = false;

static gamemission_t g_gamemission;

static uint8_t *g_current_sound_lump = NULL;
static uint8_t *g_current_sound_pos = NULL;
static unsigned int g_current_sound_remaining = 0;
static int g_current_sound_handle = 0;
static int g_current_sound_lump_num = -1;

static const uint16_t g_divisors[] =
{
  0,    6818, 6628, 6449, 6279, 6087, 5906, 5736, 5575, 5423, 5279, 5120,
  4971, 4830, 4697, 4554, 4435, 4307, 4186, 4058, 3950, 3836, 3728, 3615,
  3519, 3418, 3323, 3224, 3131, 3043, 2960, 2875, 2794, 2711, 2633, 2560,
  2485, 2415, 2348, 2281, 2213, 2153, 2089, 2032, 1975, 1918, 1864, 1810,
  1757, 1709, 1659, 1612, 1565, 1521, 1478, 1435, 1395, 1355, 1316, 1280,
  1242, 1207, 1173, 1140, 1107, 1075, 1045, 1015, 986,  959,  931,  905,
  879,  854,  829,  806,  783,  760,  739,  718,  697,  677,  658,  640,
  621,  604,  586,  570,  553,  538,  522,  507,  493,  479,  465,  452,
  439,  427,  415,  403,  391,  380,  369,  359,  348,  339,  329,  319,
  310,  302,  293,  285,  276,  269,  261,  253,  246,  239,  232,  226,
  219,  213,  207,  201,  195,  190,  184,  179,
};

static const snddevice_t g_sound_pcsound_devices[] =
{
  SNDDEVICE_PCSPEAKER,
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

const sound_module_t sound_pcsound_module =
{
  g_sound_pcsound_devices,
  arrlen(g_sound_pcsound_devices),
#if 0
  i_pcs_init_sound,
  i_pcs_shutdown_sound,
  i_pcs_get_sfx_lumpnum,
  i_pcs_update_sound,
  i_pcs_update_sound_params,
  i_pcs_start_sound,
  i_pcs_stop_sound,
  i_pcs_sound_is_playing,
#endif
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void pcs_callback_func(int *duration, int *freq)
{
}

static boolean cache_pcs_lump(sfxinfo_t *sfxinfo)
{
  int lumplen;
  int headerlen;

  /* Free the current sound lump back to the cache */

  if (g_current_sound_lump != NULL)
    {
      w_release_lump_num(g_current_sound_lump_num);
      g_current_sound_lump = NULL;
    }

  /* Load from WAD */

  g_current_sound_lump = w_cache_lump_num(sfxinfo->lumpnum, PU_STATIC);
  lumplen = w_lump_length(sfxinfo->lumpnum);

  /* Read header */

  if (g_current_sound_lump[0] != 0x00 || g_current_sound_lump[1] != 0x00)
    {
      return false;
    }

  headerlen = (g_current_sound_lump[3] << 8) | g_current_sound_lump[2];

  if (headerlen > lumplen - 4)
    {
      return false;
    }

  /* Header checks out ok */

  g_current_sound_remaining = headerlen;
  g_current_sound_pos = g_current_sound_lump + 4;
  g_current_sound_lump_num = sfxinfo->lumpnum;

  return true;
}

/* These Doom PC speaker sounds are not played - this can be seen in the
 * Heretic source code, where there are remnants of this left over
 * from Doom.
 */

static boolean is_disabled_sound(sfxinfo_t *sfxinfo)
{
  int i;
  const char *disabled_sounds[] = {
      "posact", "bgact", "dmact", "dmpain", "popain", "sawidl", "rifle",
  };

  for (i = 0; i < arrlen(disabled_sounds); ++i)
    {
      if (!strcmp(sfxinfo->name, disabled_sounds[i]))
        {
          return true;
        }
    }

  return false;
}

static int i_pcs_start_sound(sfxinfo_t *sfxinfo, int channel, int vol,
                             int sep, int pitch)
{
}

static void i_pcs_stop_sound(int handle)
{
}

/* Retrieve the raw data lump index for a given SFX name. */

static int i_pcs_get_sfx_lumpnum(sfxinfo_t *sfx)
{
  char namebuf[9];

  if (g_gamemission == doom || g_gamemission == strife)
    {
      snprintf(namebuf, sizeof(namebuf), "dp%s", (sfx->name));

      if (g_gamemission == strife && w_check_num_for_name(namebuf) == -1)
        {
          /* Missing sounds replaced with DPRIFLE. */

          snprintf(namebuf, sizeof(namebuf), "dp%s", ("rifle"));
        }
    }
  else
    {
      m_str_copy(namebuf, (sfx->name), sizeof(namebuf));
    }

  return w_get_num_for_name(namebuf);
}

static boolean i_pcs_sound_is_playing(int handle)
{
  if (!g_pcs_initialized)
    {
      return false;
    }

  if (handle != g_current_sound_handle)
    {
      return false;
    }

  return g_current_sound_lump != NULL && g_current_sound_remaining > 0;
}

static boolean i_pcs_init_sound(gamemission_t mission)
{
}

static void i_pcs_shutdown_sound(void)
{
}

static void i_pcs_update_sound(void)
{
  /* no-op. */
}

static void i_pcs_update_sound_params(int channel, int vol, int sep)
{
  /* no-op. */
}
