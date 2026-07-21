/****************************************************************************
 * apps/games/NXDoom/src/i_rtttl_music.c
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
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>

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

#ifdef CONFIG_PWM
#include <nuttx/timers/pwm.h>
#endif

#include <audioutils/rtttl.h>

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static boolean i_rtttl_init_music(void);
static void i_rtttl_shutdown_music(void);
static void i_rtttl_set_music_volume(int volume);
static void i_rtttl_pause_song(void);
static void i_rtttl_resume_song(void);
static void *i_rtttl_register_song(void *data, int len);
static void i_rtttl_unregister_song(void *handle);
static void i_rtttl_play_song(void *handle, boolean looping);
static void i_rtttl_stop_song(void);
static boolean i_rtttl_musicisplaying(void);
static void i_rtttl_pollmusic(void);

/****************************************************************************
 * Private Data
 ****************************************************************************/

const char g_doom_rtttl[] =
  "Doom:d=32,o=5,b=56:f,f,f6,f,f,d#6,f,f,c#6,f,f,b,f,f,c6,c#6,f,f,f6,f,f,d#"
  "6,f,f,c#6,f,f,8b.,f,f,f6,f,f,d#6,f,f,c#6,f,f,b,f,f,c6,c#6,f,f,f6,f,f,d#"
  "6,f,f,c#6,f,f,8b.,a#,a#,a#6,a#,a#,g#6,a#,a#,f#6,a#,a#,e6,a#,a#,f6,f#6,a#"
  ",a#,a#6,a#,a#,g#6,a#,a#,f#6,a#,a#,8e6";

/* List of sound devices that this sound module is used for.
 * It appears in i_sound.c that the default music device is 'SNDDEVICE_SB',
 * so I have just put that here. In reality, we may either wish to add an
 * additional sound device for RTTL, or put this as something else
 * (PCSPEAKER?). I don't know what most of the snddevice_t enum options
 * refer to.
 */

static const snddevice_t g_rtttl_sound_devices[] =
{
  SNDDEVICE_SB,
};

/* Player state */

static void (*g_player)(struct rtttl_tone);
static int g_fd = -1;
static bool g_playing = false;
static bool g_loop = true;

/* Player thread state */

static pthread_t g_thread;
static pthread_mutex_t g_player_lock;
static pthread_cond_t g_pausecond;
static pthread_cond_t g_playingcond;
static bool g_paused;

/****************************************************************************
 * Public Data
 ****************************************************************************/

