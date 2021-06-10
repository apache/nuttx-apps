/****************************************************************************
 * apps/system/nxplayer/nxplayer_main.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/audio/audio.h>

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "system/readline.h"
#include "system/nxplayer.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NXPLAYER_VER    "1.05"

#ifdef CONFIG_NXPLAYER_INCLUDE_HELP
#  define NXPLAYER_HELP_TEXT(x)  x
#else
#  define NXPLAYER_HELP_TEXT(x)
#endif

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

struct mp_cmd_s
{
  const char      *cmd;       /* The command text */
  const char      *arghelp;   /* Text describing the args */
  nxplayer_func    pfunc;     /* Pointer to command handler */
  const char      *help;      /* The help text */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int nxplayer_cmd_quit(FAR struct nxplayer_s *pplayer, char *parg);
static int nxplayer_cmd_play(FAR struct nxplayer_s *pplayer, char *parg);
static int nxplayer_cmd_playraw(FAR struct nxplayer_s *pplayer, char *parg);

#ifdef CONFIG_NXPLAYER_INCLUDE_SYSTEM_RESET
static int nxplayer_cmd_reset(FAR struct nxplayer_s *pplayer, char *parg);
#endif

#ifdef CONFIG_NXPLAYER_INCLUDE_PREFERRED_DEVICE
static int nxplayer_cmd_device(FAR struct nxplayer_s *pplayer, char *parg);
#endif

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
static int nxplayer_cmd_pause(FAR struct nxplayer_s *pplayer, char *parg);
static int nxplayer_cmd_resume(FAR struct nxplayer_s *pplayer, char *parg);
#endif

#ifdef CONFIG_NXPLAYER_INCLUDE_MEDIADIR
static int nxplayer_cmd_mediadir(FAR struct nxplayer_s *pplayer, char *parg);
#endif

#ifndef CONFIG_AUDIO_EXCLUDE_STOP
static int nxplayer_cmd_stop(FAR struct nxplayer_s *pplayer, char *parg);
#endif

#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
static int nxplayer_cmd_volume(FAR struct nxplayer_s *pplayer, char *parg);
#ifndef CONFIG_AUDIO_EXCLUDE_BALANCE
static int nxplayer_cmd_balance(FAR struct nxplayer_s *pplayer, char *parg);
#endif
#endif

#ifndef CONFIG_AUDIO_EXCLUDE_TONE
static int nxplayer_cmd_bass(FAR struct nxplayer_s *pplayer, char *parg);
static int nxplayer_cmd_treble(FAR struct nxplayer_s *pplayer, char *parg);
#endif

