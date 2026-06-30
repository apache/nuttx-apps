/****************************************************************************
 * apps/games/NXDoom/src/doom/s_sound.c
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
 * DESCRIPTION:  none
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "i_sound.h"
#include "i_system.h"

#include "deh_str.h"

#include "doomstat.h"
#include "doomtype.h"

#include "s_sound.h"
#include "sounds.h"

#include "m_argv.h"
#include "m_misc.h"
#include "m_random.h"

#include "p_local.h"
#include "w_wad.h"
#include "z_zone.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* when to clip out sounds
 * Does not fit the large outdoor areas.
 */

#define S_CLIPPING_DIST (1200 * FRACUNIT)

/* Distance tp origin when sounds should be maxed out.
 * This should relate to movement clipping resolution
 * (see BLOCKMAP handling).
 * In the source code release: (160*FRACUNIT).  Changed back to the
 * Vanilla value of 200 (why was this changed?)
 */

#define S_CLOSE_DIST (200 * FRACUNIT)

/* The range over which sound attenuates */

#define S_ATTENUATOR ((S_CLIPPING_DIST - S_CLOSE_DIST) >> FRACBITS)

/* Stereo separation */

#define S_STEREO_SWING (96 * FRACUNIT)

#define NORM_PRIORITY 64
#define NORM_SEP 128

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct
{
  /* sound information (if null, channel avail.) */

  sfxinfo_t *sfxinfo;

  /* origin of sound */

  mobj_t *origin;

  /* handle of the sound being played */

  int handle;

  int pitch;
} channel_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* The set of channels available */

static channel_t *channels;

/* Internal volume level, ranging from 0-127 */

static int g_snd_sfx_volume;

/* Whether songs are mus_paused */

static boolean mus_paused;

/* Music currently being played */

static musicinfo_t *mus_playing = NULL;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Maximum volume of a sound effect.
 * Internal default is max out of 0-15.
 */

int g_sfx_volume = 8;

/* Maximum volume of music. */

int g_music_volume = 8;

/* Number of channels to use */

int snd_channels = 8;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void s_stop_channel(int cnum)
{
  int i;
  channel_t *c;

  c = &channels[cnum];

  if (c->sfxinfo)
    {
      /* stop the sound playing */

      if (i_sound_playing(c->handle))
        {
          i_stop_sound(c->handle);
        }

      /* check to see if other channels are playing the sound */

      for (i = 0; i < snd_channels; i++)
        {
          if (cnum != i && c->sfxinfo == channels[i].sfxinfo)
            {
              break;
            }
        }

      /* degrade usefulness of sound data */

      c->sfxinfo->usefulness--;
      c->sfxinfo = NULL;
      c->origin = NULL;
    }
}

/* S_GetChannel: If none available, return -1. Otherwise channel #. */

static int s_get_channel(mobj_t *origin, sfxinfo_t *sfxinfo)
{
  /* channel number to use */

  int cnum;

  channel_t *c;

  /* Find an open channel */

  for (cnum = 0; cnum < snd_channels; cnum++)
    {
      if (!channels[cnum].sfxinfo)
        {
          break;
        }
      else if (origin && channels[cnum].origin == origin)
        {
          s_stop_channel(cnum);
          break;
        }
    }

  /* None available */

  if (cnum == snd_channels)
    {
      /* Look for lower priority */

      for (cnum = 0; cnum < snd_channels; cnum++)
        {
          if (channels[cnum].sfxinfo->priority >= sfxinfo->priority)
            {
              break;
            }
        }

      if (cnum == snd_channels)
        {
          /* FUCK!  No lower priority.  Sorry, Charlie. */

          return -1;
        }
      else
        {
          /* Otherwise, kick out lower priority. */

          s_stop_channel(cnum);
        }
    }

  c = &channels[cnum];

  /* channel is decided to be cnum. */

  c->sfxinfo = sfxinfo;
  c->origin = origin;

  return cnum;
}