const music_module_t g_rtttl_music_module =
{
  .sound_devices = g_rtttl_sound_devices,
  .num_sound_devices = arrlen(g_rtttl_sound_devices),
  .init = i_rtttl_init_music,
  .shutdown = i_rtttl_shutdown_music,
  .set_music_volume = i_rtttl_set_music_volume,
  .pause_music = i_rtttl_pause_song,
  .resume_music = i_rtttl_resume_song,
  .register_song = i_rtttl_register_song,
  .unregister_song = i_rtttl_unregister_song,
  .play_song = i_rtttl_play_song,
  .stop_song = i_rtttl_stop_song,
  .music_is_playing = i_rtttl_musicisplaying,
  .poll = i_rtttl_pollmusic,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: wait_while_paused
 *
 * Description:
 *   Wait while the music is paused.
 *
 ****************************************************************************/

static void wait_while_paused(void)
{
  pthread_mutex_lock(&g_player_lock);

  while (g_paused)
    {
      pthread_cond_wait(&g_pausecond, &g_player_lock);
    }

  pthread_mutex_unlock(&g_player_lock);
}

/****************************************************************************
 * Name: wait_while_stopped
 *
 * Description:
 *   Wait until we are told that a song is started (i.e. while song stopped
 *   and nothing is playing).
 *
 ****************************************************************************/

static void wait_while_stopped(void)
{
  pthread_mutex_lock(&g_player_lock);

  while (!g_playing)
    {
      pthread_cond_wait(&g_playingcond, &g_player_lock);
    }

  pthread_mutex_unlock(&g_player_lock);
}

/****************************************************************************
 * Name: play_default
 *
 * Description:
 *   Default player which does nothing.
 *
 * Input Parameters:
 *   sound - The sound to play
 *
 ****************************************************************************/

static void play_default(struct rtttl_tone sound)
{
  wait_while_paused();
  return;
}

#ifdef CONFIG_PWM

/****************************************************************************
 * Name: play_pwm
 *
 * Description:
 *   RTTTL player for audio sinks using PWM devices
 *
 * Input Parameters:
 *   sound - The sound to play
 *
 ****************************************************************************/

static void play_pwm(struct rtttl_tone sound)
{
  int err;
  int i;
  struct pwm_info_s info;

  wait_while_paused(); /* Wait until unpaused */

  /* If we are not meant to be playing (i.e. song was stopped), we should
   * return from this function.
   */

  if (!g_playing)
    {
      return;
    }

  info.channels[0].channel = 1;
  info.channels[0].duty = b16divi(uitoub16(50) - 1, 100); /* 50% duty cycle */
  info.channels[0].cpol = PWM_CPOL_NDEF;                  /* Default */
  info.channels[0].dcpol = PWM_DCPOL_NDEF;                /* Default */
  info.frequency = sound.frequency_100hz / 100;

  /* Copy settings to all channels */

  for (i = 1; i < CONFIG_PWM_NCHANNELS; i++)
    {
      info.channels[i] = info.channels[0];
      info.channels[i].channel = i + 1;
    }

  err = ioctl(g_fd, PWMIOC_SETCHARACTERISTICS, &info);
  if (err < 0)
    {
      fprintf(stderr, "Failed to set PWM characteristics: %d\n", errno);
      return;
    }

  err = ioctl(g_fd, PWMIOC_START, 0);
  if (err < 0)
    {
      fprintf(stderr, "Failed to start PWM: %d\n", errno);
      return;
    }

  usleep(sound.duration_us); /* Wait */

  err = ioctl(g_fd, PWMIOC_STOP, 0);
  if (err < 0)
    {
      fprintf(stderr, "Failed to stop PWM: %d\n", errno);
    }
}
#endif

static void *player_thread(void *arg)
{
  for (; ; )
    {
      /* If there is nothing playing (i.e. song stopped or done), block here
       * until the main DOOM player logic tells us it wants to play a song.
       */

      wait_while_stopped();

      rtttl_play(g_doom_rtttl, g_player);

      /* If we are not meant to loop, then after one play we will indicate
       * that we are 'not playing'. This will causes the thread to block.
       * Otherwise, this will continue forever.
       */

      if (!g_loop)
        {
          pthread_mutex_lock(&g_player_lock);
          g_playing = false;
          g_paused = false;
          pthread_mutex_unlock(&g_player_lock);
        }
    }

  return NULL;
}

/****************************************************************************
 * Name: i_rtttl_init_music
 *
 * Description:
 *   Choose the appropriate player implementation for RTTTl based on the
 *   configured music device.
 *
 * Returned Value:
 *   True on success, false otherwise.
 *
 ****************************************************************************/

static boolean i_rtttl_init_music(void)
{
  int err;
  g_player = play_default;
#ifdef CONFIG_PWM
  if (strstr(CONFIG_GAMES_NXDOOM_RTTTL_DEVICE, "pwm"))
    {
      g_player = play_pwm;
    }
#endif

  /* Initialize thread state */

  pthread_mutex_init(&g_player_lock, NULL);
  pthread_cond_init(&g_pausecond, NULL);
  pthread_cond_init(&g_playingcond, NULL);
  g_paused = true;
  g_loop = false;
  g_playing = false;

  /* Start the music playing thread */

  err = pthread_create(&g_thread, NULL, player_thread, NULL);
  if (err)
    {
      return false;
    }

  /* We only managed to match with the default player. */

  if (g_player == play_default)
    {
      return true;
    }

  /* If we're not using the default player, we need to open a file
   * descriptor.
   */

  g_fd = open(CONFIG_GAMES_NXDOOM_RTTTL_DEVICE, O_RDWR);
  if (g_fd < 0)
    {
      fprintf(stderr,
              "Couldn't open RTTL device '" CONFIG_GAMES_NXDOOM_RTTTL_DEVICE
              "': %d",
              errno);
      return false;
    }

  return true;
}

/****************************************************************************
 * Name: i_rtttl_shutdown_music
 *
 * Description:
 *   Turns off the music and de-initializes the module.
 *
 ****************************************************************************/

static void i_rtttl_shutdown_music(void)
{
  i_rtttl_stop_song();      /* Stop music */
  pthread_cancel(g_thread); /* Kill thread */
  close(g_fd);              /* Clean up device */
}

static void i_rtttl_set_music_volume(int volume)
{
  return; /* Currently not possible */
}

/****************************************************************************
 * Name: i_rtttl_pause_song
 *
 * Description:
 *   Pauses the currently playing song. Right now, this is implemented by
 *   causing the RTTL `play` function to block on a condition variable.
 *
 ****************************************************************************/

static void i_rtttl_pause_song(void)
{
  pthread_mutex_lock(&g_player_lock);
  g_paused = true;
  pthread_mutex_unlock(&g_player_lock);
}

/****************************************************************************
 * Name: i_rtttl_resume_song
 *
 * Description:
 *   Resume the currently paused song. Unblocks the RTTL `play` function by
 *   signalling the condition variable.
 *
 ****************************************************************************/

static void i_rtttl_resume_song(void)
{
  pthread_mutex_lock(&g_player_lock);
  g_paused = false;
  pthread_cond_signal(&g_pausecond);
  pthread_mutex_unlock(&g_player_lock);
}

/****************************************************************************
 * Name: i_rtttl_register_song
 *
 * Description:
 *   Appears to be used to register song data from the DOOM lump. Right now,
 *   we are using a statically stored RTTL string so we just ignore this. It
 *   would be good to have RTTL data substituted for lump data in the future
 *   so we can play the different songs provided by DOOM WADS.
 *
 * Input Parameters:
 *   data - Song data
 *   len - The length of the song data in bytes
 *
 * Returns: In theory, a handle representing the registered song. However, in
 * practice, we return NULL to indicate this function isn't doing anything.
 *
 ****************************************************************************/

static void *i_rtttl_register_song(void *data, int len)
{
  /* TODO: no idea what this would be for; right now we play only the theme */

  return NULL;
}

/****************************************************************************
 * Name: i_rtttl_unregister_song
 *
 * Description:
 *   Unregisters one of the registered songs from `i_rtttl_register_song`.
 *   Currently unimplemented since we register nothing.
 *
 * Input Parameters:
 *   handle - The returned handle to the registered song
 *
 ****************************************************************************/

static void i_rtttl_unregister_song(void *handle)
{
  /* TODO: no idea what this would be for */

  return;
}

/****************************************************************************
 * Name: i_rtttl_play_song
 *
 * Description:
 *   Plays a previously registered song via the RTTTL library.
 *
 * Input Parameters:
 *   handle - The handle to a previously registered song
 *   looping - True if the song should loop, false otherwise
 *
 ****************************************************************************/

static void i_rtttl_play_song(void *handle, boolean looping)
{
  pthread_mutex_lock(&g_player_lock);
  g_playing = true;
  g_paused = false;
  g_loop = looping;
  pthread_cond_signal(&g_playingcond);
  pthread_mutex_unlock(&g_player_lock);
}

/****************************************************************************
 * Name: i_rtttl_stop_song
 *
 * Description:
 *   Stops playing the currently playing song (fully, not paused).
 *
 ****************************************************************************/

static void i_rtttl_stop_song(void)
{
  pthread_mutex_lock(&g_player_lock);
  g_playing = false;

  /* Song not paused, allows play function to return early by also unblocking
   * it. Then control is returned to the thread's main loop instead of the
   * loop inside the RTTL library thread, where the thread can then block
   * until a new song is played.
   */

  if (g_paused)
    {
      g_paused = false;
      pthread_cond_signal(&g_pausecond);
    }

  g_loop = false;
  pthread_mutex_unlock(&g_player_lock);
}

static boolean i_rtttl_musicisplaying(void)
{
  return g_playing;
}

static void i_rtttl_pollmusic(void)
{
  /* Unsure what this function does */

  return;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
