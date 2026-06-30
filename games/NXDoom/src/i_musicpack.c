/****************************************************************************
 * apps/games/NXDoom/src/i_musicpack.c
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

#include "i_glob.h"

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
 * Private Function Prototypes
 ****************************************************************************/

static boolean i_null_init_music(void);
static void i_null_shutdown_music(void);
static void i_null_set_music_volume(int volume);
static void i_null_pause_song(void);
static void i_null_resume_song(void);
static void *i_null_register_song(void *data, int len);
static void i_null_unregister_song(void *handle);
static void i_null_play_song(void *handle, boolean looping);
static void i_null_stop_song(void);
static boolean i_null_musicisplaying(void);
static void i_null_pollmusic(void);

/****************************************************************************
 * Public Data
 ****************************************************************************/

char *music_pack_path = "";

const music_module_t music_pack_module =
{
  NULL,
  0,
  i_null_init_music,
  i_null_shutdown_music,
  i_null_set_music_volume,
  i_null_pause_song,
  i_null_resume_song,
  i_null_register_song,
  i_null_unregister_song,
  i_null_play_song,
  i_null_stop_song,
  i_null_musicisplaying,
  i_null_pollmusic,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static boolean i_null_init_music(void)
{
  return false;
}

static void i_null_shutdown_music(void)
{
  return;
}

static void i_null_set_music_volume(int volume)
{
  return;
}

static void i_null_pause_song(void)
{
  return;
}

static void i_null_resume_song(void)
{
  return;
}

static void *i_null_register_song(void *data, int len)
{
  return NULL;
}

static void i_null_unregister_song(void *handle)
{
  return;
}

static void i_null_play_song(void *handle, boolean looping)
{
  return;
}

static void i_null_stop_song(void)
{
  return;
}

static boolean i_null_musicisplaying(void)
{
  return false;
}

static void i_null_pollmusic(void)
{
  return;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