#ifdef CONFIG_NXPLAYER_INCLUDE_HELP
static int nxplayer_cmd_help(FAR struct nxplayer_s *pplayer, char *parg);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct mp_cmd_s g_nxplayer_cmds[] =
{
#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
#ifndef CONFIG_AUDIO_EXCLUDE_BALANCE
  {
    "balance",
    "d%",
    nxplayer_cmd_balance,
    NXPLAYER_HELP_TEXT("Set balance percentage (< 50% means more left)")
  },
#endif
#endif
#ifndef CONFIG_AUDIO_EXCLUDE_TONE
  {
    "bass",
    "d%",
    nxplayer_cmd_bass,
    NXPLAYER_HELP_TEXT("Set bass level percentage")
  },
#endif
#ifdef CONFIG_NXPLAYER_INCLUDE_PREFERRED_DEVICE
  {
    "device",
    "devfile",
    nxplayer_cmd_device,
    NXPLAYER_HELP_TEXT("Specify a preferred audio device")
  },
#endif
#ifdef CONFIG_NXPLAYER_INCLUDE_HELP
  {
    "h",
    "",
    nxplayer_cmd_help,
    NXPLAYER_HELP_TEXT("Display help for commands")
  },
  {
    "help",
    "",
    nxplayer_cmd_help,
    NXPLAYER_HELP_TEXT("Display help for commands")
  },
#endif
#ifdef CONFIG_NXPLAYER_INCLUDE_MEDIADIR
  {
    "mediadir",
    "path",
     nxplayer_cmd_mediadir,
     NXPLAYER_HELP_TEXT("Change the media directory")
  },
#endif
  {
    "play",
    "filename",
    nxplayer_cmd_play,
    NXPLAYER_HELP_TEXT("Play a media file")
  },
  {
    "playraw",
    "filename",
    nxplayer_cmd_playraw,
    NXPLAYER_HELP_TEXT("Play a raw data file")
  },
#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
  {
    "pause",
    "",
    nxplayer_cmd_pause,
    NXPLAYER_HELP_TEXT("Pause playback")
  },
#endif
#ifdef CONFIG_NXPLAYER_INCLUDE_SYSTEM_RESET
  {
    "reset",
    "",
    nxplayer_cmd_reset,
    NXPLAYER_HELP_TEXT("Perform a HW reset")
  },
#endif
#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
  {
    "resume",
    "",
    nxplayer_cmd_resume,
    NXPLAYER_HELP_TEXT("Resume playback")
  },
#endif
#ifndef CONFIG_AUDIO_EXCLUDE_STOP
  {
    "stop",
    "",
    nxplayer_cmd_stop,
    NXPLAYER_HELP_TEXT("Stop playback")
  },
#endif
  {
    "tone",
    "freq secs",
    NULL,
    NXPLAYER_HELP_TEXT("Produce a pure tone")
  },
#ifndef CONFIG_AUDIO_EXCLUDE_TONE
  {
    "treble",
    "d%",
    nxplayer_cmd_treble,
    NXPLAYER_HELP_TEXT("Set treble level percentage")
  },
#endif
  {
    "q",
    "",
    nxplayer_cmd_quit,
    NXPLAYER_HELP_TEXT("Exit NxPlayer")
  },
  {
    "quit",
    "",
    nxplayer_cmd_quit,
    NXPLAYER_HELP_TEXT("Exit NxPlayer")
  },
#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
  {
    "volume",
    "d%",
    nxplayer_cmd_volume,
    NXPLAYER_HELP_TEXT("Set volume to level specified")
  }
#endif
};

static const int g_nxplayer_cmd_count = sizeof(g_nxplayer_cmds) /
                                        sizeof(struct mp_cmd_s);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxplayer_cmd_play
 *
 *   nxplayer_cmd_play() plays the specified media file using the nxplayer
 *   context.
 *
 ****************************************************************************/

static int nxplayer_cmd_play(FAR struct nxplayer_s *pplayer, char *parg)
{
  int     ret;

  /* Try to play the file specified */

  ret = nxplayer_playfile(pplayer, parg, AUDIO_FMT_UNDEF, AUDIO_FMT_UNDEF);

  /* nxplayer_playfile returned values:
   *
   *   OK         File is being played
   *   -EBUSY     The media device is busy
   *   -ENOSYS    The media file is an unsupported type
   *   -ENODEV    No audio device suitable to play the media type
   *   -ENOENT    The media file was not found
   */

  switch (-ret)
    {
      case OK:
        break;

      case ENODEV:
        printf("No suitable Audio Device found\n");
        break;

      case EBUSY:
        printf("Audio device busy\n");
        break;

      case ENOENT:
        printf("File %s not found\n", parg);
        break;

      case ENOSYS:
        printf("Unknown audio format\n");
        break;

      default:
        printf("Error playing file: %d\n", -ret);
        break;
    }

  return ret;
}

/****************************************************************************
 * Name: nxplayer_cmd_playraw
 *
 *   nxplayer_cmd_play() plays the raw data file using the nxplayer
 *   context.
 *
 ****************************************************************************/

