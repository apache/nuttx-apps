/****************************************************************************
 * apps/system/nxlooper/nxlooper_main.c
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
#include "system/nxlooper.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NXLOOPER_VER    "1.00"

#ifdef CONFIG_NXLOOPER_INCLUDE_HELP
#  define NXLOOPER_HELP_TEXT(x)  x
#else
#  define NXLOOPER_HELP_TEXT(x)
#endif

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

typedef int (*nxlooper_func)(FAR struct nxlooper_s *plooper, char *pargs);

struct mp_cmd_s
{
  FAR const char *cmd;       /* The command text */
  FAR const char *arghelp;   /* Text describing the args */
  nxlooper_func  pfunc;      /* Pointer to command handler */
  FAR const char *help;      /* The help text */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int nxlooper_cmd_quit(FAR struct nxlooper_s *plooper, char *parg);
static int nxlooper_cmd_loopback(FAR struct nxlooper_s *plooper, char *parg);

#ifdef CONFIG_NXLOOPER_INCLUDE_SYSTEM_RESET
static int nxlooper_cmd_reset(FAR struct nxlooper_s *plooper, char *parg);
#endif

#ifdef CONFIG_NXLOOPER_INCLUDE_PREFERRED_DEVICE
static int nxlooper_cmd_device(FAR struct nxlooper_s *plooper, char *parg);
#endif

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
static int nxlooper_cmd_pause(FAR struct nxlooper_s *plooper, char *parg);
static int nxlooper_cmd_resume(FAR struct nxlooper_s *plooper, char *parg);
#endif

#ifndef CONFIG_AUDIO_EXCLUDE_STOP
static int nxlooper_cmd_stop(FAR struct nxlooper_s *plooper, char *parg);
#endif

#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
static int nxlooper_cmd_volume(FAR struct nxlooper_s *plooper, char *parg);
#endif

#ifdef CONFIG_NXLOOPER_INCLUDE_HELP
static int nxlooper_cmd_help(FAR struct nxlooper_s *plooper, char *parg);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct mp_cmd_s g_nxlooper_cmds[] =
{
#ifdef CONFIG_NXLOOPER_INCLUDE_PREFERRED_DEVICE
  {
    "device",
    "devfile",
    nxlooper_cmd_device,
    NXLOOPER_HELP_TEXT("Specify a preferred play/record device")
  },
#endif
#ifdef CONFIG_NXLOOPER_INCLUDE_HELP
  {
    "h",
    "",
    nxlooper_cmd_help,
    NXLOOPER_HELP_TEXT("Display help for commands")
  },
  {
    "help",
    "",
    nxlooper_cmd_help,
    NXLOOPER_HELP_TEXT("Display help for commands")
  },
#endif
  {
    "loopback",
    "channels bpsamp samprate chmap",
    nxlooper_cmd_loopback,
    NXLOOPER_HELP_TEXT("Audio loopback test")
  },
#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
  {
    "pause",
    "",
    nxlooper_cmd_pause,
    NXLOOPER_HELP_TEXT("Pause loopback")
  },
#endif
#ifdef CONFIG_NXLOOPER_INCLUDE_SYSTEM_RESET
  {
    "reset",
    "",
    nxlooper_cmd_reset,
    NXLOOPER_HELP_TEXT("Perform a HW reset")
  },
#endif
#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
  {
    "resume",
    "",
    nxlooper_cmd_resume,
    NXLOOPER_HELP_TEXT("Resume loopback")
  },
#endif
#ifndef CONFIG_AUDIO_EXCLUDE_STOP
  {
    "stop",
    "",
    nxlooper_cmd_stop,
    NXLOOPER_HELP_TEXT("Stop loopback")
  },
#endif
  {
    "q",
    "",
    nxlooper_cmd_quit,
    NXLOOPER_HELP_TEXT("Exit NxLooper")
  },
  {
    "quit",
    "",
    nxlooper_cmd_quit,
    NXLOOPER_HELP_TEXT("Exit NxLooper")
  },
#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
  {
    "volume",
    "d%",
    nxlooper_cmd_volume,
    NXLOOPER_HELP_TEXT("Set volume to level specified")
  }
#endif
};