/* Changes volume and stereo-separation variables from the norm of a sound
 * effect to be played. If the sound is not audible, returns a 0. Otherwise,
 * modifies parameters and returns 1.
 */

static int s_adjust_sound_params(mobj_t *listener, mobj_t *source, int *vol,
                                 int *sep)
{
  fixed_t approx_dist;
  fixed_t adx;
  fixed_t ady;
  angle_t angle;

  /* calculate the distance to sound origin and clip it if necessary */

  adx = abs(listener->x - source->x);
  ady = abs(listener->y - source->y);

  /* From _GG1_ p.428. Appox. Euclidean distance fast. */

  approx_dist = adx + ady - ((adx < ady ? adx : ady) >> 1);

  if (gamemap != 8 && approx_dist > S_CLIPPING_DIST)
    {
      return 0;
    }

  /* angle of source to listener */

  angle = r_point_to_angle2(listener->x, listener->y, source->x, source->y);

  if (angle > listener->angle)
    {
      angle = angle - listener->angle;
    }
  else
    {
      angle = angle + (0xffffffff - listener->angle);
    }

  angle >>= ANGLETOFINESHIFT;

  /* stereo separation */

  *sep = 128 - (fixed_mul(S_STEREO_SWING, finesine[angle]) >> FRACBITS);

  /* volume calculation */

  if (approx_dist < S_CLOSE_DIST)
    {
      *vol = g_snd_sfx_volume;
    }
  else if (gamemap == 8)
    {
      if (approx_dist > S_CLIPPING_DIST)
        {
          approx_dist = S_CLIPPING_DIST;
        }

      *vol = 15 + ((g_snd_sfx_volume - 15) *
                   ((S_CLIPPING_DIST - approx_dist) >> FRACBITS)) /
                      S_ATTENUATOR;
    }
  else
    {
      /* distance effect */

      *vol = (g_snd_sfx_volume *
              ((S_CLIPPING_DIST - approx_dist) >> FRACBITS)
              ) / S_ATTENUATOR;
    }

  return (*vol > 0);
}

/* clamp supplied integer to the range 0 <= x <= 255. */

static int clamp(int x)
{
  if (x < 0)
    {
      return 0;
    }
  else if (x > 255)
    {
      return 255;
    }

  return x;
}

static void s_shutdown(void)
{
  i_shutdown_sound();
  i_shutdown_music();
}