static int nxplayer_cmd_playraw(FAR struct nxplayer_s *pplayer, char *parg)
{
  int ret;
  int channels = 0;
  int bpsamp = 0;
  int samprate = 0;
  int chmap = 0;
  char filename[128];

  sscanf(parg, "%s %d %d %d %d", filename, &channels, &bpsamp,
                                 &samprate, &chmap);

  /* Try to play the file specified */

  ret = nxplayer_playraw(pplayer, filename, channels,
                         bpsamp, samprate, chmap);

  /* nxplayer_playfile returned values:
   *
   *   OK         File is being played
   *   -EBUSY     The media device is busy
   *   -ENOSYS    The media file is an unsupported type
   *   -ENODEV    No audio device suitable to play the media type
   *   -ENOENT    The media file was not found
   */

  switch (-ret)
    {
      case OK:
        break;

      case ENODEV:
        printf("No suitable Audio Device found\n");
        break;

      case EBUSY:
        printf("Audio device busy\n");
        break;

      case ENOENT:
        printf("File %s not found\n", filename);
        break;

      case ENOSYS:
        printf("Unknown audio format\n");
        break;

      default:
        printf("Error playing file: %d\n", -ret);
        break;
    }

  return ret;
}

/****************************************************************************
 * Name: nxplayer_cmd_volume
 *
 *   nxplayer_cmd_volume() sets the volume level.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
static int nxplayer_cmd_volume(FAR struct nxplayer_s *pplayer, char *parg)
{
  uint16_t   percent;

  /* If no arg given, then print current volume */

  if (parg == NULL || *parg == '\0')
    {
      printf("volume: %d\n", pplayer->volume / 10);
    }
  else
    {
      /* Get the percentage value from the argument */

      percent = (uint16_t) (atof(parg) * 10.0);
      nxplayer_setvolume(pplayer, percent);
    }

  return OK;
}
#endif

/****************************************************************************
 * Name: nxplayer_cmd_bass
 *
 *   nxplayer_cmd_bass() sets the bass level and range.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_TONE
static int nxplayer_cmd_bass(FAR struct nxplayer_s *pplayer, char *parg)
{
  uint8_t   level_percent;

  /* If no arg given, then print current bass */

  if (parg == NULL || *parg == '\0')
    {
      printf("bass: %d\n", pplayer->bass);
    }
  else
    {
      /* Get the level and range percentage value from the argument */

      level_percent = (uint8_t) atoi(parg);
      nxplayer_setbass(pplayer, level_percent);
    }

  return OK;
}
#endif

/****************************************************************************
 * Name: nxplayer_cmd_treble
 *
 *   nxplayer_cmd_treble() sets the treble level and range.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_TONE
static int nxplayer_cmd_treble(FAR struct nxplayer_s *pplayer, char *parg)
{
  uint8_t   level_percent;

  /* If no arg given, then print current bass */

  if (parg == NULL || *parg == '\0')
    {
      printf("treble: %d\n", pplayer->treble);
    }
  else
    {
      /* Get the level and range percentage value from the argument */

      level_percent = (uint8_t) atoi(parg);
      nxplayer_settreble(pplayer, level_percent);
    }

  return OK;
}
#endif

/****************************************************************************
 * Name: nxplayer_cmd_balance
 *
 *   nxplayer_cmd_balance() sets the balance level.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
#ifndef CONFIG_AUDIO_EXCLUDE_BALANCE
static int nxplayer_cmd_balance(FAR struct nxplayer_s *pplayer, char *parg)
{
  uint16_t   percent;

  /* If no arg given, then print current volume */

  if (parg == NULL || *parg == '\0')
    {
      printf("balance: %d\n", pplayer->volume / 10);
    }
  else
    {
      /* Get the percentage value from the argument */

      percent = (uint16_t) (atof(parg) * 10.0);
      nxplayer_setbalance(pplayer, percent);
    }

  return OK;
}
#endif
#endif

/****************************************************************************
 * Name: nxplayer_cmd_reset
 *
 *   nxplayer_cmd_reset() performs a HW reset of all the audio devices.
 *
 ****************************************************************************/

#ifdef CONFIG_NXPLAYER_INCLUDE_SYSTEM_RESET
static int nxplayer_cmd_reset(FAR struct nxplayer_s *pplayer, char *parg)
{
  nxplayer_systemreset(pplayer);

  return OK;
}
#endif

/****************************************************************************
 * Name: nxplayer_cmd_mediadir
 *
 *   nxplayer_cmd_mediadir() displays or changes the media directory
 *   context.
 *
 ****************************************************************************/