static const int g_nxlooper_cmd_count = sizeof(g_nxlooper_cmds) /
                                        sizeof(struct mp_cmd_s);

/****************************************************************************
 * Name: nxlooper_cmd_loopback
 *
 *   nxlooper_cmd_loop() play and record the raw data file using the nxlooper
 *   context.
 *
 ****************************************************************************/

static int nxlooper_cmd_loopback(FAR struct nxlooper_s *plooper, char *parg)
{
  int ret;
  int channels = 0;
  int bpsamp = 0;
  int samprate = 0;
  int chmap = 0;

  sscanf(parg, "%d %d %d %d", &channels, &bpsamp,
                                 &samprate, &chmap);

  /* Try to loopback raw data with settings specified */

  ret = nxlooper_loopraw(plooper, channels, bpsamp,
                         samprate, chmap);

  /* nxlooper_loopraw returned values:
   *
   *   OK         Loopback is being run
   *   -EBUSY     The media device is busy
   *   -ENOSYS    The audio format is not unsupported
   *   -ENODEV    No audio device suitable to play and record the media type
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

      case ENOSYS:
        printf("Unknown audio format\n");
        break;

      default:
        printf("Error loopback test: %d\n", -ret);
        break;
    }

  return ret;
}

/****************************************************************************
 * Name: nxlooper_cmd_volume
 *
 *   nxlooper_cmd_volume() sets the volume level.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
static int nxlooper_cmd_volume(FAR struct nxlooper_s *plooper, char *parg)
{
  uint16_t percent;

  /* If no arg given, then print current volume */

  if (parg == NULL || *parg == '\0')
    {
      printf("volume: %d\n", plooper->volume / 10);
    }
  else
    {
      /* Get the percentage value from the argument */

      percent = (uint16_t)(atof(parg) * 10.0);
      return nxlooper_setvolume(plooper, percent);
    }

  return OK;
}
#endif

/****************************************************************************
 * Name: nxlooper_cmd_reset
 *
 *   nxlooper_cmd_reset() performs a HW reset of all the audio devices.
 *
 ****************************************************************************/

#ifdef CONFIG_NXLOOPER_INCLUDE_SYSTEM_RESET
static int nxlooper_cmd_reset(FAR struct nxlooper_s *plooper, char *parg)
{
  return nxlooper_systemreset(plooper);
}
#endif

/****************************************************************************
 * Name: nxlooper_cmd_stop
 *
 *   nxlooper_cmd_stop() stops loopback
 *   context.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_STOP
static int nxlooper_cmd_stop(FAR struct nxlooper_s *plooper, char *parg)
{
  /* Stop the loopback */

  return nxlooper_stop(plooper);
}
#endif

/****************************************************************************
 * Name: nxlooper_cmd_pause
 *
 *   nxlooper_cmd_pause() pauses loopback
 *   context.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
static int nxlooper_cmd_pause(FAR struct nxlooper_s *plooper, char *parg)
{
  /* Pause the loopback */

  return nxlooper_pause(plooper);
}
#endif

/****************************************************************************
 * Name: nxlooper_cmd_resume
 *
 *   nxlooper_cmd_resume() resumes loopback
 *   context.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
static int nxlooper_cmd_resume(FAR struct nxlooper_s *plooper, char *parg)
{
  /* Resume the loopback */

  return nxlooper_resume(plooper);
}
#endif

/****************************************************************************
 * Name: nxlooper_cmd_device
 *
 *   nxlooper_cmd_device() sets the preferred audio device for loopback
 *
 ****************************************************************************/

