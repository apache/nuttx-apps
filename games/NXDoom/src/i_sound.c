/****************************************************************************
 * apps/games/NXDoom/src/i_sound.c
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "doomtype.h"

#include "gusconf.h"
#include "i_sound.h"
#include "i_video.h"
#include "m_argv.h"
#include "m_config.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Sound sample rate to use for digital output (Hz) */

int snd_samplerate = 44100;

/* Maximum number of bytes to dedicate to allocated sound effects.
 * (Default: 64MB)
 */

int snd_cachesize = 64 * 1024 * 1024;

/* Config variable that controls the sound buffer size.
 * We default to 28ms (1000 / 35fps = 1 buffer per tic).
 */

int snd_maxslicetime_ms = 28;

/* External command to invoke to play back music. */

char *snd_musiccmd = "";

/* Whether to vary the pitch of sound effects
 * Each game will set the default differently
 */

int snd_pitchshift = -1;

int snd_musicdevice = SNDDEVICE_SB;
int snd_sfxdevice = SNDDEVICE_SB;

/* Scale factor used when converting libsamplerate floating point numbers
 * to integers. Too high means the sounds can clip; too low means they
 * will be too quiet. This is an amount that should avoid clipping most
 * of the time: with all the Doom IWAD sound effects, at least. If a PWAD
 * is used, clipping might occur.
 *
 * NOTE: originally from i_sdlsound.c
 */

float libsamplerate_scale = 0.65f;
int use_libsamplerate = 0;

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Low-level sound and music modules we are using */

static const sound_module_t *sound_module;
static const music_module_t *music_module;

/* If true, the music pack module was successfully initialized. */

static boolean music_packs_active = false;

/* This is either equal to music_module or &music_pack_module,
 * depending on whether the current track is substituted.
 */

static const music_module_t *active_music_module;

/* DOS-specific options: These are unused but should be maintained
 * so that the config file can be shared between chocolate
 * doom and doom.exe
 */

static int snd_sbport = 0;
static int snd_sbirq = 0;
static int snd_sbdma = 0;
static int snd_mport = 0;

/* Compiled-in sound modules: */

static const sound_module_t *sound_modules[] =
{
  &sound_pcsound_module,
  NULL,
};

/* Compiled-in music modules: */