#ifdef CONFIG_NXPLAYER_INCLUDE_MEDIADIR
static int nxplayer_cmd_mediadir(FAR struct nxplayer_s *pplayer, char *parg)
{
  /* If no arg given, then print current media dir */

  if (parg == NULL || *parg == '\0')
    printf("%s\n", pplayer->mediadir);
  else
    nxplayer_setmediadir(pplayer, parg);

  return OK;
}
#endif

/****************************************************************************
 * Name: nxplayer_cmd_stop
 *
 *   nxplayer_cmd_stop() stops playback of currently playing file
 *   context.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_STOP
static int nxplayer_cmd_stop(FAR struct nxplayer_s *pplayer, char *parg)
{
  /* Stop the playback */

  nxplayer_stop(pplayer);

  return OK;
}
#endif

/****************************************************************************
 * Name: nxplayer_cmd_pause
 *
 *   nxplayer_cmd_pause() pauses playback of currently playing file
 *   context.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
static int nxplayer_cmd_pause(FAR struct nxplayer_s *pplayer, char *parg)
{
  /* Pause the playback */

  nxplayer_pause(pplayer);

  return OK;
}
#endif

/****************************************************************************
 * Name: nxplayer_cmd_resume
 *
 *   nxplayer_cmd_resume() resumes playback of currently playing file
 *   context.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
static int nxplayer_cmd_resume(FAR struct nxplayer_s *pplayer, char *parg)
{
  /* Resume the playback */

  nxplayer_resume(pplayer);

  return OK;
}
#endif

/****************************************************************************
 * Name: nxplayer_cmd_device
 *
 *   nxplayer_cmd_device() sets the preferred audio device for playback
 *
 ****************************************************************************/

#ifdef CONFIG_NXPLAYER_INCLUDE_PREFERRED_DEVICE
static int nxplayer_cmd_device(FAR struct nxplayer_s *pplayer, char *parg)
{
  int     ret;
  char    path[32];

  /* First try to open the file directly */

  ret = nxplayer_setdevice(pplayer, parg);
  if (ret == -ENOENT)
    {
      /* Append the /dev/audio path and try again */

#ifdef CONFIG_AUDIO_CUSTOM_DEV_PATH
#ifdef CONFIG_AUDIO_DEV_ROOT
      snprintf(path,  sizeof(path), "/dev/%s", parg);
#else
      snprintf(path,  sizeof(path), CONFIG_AUDIO_DEV_PATH "/%s", parg);
#endif
#else
      snprintf(path, sizeof(path), "/dev/audio/%s", parg);
#endif
      ret = nxplayer_setdevice(pplayer, path);
    }

  /* Test if the device file exists */

  if (ret == -ENOENT)
    {
      /* Device doesn't exit.  Report error */

      printf("Device %s not found\n", parg);
      return ret;
    }

  /* Test if is is an audio device */

  if (ret == -ENODEV)
    {
      printf("Device %s is not an audio device\n", parg);
      return ret;
    }

  if (ret < 0)
    {
      return ret;
    }

  /* Device set successfully */

  return OK;
}
#endif /* CONFIG_NXPLAYER_INCLUDE_PREFERRED_DEVICE */

/****************************************************************************
 * Name: nxplayer_cmd_quit
 *
 *   nxplayer_cmd_quit() terminates the application
 ****************************************************************************/

static int nxplayer_cmd_quit(FAR struct nxplayer_s *pplayer, char *parg)
{
  /* Stop the playback if any */

#ifndef CONFIG_AUDIO_EXCLUDE_STOP
  nxplayer_stop(pplayer);
#endif

  return OK;
}

/****************************************************************************
 * Name: nxplayer_cmd_help
 *
 *   nxplayer_cmd_help() displays the application's help information on
 *   supported commands and command syntax.
 ****************************************************************************/