static void s_stop_music(void)
{
  if (mus_playing)
    {
      if (mus_paused)
        {
          i_resume_song();
        }

      i_stop_song();
      i_unregister_song(mus_playing->handle);
      w_release_lump_num(mus_playing->lumpnum);
      mus_playing->data = NULL;
      mus_playing = NULL;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Initializes sound stuff, including volume
 *
 * Sets channels, SFX and music volume, allocates channel buffer, sets s_sfx
 * lookup.
 */

void s_init(int sfxvolume, int musicvolume)
{
  int i;

#if 0
  if (gameversion == exe_doom_1_666)
    {
      if (logical_gamemission == doom)
        {
          i_set_opl_driver_ver(opl_doom1_1_666);
        }
      else
        {
          i_set_opl_driver_ver(opl_doom2_1_666);
        }
    }
  else
    {
      i_set_opl_driver_ver(opl_doom_1_9);
    }
#endif

  i_precache_sounds(s_sfx, SFX_NUMSFX);

  s_set_sfx_volume(sfxvolume);
  s_set_music_volume(musicvolume);

  /* Allocating the internal channels for mixing
   * (the maximum number of sounds rendered
   * simultaneously) within zone memory.
   */

  channels = z_malloc(snd_channels * sizeof(channel_t), PU_STATIC, 0);

  /* Free all channels for use */

  for (i = 0; i < snd_channels; i++)
    {
      channels[i].sfxinfo = 0;
    }

  /* no sounds are playing, and they are not mus_paused */

  mus_paused = 0;

  /* Note that sounds have not been cached (yet). */

  for (i = 1; i < SFX_NUMSFX; i++)
    {
      s_sfx[i].lumpnum = s_sfx[i].usefulness = -1;
    }

  /* Doom defaults to pitch-shifting off. */

  if (snd_pitchshift == -1)
    {
      snd_pitchshift = 0;
    }

  i_at_exit(s_shutdown, true);
}

/* Per level startup code.
 * Kills playing sounds at start of level, determines music if any, changes
 * music.
 */

void s_start(void)
{
  int cnum;
  int mnum;

  /* kill all playing sounds at start of level (trust me - a good idea)
   */

  for (cnum = 0; cnum < snd_channels; cnum++)
    {
      if (channels[cnum].sfxinfo)
        {
          s_stop_channel(cnum);
        }
    }

  /* start new music for the level */

  mus_paused = 0;

  if (gamemode == commercial)
    {
      mnum = MUS_RUNNIN + gamemap - 1;
    }
  else
    {
      int spmus[] = {
          /* Song - Who? - Where? */

          MUS_E3M4, /* American     e4m1 */
          MUS_E3M2, /* Romero       e4m2 */
          MUS_E3M3, /* Shawn        e4m3 */
          MUS_E1M5, /* American     e4m4 */
          MUS_E2M7, /* Tim          e4m5 */
          MUS_E2M4, /* Romero       e4m6 */
          MUS_E2M6, /* J.Anderson   e4m7 CHIRON.WAD */
          MUS_E2M5, /* Shawn        e4m8 */
          MUS_E1M9, /* Tim          e4m9 */
      };

      if (gameepisode < 4)
        {
          mnum = MUS_E1M1 + (gameepisode - 1) * 9 + gamemap - 1;
        }
      else
        {
          mnum = spmus[gamemap - 1];
        }
    }

  s_change_music(mnum, true);
}

void s_stop_sound(mobj_t *origin)
{
  int cnum;

  for (cnum = 0; cnum < snd_channels; cnum++)
    {
      if (channels[cnum].sfxinfo && channels[cnum].origin == origin)
        {
          s_stop_channel(cnum);
          break;
        }
    }
}

void s_start_sound(void *origin_p, int sfx_id)
{
  sfxinfo_t *sfx;
  mobj_t *origin;
  int rc;
  int sep;
  int pitch;
  int cnum;
  int volume;

  origin = (mobj_t *)origin_p;
  volume = g_snd_sfx_volume;

  /* check for bogus sound # */

  if (sfx_id < 1 || sfx_id > SFX_NUMSFX)
    {
      i_error("Bad sfx #: %d", sfx_id);
    }

  sfx = &s_sfx[sfx_id];

  /* Initialize sound parameters */

  pitch = NORM_PITCH;
  if (sfx->link)
    {
      volume += sfx->volume;
      pitch = sfx->pitch;

      if (volume < 1)
        {
          return;
        }

      if (volume > g_snd_sfx_volume)
        {
          volume = g_snd_sfx_volume;
        }
    }

  /* Check to see if it is audible, and if not, modify the params */

  if (origin && origin != players[consoleplayer].mo)
    {
      rc = s_adjust_sound_params(players[consoleplayer].mo, origin, &volume,
                                 &sep);

      if (origin->x == players[consoleplayer].mo->x &&
          origin->y == players[consoleplayer].mo->y)
        {
          sep = NORM_SEP;
        }

      if (!rc)
        {
          return;
        }
    }
  else
    {
      sep = NORM_SEP;
    }

  /* hacks to vary the sfx pitches */

  if (sfx_id >= SFX_SAWUP && sfx_id <= SFX_SAWHIT)
    {
      pitch += 8 - (m_random() & 15);
    }
  else if (sfx_id != SFX_ITEMUP && sfx_id != SFX_TINK)
    {
      pitch += 16 - (m_random() & 31);
    }

  pitch = clamp(pitch);

  s_stop_sound(origin); /* kill old sound */

  /* try to find a channel */

  cnum = s_get_channel(origin, sfx);

  if (cnum < 0)
    {
      return;
    }

  /* increase the usefulness */

  if (sfx->usefulness++ < 0)
    {
      sfx->usefulness = 1;
    }

  if (sfx->lumpnum < 0)
    {
      sfx->lumpnum = i_get_sfx_lumpnum(sfx);
    }

  channels[cnum].pitch = pitch;
  channels[cnum].handle =
      i_start_sound(sfx, cnum, volume, sep, channels[cnum].pitch);
}

/* Stop and resume music, during game PAUSE. */

void s_pause_sound(void)
{
  if (mus_playing && !mus_paused)
    {
      i_pause_song();
      mus_paused = true;
    }
}

void s_resume_sound(void)
{
  if (mus_playing && mus_paused)
    {
      i_resume_song();
      mus_paused = false;
    }
}

/* Updates music & sounds */

void s_update_sounds(mobj_t *listener)
{
  int audible;
  int cnum;
  int volume;
  int sep;
  sfxinfo_t *sfx;
  channel_t *c;

  i_update_sound();

  for (cnum = 0; cnum < snd_channels; cnum++)
    {
      c = &channels[cnum];
      sfx = c->sfxinfo;

      if (c->sfxinfo)
        {
          if (i_sound_playing(c->handle))
            {
              /* initialize parameters */

              volume = g_snd_sfx_volume;
              sep = NORM_SEP;

              if (sfx->link)
                {
                  volume += sfx->volume;
                  if (volume < 1)
                    {
                      s_stop_channel(cnum);
                      continue;
                    }
                  else if (volume > g_snd_sfx_volume)
                    {
                      volume = g_snd_sfx_volume;
                    }
                }

              /* check non-local sounds for distance clipping or modify their
               * params
               */

              if (c->origin && listener != c->origin)
                {
                  audible = s_adjust_sound_params(listener, c->origin,
                                                  &volume, &sep);

                  if (!audible)
                    {
                      s_stop_channel(cnum);
                    }
                  else
                    {
                      i_update_sound_params(c->handle, volume, sep);
                    }
                }
            }
          else
            {
              /* if channel is allocated but sound has stopped, free it
               */

              s_stop_channel(cnum);
            }
        }
    }
}

void s_set_music_volume(int volume)
{
  if (volume < 0 || volume > 127)
    {
      i_error("Attempt to set music volume at %d", volume);
    }

  i_set_music_volume(volume);
}

void s_set_sfx_volume(int volume)
{
  if (volume < 0 || volume > 127)
    {
      i_error("Attempt to set sfx volume at %d", volume);
    }

  g_snd_sfx_volume = volume;
}

/* Starts some music with the music id found in sounds.h. */

void s_start_music(int m_id)
{
  s_change_music(m_id, false);
}

void s_change_music(int musicnum, int looping)
{
  musicinfo_t *music = NULL;
  char namebuf[9];
  void *handle;

  /* The Doom IWAD file has two versions of the intro music: d_intro
   * and d_introa. The latter is used for OPL playback.
   */

  if (musicnum == MUS_INTRO &&
      (snd_musicdevice == SNDDEVICE_ADLIB ||
       snd_musicdevice == SNDDEVICE_SB) &&
      w_check_num_for_name("D_INTROA") >= 0)
    {
      musicnum = MUS_INTROA;
    }

  if (musicnum <= MUS_NONE || musicnum >= MUS_NUMMUSIC)
    {
      i_error("Bad music number %d", musicnum);
    }
  else
    {
      music = &s_music[musicnum];
    }

  if (mus_playing == music)
    {
      return;
    }

  /* shutdown old music */

  s_stop_music();

  /* get lumpnum if necessary */

  if (!music->lumpnum)
    {
      snprintf(namebuf, sizeof(namebuf), "d_%s", (music->name));
      music->lumpnum = w_get_num_for_name(namebuf);
    }

  music->data = w_cache_lump_num(music->lumpnum, PU_STATIC);

  handle = i_register_song(music->data, w_lump_length(music->lumpnum));
  music->handle = handle;
  i_play_song(handle, looping);

  mus_playing = music;
}
