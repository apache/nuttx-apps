/****************************************************************************
 * apps/games/NXDoom/src/doom/s_sound.h
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
 *  The not so system specific sound interface.
 *
 ****************************************************************************/

#ifndef __S_SOUND__
#define __S_SOUND__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "p_mobj.h"
#include "sounds.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern int snd_channels;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Initializes sound stuff, including volume
 * Sets channels, SFX and music volume, allocates channel buffer, sets s_sfx
 * lookup.
 */

void s_init(int sfxvolume, int musicvolume);

/* Per level startup code.
 * Kills playing sounds at start of level, determines music if any, changes
 * music.
 */

void s_start(void);

/* Start sound for thing at <origin> using <sound_id> from sounds.h */

void s_start_sound(void *origin, int sound_id);

/* Stop sound for thing at <origin> */

void s_stop_sound(mobj_t *origin);

/* Start music using <music_id> from sounds.h */

void s_start_music(int music_id);

/* Start music using <music_id> from sounds.h, and set whether looping */

void s_change_music(int music_id, int looping);

/* Stop and resume music, during game PAUSE. */

void s_pause_sound(void);
void s_resume_sound(void);

/* Updates music & sounds */

void s_update_sounds(mobj_t *listener);

void s_set_music_volume(int volume);
void s_set_sfx_volume(int volume);

#endif /* __S_SOUND__ */