#ifdef CONFIG_NXLOOPER_INCLUDE_PREFERRED_DEVICE
static int nxlooper_cmd_device(FAR struct nxlooper_s *plooper, char *parg)
{
  int  ret;
  char path[PATH_MAX];

  /* First try to open the file directly */

  ret = nxlooper_setdevice(plooper, parg);
  if (ret == -ENOENT)
    {
      /* Append the /dev/audio path and try again */

#ifdef CONFIG_AUDIO_CUSTOM_DEV_PATH
#ifdef CONFIG_AUDIO_DEV_ROOT
      snprintf(path, sizeof(path), "/dev/%s", parg);
#else
      snprintf(path, sizeof(path), CONFIG_AUDIO_DEV_PATH "/%s", parg);
#endif
#else
      snprintf(path, sizeof(path), "/dev/audio/%s", parg);
#endif
      ret = nxlooper_setdevice(plooper, path);
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
#endif /* CONFIG_NXLOOPER_INCLUDE_PREFERRED_DEVICE */

/****************************************************************************
 * Name: nxlooper_cmd_quit
 *
 *   nxlooper_cmd_quit() terminates the application
 ****************************************************************************/

static int nxlooper_cmd_quit(FAR struct nxlooper_s *plooper, char *parg)
{
  /* Stop the loopback if any */

#ifndef CONFIG_AUDIO_EXCLUDE_STOP
  return nxlooper_stop(plooper);
#endif

  return OK;
}

/****************************************************************************
 * Name: nxlooper_cmd_help
 *
 *   nxlooper_cmd_help() displays the application's help information on
 *   supported commands and command syntax.
 ****************************************************************************/

#ifdef CONFIG_NXLOOPER_INCLUDE_HELP
static int nxlooper_cmd_help(FAR struct nxlooper_s *plooper, char *parg)
{
  int len;
  int maxlen = 0;
  int x;
  int c;

  /* Calculate length of longest cmd + arghelp */

  for (x = 0; x < g_nxlooper_cmd_count; x++)
    {
      len = strlen(g_nxlooper_cmds[x].cmd) +
                   strlen(g_nxlooper_cmds[x].arghelp);
      if (len > maxlen)
        {
          maxlen = len;
        }
    }

  printf("NxLooper commands\n================\n");
  for (x = 0; x < g_nxlooper_cmd_count; x++)
    {
      /* Print the command and it's arguments */

      printf("  %s %s", g_nxlooper_cmds[x].cmd, g_nxlooper_cmds[x].arghelp);

      /* Calculate number of spaces to print before the help text */

      len = maxlen - (strlen(g_nxlooper_cmds[x].cmd) +
                      strlen(g_nxlooper_cmds[x].arghelp));
      for (c = 0; c < len; c++)
        {
          printf(" ");
        }

      printf("  : %s\n", g_nxlooper_cmds[x].help);
    }

  return OK;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxlooper
 *
 *   nxlooper() reads in commands from the console using the readline
 *   system add-in and impalements a command-line based media loop
 *   tester that uses the NuttX audio system to record and then play
 *   raw data from the lower audio driver. Commands are provided for
 *   setting volume, base and other audio features, as well as for
 *   pausing and stopping the test.
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
  char                  buffer[CONFIG_NSH_LINELEN];
  int                   len;
  int                   x;
  int                   running;
  FAR char              *cmd;
  FAR char              *arg;
  FAR struct nxlooper_s *plooper;

  printf("NxLooper version " NXLOOPER_VER "\n");
  printf("h for commands, q to exit\n");
  printf("\n");

  /* Initialize our NxLooper context */

  plooper = nxlooper_create();
  if (plooper == NULL)
    {
      printf("Error: Out of RAM\n");
      return -ENOMEM;
    }

  /* Loop until the user exits */

  running = TRUE;
  while (running)
    {
      /* Print a prompt */

      printf("nxlooper> ");
      fflush(stdout);

      /* Read a line from the terminal */

      len = readline(buffer, sizeof(buffer), stdin, stdout);
      buffer[len] = '\0';
      if (len > 0)
        {
          if (strncmp(buffer, "!", 1) != 0)
            {
              /* nxlooper command */

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

              /* Find the command in our cmd array */

              for (x = 0; x < g_nxlooper_cmd_count; x++)
                {
                  if (strcmp(cmd, g_nxlooper_cmds[x].cmd) == 0)
                    {
                      /* Command found.  Call it's handler if not NULL */

                      if (g_nxlooper_cmds[x].pfunc != NULL)
                        {
                          g_nxlooper_cmds[x].pfunc(plooper, arg);
                        }

                      /* Test if it is a quit command */

                      if (g_nxlooper_cmds[x].pfunc == nxlooper_cmd_quit)
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
              printf("%s: unknown nxlooper command\n", buffer);
#endif
            }
        }
    }

  /* Release the NxLooper context */

  nxlooper_release(plooper);

  return OK;
}