static const music_module_t *music_modules[] =
{
#if 0
    &music_opl_module,
#endif
    NULL,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Check if a sound device is in the given list of devices */

static boolean snd_device_in_list(snddevice_t device,
                                  const snddevice_t *list, int len)
{
  int i;

  for (i = 0; i < len; ++i)
    {
      if (device == list[i])
        {
          return true;
        }
    }

  return false;
}

/* Find and initialize a sound_module_t appropriate for the setting
 * in snd_sfxdevice.
 */

static void init_sfx_module(gamemission_t mission)
{
  int i;

  sound_module = NULL;

  for (i = 0; sound_modules[i] != NULL; ++i)
    {
      /* Is the sfx device in the list of devices supported by
       * this module?
       */

      if (snd_device_in_list(snd_sfxdevice, sound_modules[i]->sound_devices,
                          sound_modules[i]->num_sound_devices))
        {
          /* initialize the module */

          if (sound_modules[i]->init(mission))
            {
              sound_module = sound_modules[i];
              return;
            }
        }
    }
}

/* initialize music according to snd_musicdevice. */

static void init_music_module(void)
{
  int i;

  music_module = NULL;

  for (i = 0; music_modules[i] != NULL; ++i)
    {
      /* Is the music device in the list of devices supported
       * by this module?
       */

      if (snd_device_in_list(snd_musicdevice,
                             music_modules[i]->sound_devices,
                             music_modules[i]->num_sound_devices))
        {
          /* initialize the module */

          if (music_modules[i]->init())
            {
              music_module = music_modules[i];
              return;
            }
        }
    }
}

static void check_volume_separation(int *vol, int *sep)
{
  if (*sep < 0)
    {
      *sep = 0;
    }
  else if (*sep > 254)
    {
      *sep = 254;
    }

  if (*vol < 0)
    {
      *vol = 0;
    }
  else if (*vol > 127)
    {
      *vol = 127;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* initializes sound stuff, including volume
 * Sets channels, SFX and music volume, allocates channel buffer, sets s_sfx
 * lookup.
 */

void i_init_sound(gamemission_t mission)
{
  boolean nosound, nosfx, nomusic, nomusicpacks;

  /* @vanilla
   *
   * Disable all sound output.
   */

  nosound = m_check_parm("-nosound") > 0;

  /* @vanilla
   *
   * Disable sound effects.
   */

  nosfx = m_check_parm("-nosfx") > 0;

  /* @vanilla
   *
   * Disable music.
   */

  nomusic = m_check_parm("-nomusic") > 0;

  /* Disable substitution music packs. */

  nomusicpacks = m_parm_exists("-nomusicpacks");

  /* Auto configure the music pack directory. */

  m_set_music_pack_dir();

  /* initialize the sound and music subsystems. */

  if (!nosound && !screensaver_mode)
    {
      /* This is kind of a hack. If native MIDI is enabled, set up
       * the TIMIDITY_CFG environment variable here before SDL_mixer
       * is opened.
       */

      if (!nomusic && (snd_musicdevice == SNDDEVICE_GENMIDI ||
                       snd_musicdevice == SNDDEVICE_GUS))
        {
          i_init_timidity_config();
        }

      if (!nosfx)
        {
          init_sfx_module(mission);
        }

      if (!nomusic)
        {
          init_music_module();
          active_music_module = music_module;
        }

      /* We may also have substitute MIDIs we can load. */

      if (!nomusicpacks && music_module != NULL)
        {
          music_packs_active = music_pack_module.init();
        }
    }
}

void i_shutdown_sound(void)
{
  if (sound_module != NULL)
    {
      sound_module->shutdown();
    }

  if (music_packs_active)
    {
      music_pack_module.shutdown();
    }

  if (music_module != NULL)
    {
      music_module->shutdown();
    }
}

int i_get_sfx_lumpnum(sfxinfo_t *sfxinfo)
{
  if (sound_module != NULL)
    {
      return sound_module->get_sfx_lumpnum(sfxinfo);
    }
  else
    {
      return 0;
    }
}

void i_update_sound(void)
{
  if (sound_module != NULL)
    {
      sound_module->update();
    }

  if (active_music_module != NULL && active_music_module->poll != NULL)
    {
      active_music_module->poll();
    }
}

void i_update_sound_params(int channel, int vol, int sep)
{
  if (sound_module != NULL)
    {
      check_volume_separation(&vol, &sep);
      sound_module->update_sound_params(channel, vol, sep);
    }
}

int i_start_sound(sfxinfo_t *sfxinfo, int channel, int vol, int sep,
                  int pitch)
{
  if (sound_module != NULL)
    {
      check_volume_separation(&vol, &sep);
      return sound_module->start_sound(sfxinfo, channel, vol, sep, pitch);
    }
  else
    {
      return 0;
    }
}

void i_stop_sound(int channel)
{
  if (sound_module != NULL)
    {
      sound_module->stop_sound(channel);
    }
}

boolean i_sound_playing(int channel)
{
  if (sound_module != NULL)
    {
      return sound_module->sound_is_playing(channel);
    }
  else
    {
      return false;
    }
}

void i_precache_sounds(sfxinfo_t *sounds, int num_sounds)
{
  if (sound_module != NULL && sound_module->cache_sounds != NULL)
    {
      sound_module->cache_sounds(sounds, num_sounds);
    }
}

void i_init_music(void)
{
}

void i_shutdown_music(void)
{
}

void i_set_music_volume(int volume)
{
  if (music_module != NULL)
    {
      music_module->set_music_volume(volume);

      if (music_packs_active && music_module != &music_pack_module)
        {
          music_pack_module.set_music_volume(volume);
        }
    }
}

void i_pause_song(void)
{
  if (active_music_module != NULL)
    {
      active_music_module->pause_music();
    }
}

void i_resume_song(void)
{
  if (active_music_module != NULL)
    {
      active_music_module->resume_music();
    }
}

void *i_register_song(void *data, int len)
{
  /* If the music pack module is active, check to see if there is a
   * valid substitution for this track. If there is, we set the
   * active_music_module pointer to the music pack module for the
   * duration of this particular track.
   */

  if (music_packs_active)
    {
      void *handle;

      handle = music_pack_module.register_song(data, len);
      if (handle != NULL)
        {
          active_music_module = &music_pack_module;
          return handle;
        }
    }

  /* No substitution for this track, so use the main module. */

  active_music_module = music_module;
  if (active_music_module != NULL)
    {
      return active_music_module->register_song(data, len);
    }
  else
    {
      return NULL;
    }
}

void i_unregister_song(void *handle)
{
  if (active_music_module != NULL)
    {
      active_music_module->unregister_song(handle);
    }
}

void i_play_song(void *handle, boolean looping)
{
  if (active_music_module != NULL)
    {
      active_music_module->play_song(handle, looping);
    }
}

void i_stop_song(void)
{
  if (active_music_module != NULL)
    {
      active_music_module->stop_song();
    }
}

void i_bind_sound_variables(void)
{
  m_bind_int_variable("snd_musicdevice", &snd_musicdevice);
  m_bind_int_variable("snd_sfxdevice", &snd_sfxdevice);
  m_bind_int_variable("snd_sbport", &snd_sbport);
  m_bind_int_variable("snd_sbirq", &snd_sbirq);
  m_bind_int_variable("snd_sbdma", &snd_sbdma);
  m_bind_int_variable("snd_mport", &snd_mport);
  m_bind_int_variable("snd_maxslicetime_ms", &snd_maxslicetime_ms);
  m_bind_string_variable("snd_musiccmd", &snd_musiccmd);
  m_bind_int_variable("snd_samplerate", &snd_samplerate);
  m_bind_int_variable("snd_cachesize", &snd_cachesize);
  m_bind_int_variable("snd_pitchshift", &snd_pitchshift);

  m_bind_string_variable("music_pack_path", &music_pack_path);
  m_bind_string_variable("timidity_cfg_path", &timidity_cfg_path);
  m_bind_string_variable("gus_patch_path", &gus_patch_path);
  m_bind_int_variable("gus_ram_kb", &gus_ram_kb);

  m_bind_int_variable("use_libsamplerate", &use_libsamplerate);
  m_bind_float_variable("libsamplerate_scale", &libsamplerate_scale);
}
