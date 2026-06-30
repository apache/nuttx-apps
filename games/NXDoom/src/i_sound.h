/****************************************************************************
 * apps/games/NXDoom/src/i_sound.h
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

#ifndef __I_SOUND__
#define __I_SOUND__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "d_mode.h"
#include "doomtype.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* so that the individual game logic and sound driver code agree */

#define NORM_PITCH 127

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* SoundFX struct. */

typedef struct sfxinfo_struct sfxinfo_t;

struct sfxinfo_struct
{
  /* tag name, used for hexen. */

  const char *tagname;

  /* lump name.  If we are running with use_sfx_prefix=true, a
   * 'DS' (or 'DP' for PC speaker sounds) is prepended to this.
   */

  char name[9];

  /* Sfx priority */

  int priority;

  /* referenced sound if a link */

  sfxinfo_t *link;

  /* pitch if a link (Doom), whether to pitch-shift (Hexen) */

  int pitch;

  /* volume if a link */

  int volume;

  /* this is checked every second to see if sound
   * can be thrown out (if 0, then decrement, if -1,
   * then throw out, if > 0, then it is in use)
   */

  int usefulness;

  /* lump number of sfx */

  int lumpnum;

  /* Maximum number of channels that the sound can be played on
   * (Heretic)
   */

  int numchannels;

  /* data used by the low level code */

  void *driver_data;
};

/* MusicInfo struct. */

typedef struct
{
  /* up to 6-character name */

  const char *name;

  /* lump number of music */

  int lumpnum;

  /* music data */

  void *data;

  /* music handle once registered */

  void *handle;
} musicinfo_t;

typedef enum
{
  SNDDEVICE_NONE = 0,
  SNDDEVICE_PCSPEAKER = 1,
  SNDDEVICE_ADLIB = 2,
  SNDDEVICE_SB = 3,
  SNDDEVICE_PAS = 4,
  SNDDEVICE_GUS = 5,
  SNDDEVICE_WAVEBLASTER = 6,
  SNDDEVICE_SOUNDCANVAS = 7,
  SNDDEVICE_GENMIDI = 8,
  SNDDEVICE_AWE32 = 9,
  SNDDEVICE_CD = 10,
  SNDDEVICE_FSYNTH = 11,
} snddevice_t;

/* Interface for sound modules */

typedef struct
{
  /* List of sound devices that this sound module is used for. */

  const snddevice_t *sound_devices;
  int num_sound_devices;

  /* initialise sound module
   * Returns true if successfully initialised
   */

  boolean (*init)(gamemission_t mission);

  /* shutdown sound module */

  void (*shutdown)(void);

  /* Returns the lump index of the given sound. */

  int (*get_sfx_lumpnum)(sfxinfo_t *sfxinfo);

  /* Called periodically to update the subsystem. */

  void (*update)(void);

  /* Update the sound settings on the given channel. */

  void (*update_sound_params)(int channel, int vol, int sep);

  /* Start a sound on a given channel.  Returns the channel id
   * or -1 on failure.
   */

  int (*start_sound)(sfxinfo_t *sfxinfo, int channel, int vol, int sep,
                    int pitch);

  /* Stop the sound playing on the given channel. */

  void (*stop_sound)(int channel);

  /* Query if a sound is playing on the given channel */

  boolean (*sound_is_playing)(int channel);

  /* Called on startup to precache sound effects (if necessary) */

  void (*cache_sounds)(sfxinfo_t *sounds, int num_sounds);
} sound_module_t;

/* Interface for music modules */

typedef struct
{
  /* List of sound devices that this music module is used for. */

  const snddevice_t *sound_devices;
  int num_sound_devices;

  /* initialise the music subsystem */

  boolean (*init)(void);

  /* shutdown the music subsystem */

  void (*shutdown)(void);

  /* Set music volume - range 0-127 */

  void (*set_music_volume)(int volume);

  /* Pause music */

  void (*pause_music)(void);

  /* Un-pause music */

  void (*resume_music)(void);

  /* Register a song handle from data
   * Returns a handle that can be used to play the song
   */

  void *(*register_song)(void *data, int len);

  /* Un-register (free) song data */

  void (*unregister_song)(void *handle);

  /* Play the song */

  void (*play_song)(void *handle, boolean looping);

  /* Stop playing the current song. */

  void (*stop_song)(void);

  /* Query if music is playing. */

  boolean (*music_is_playing)(void);

  /* Invoked periodically to poll. */

  void (*poll)(void);
} music_module_t;

#if 0
/* DMX version to emulate for OPL emulation: */

typedef enum
{
  opl_doom1_1_666, /* Doom 1 v1.666 */
  opl_doom2_1_666, /* Doom 2 v1.666, Hexen, Heretic */
  opl_doom_1_9     /* Doom v1.9, Strife */
} opl_driver_ver_t;
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern int snd_sfxdevice;
extern int snd_musicdevice;
extern int snd_samplerate;
extern int snd_cachesize;
extern int snd_maxslicetime_ms;
extern char *snd_musiccmd;
extern int snd_pitchshift;
extern int use_libsamplerate;
extern float libsamplerate_scale;

extern const sound_module_t sound_sdl_module;
extern const sound_module_t sound_pcsound_module;
extern const music_module_t music_sdl_module;
extern const music_module_t music_pack_module;
extern const music_module_t music_fl_module;

/* For native music module: */

extern char *music_pack_path;
extern char *timidity_cfg_path;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void i_init_sound(gamemission_t mission);
void i_shutdown_sound(void);
int i_get_sfx_lumpnum(sfxinfo_t *sfxinfo);
void i_update_sound(void);
void i_update_sound_params(int channel, int vol, int sep);
int i_start_sound(sfxinfo_t *sfxinfo, int channel, int vol, int sep,
                  int pitch);
void i_stop_sound(int channel);
boolean i_sound_playing(int channel);
void i_precache_sounds(sfxinfo_t *sounds, int num_sounds);

void i_init_music(void);
void i_shutdown_music(void);
void i_set_music_volume(int volume);
void i_pause_song(void);
void i_resume_song(void);
void *i_register_song(void *data, int len);
void i_unregister_song(void *handle);
void i_play_song(void *handle, boolean looping);
void i_stop_song(void);

void i_bind_sound_variables(void);

#if 0
void i_set_opl_driver_ver(opl_driver_ver_t ver);
void i_opl_dev_messages(char *, size_t);
#endif

/* Sound modules */

void i_init_timidity_config(void);

#endif /* __I_SOUND__ */