#ifdef CONFIG_NXPLAYER_INCLUDE_HELP
static int nxplayer_cmd_help(FAR struct nxplayer_s *pplayer, char *parg)
{
  int   len;
  int   maxlen = 0;
  int   x;
  int   c;

  /* Calculate length of longest cmd + arghelp */

  for (x = 0; x < g_nxplayer_cmd_count; x++)
    {
      len = strlen(g_nxplayer_cmds[x].cmd) +
                   strlen(g_nxplayer_cmds[x].arghelp);
      if (len > maxlen)
        {
          maxlen = len;
        }
    }

  printf("NxPlayer commands\n================\n");
  for (x = 0; x < g_nxplayer_cmd_count; x++)
    {
      /* Print the command and it's arguments */

      printf("  %s %s", g_nxplayer_cmds[x].cmd, g_nxplayer_cmds[x].arghelp);

      /* Calculate number of spaces to print before the help text */

      len = maxlen - (strlen(g_nxplayer_cmds[x].cmd) +
                      strlen(g_nxplayer_cmds[x].arghelp));
      for (c = 0; c < len; c++)
        {
          printf(" ");
        }

      printf("  : %s\n", g_nxplayer_cmds[x].help);
    }

  return OK;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxplayer
 *
 *   nxplayer() reads in commands from the console using the readline
 *   system add-in and impalements a command-line based media player that
 *   uses the NuttX audio system to play media files read in from the
 *   file system.  Commands are provided for setting volume, base and
 *   other audio features, as well as for pausing and stopping the
 *   playback.
 *
 * Input Parameters:
 *   buf       - The user allocated buffer to be filled.
 *   buflen    - the size of the buffer.
 *   instream  - The stream to read characters from
 *   outstream - The stream to each characters to.
 *
 * Returned values:
 *   On success, the (positive) number of bytes transferred is returned.
 *   EOF is returned to indicate either an end of file condition or a
 *   failure.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  char                    buffer[CONFIG_NSH_LINELEN];
  int                     len, x, running;
  char                    *cmd, *arg;
  FAR struct nxplayer_s   *pplayer;

  printf("NxPlayer version " NXPLAYER_VER "\n");
  printf("h for commands, q to exit\n");
  printf("\n");

  /* Initialize our NxPlayer context */

  pplayer = nxplayer_create();
  if (pplayer == NULL)
    {
      printf("Error:  Out of RAM\n");
      return -ENOMEM;
    }

  /* Loop until the user exits */

  running = TRUE;
  while (running)
    {
      /* Print a prompt */

      printf("nxplayer> ");
      fflush(stdout);

      /* Read a line from the terminal */

      len = readline(buffer, sizeof(buffer), stdin, stdout);
      buffer[len] = '\0';
      if (len > 0)
        {
          if (strncmp(buffer, "!", 1) != 0)
            {
              /* nxplayer command */

              if (buffer[len - 1] == '\n')
                {
                  buffer[len - 1] = '\0';
                }

              /* Parse the command from the argument */

              cmd = strtok_r(buffer, " \n", &arg);
              if (cmd == NULL)
                {
                  continue;
                }

              /* Remove leading spaces from arg */

              while (*arg == ' ')
                {
                  arg++;
                }

              /* Find the command in our cmd array */

              for (x = 0; x < g_nxplayer_cmd_count; x++)
                {
                  if (strcmp(cmd, g_nxplayer_cmds[x].cmd) == 0)
                    {
                      /* Command found.  Call it's handler if not NULL */

                      if (g_nxplayer_cmds[x].pfunc != NULL)
                        {
                          g_nxplayer_cmds[x].pfunc(pplayer, arg);
                        }

                      /* Test if it is a quit command */

                      if (g_nxplayer_cmds[x].pfunc == nxplayer_cmd_quit)
                        {
                          running = FALSE;
                        }

                      break;
                    }
                }
            }
          else
            {
#ifdef CONFIG_SYSTEM_SYSTEM
              /* Transfer nuttx shell */

              system(buffer + 1);
#else
              printf("%s:  unknown nxplayer command\n", buffer);
#endif
            }
        }
    }

  /* Release the NxPlayer context */

  /* nxplayer_detach(pplayer); */

  nxplayer_release(pplayer);

  return OK;
}
