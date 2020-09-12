/****************************************************************************
 * apps/system/nxrecorder/nxrecorder_main.c
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
#include "system/nxrecorder.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NXRECORDER_VER          "1.00"

#ifdef CONFIG_NXRECORDER_INCLUDE_HELP
#  define NXRECORDER_HELP_TEXT(x)  x
#else
#  define NXRECORDER_HELP_TEXT(x)
#endif

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

struct mp_cmd_s
{
  const char      *cmd;       /* The command text */
  const char      *arghelp;   /* Text describing the args */
  nxrecorder_func pfunc;      /* Pointer to command handler */
  const char      *help;      /* The help text */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int nxrecorder_cmd_quit(FAR struct nxrecorder_s *precorder,
                               FAR char *parg);
static int nxrecorder_cmd_recordraw(FAR struct nxrecorder_s *precorder,
                                    FAR char *parg);
static int nxrecorder_cmd_device(FAR struct nxrecorder_s *precorder,
                                 FAR char *parg);

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
static int nxrecorder_cmd_pause(FAR struct nxrecorder_s *precorder,
                                FAR char *parg);
static int nxrecorder_cmd_resume(FAR struct nxrecorder_s *precorder,
                                 FAR char *parg);
#endif

#ifndef CONFIG_AUDIO_EXCLUDE_STOP
static int nxrecorder_cmd_stop(FAR struct nxrecorder_s *precorder,
                               FAR char *parg);
#endif

#ifdef CONFIG_NXRECORDER_INCLUDE_HELP
static int nxrecorder_cmd_help(FAR struct nxrecorder_s *precorder,
                               FAR char *parg);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct mp_cmd_s g_nxrecorder_cmds[] =
{
  {
    "device",
    "devfile",
    nxrecorder_cmd_device,
    NXRECORDER_HELP_TEXT("Specify a preferred audio device")
  },
#ifdef CONFIG_NXRECORDER_INCLUDE_HELP
  {
    "h",
    "",
    nxrecorder_cmd_help,
    NXRECORDER_HELP_TEXT("Display help for commands")
  },
  {
    "help",
    "",
    nxrecorder_cmd_help,
    NXRECORDER_HELP_TEXT("Display help for commands")
  },
#endif
  {
    "recordraw",
    "filename",
    nxrecorder_cmd_recordraw,
    NXRECORDER_HELP_TEXT("Record a pcm raw file")
  },
#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
  {
    "pause",
    "",
    nxrecorder_cmd_pause,
    NXRECORDER_HELP_TEXT("Pause record")
  },
  {
    "resume",
    "",
    nxrecorder_cmd_resume,
    NXRECORDER_HELP_TEXT("Resume record")
  },
#endif
#ifndef CONFIG_AUDIO_EXCLUDE_STOP
  {
    "stop",
    "",
    nxrecorder_cmd_stop,
    NXRECORDER_HELP_TEXT("Stop record")
  },
#endif
  {
    "q",
    "",
    nxrecorder_cmd_quit,
    NXRECORDER_HELP_TEXT("Exit NxRecorder")
  },
  {
    "quit",
    "",
    nxrecorder_cmd_quit,
    NXRECORDER_HELP_TEXT("Exit NxRecorder")
  },
};

static const int g_nxrecorder_cmd_count = sizeof(g_nxrecorder_cmds) /
                                          sizeof(struct mp_cmd_s);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxrecorder_cmd_recordraw
 *
 *   nxrecorder_cmd_recordraw() records the raw data file using the
 *   nxrecorder context.
 *
 ****************************************************************************/

static int nxrecorder_cmd_recordraw(FAR struct nxrecorder_s *precorder,
                                    FAR char *parg)
{
  int ret;
  int channels = 0;
  int bpsamp = 0;
  int samprate = 0;
  int chmap = 0;
  char filename[128];

  sscanf(parg, "%s %d %d %d %d", filename, &channels, &bpsamp,
                                 &samprate, &chmap);

  /* Try to record the file specified */

  ret = nxrecorder_recordraw(precorder,
                             filename,
                             channels,
                             bpsamp,
                             samprate,
                             chmap);

  /* nxrecorder_recordfile returned values:
   *
   *   OK         File is being recorded
   *   -EBUSY     The media device is busy
   *   -ENOSYS    The media file is an unsupported type
   *   -ENODEV    No audio device suitable to record the media type
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
        printf("Error recording file: %d\n", -ret);
        break;
    }

  return ret;
}

/****************************************************************************
 * Name: nxrecorder_cmd_stop
 *
 *   nxrecorder_cmd_stop() stops record of currently recording file
 *   context.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_STOP
static int nxrecorder_cmd_stop(FAR struct nxrecorder_s *precorder,
                               FAR char *parg)
{
  /* Stop the record */

  nxrecorder_stop(precorder);

  return OK;
}
#endif

/****************************************************************************
 * Name: nxrecorder_cmd_pause
 *
 *   nxrecorder_cmd_pause() pauses record of currently recording file
 *   context.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
static int nxrecorder_cmd_pause(FAR struct nxrecorder_s *precorder,
                                FAR char *parg)
{
  /* Pause the record */

  nxrecorder_pause(precorder);

  return OK;
}
#endif

/****************************************************************************
 * Name: nxrecorder_cmd_resume
 *
 *   nxrecorder_cmd_resume() resumes record of currently recording file
 *   context.
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
static int nxrecorder_cmd_resume(FAR struct nxrecorder_s *precorder,
                                 FAR char *parg)
{
  /* Resume the record */

  nxrecorder_resume(precorder);

  return OK;
}
#endif

/****************************************************************************
 * Name: nxrecorder_cmd_device
 *
 *   nxrecorder_cmd_device() sets the preferred audio device for record
 *
 ****************************************************************************/

static int nxrecorder_cmd_device(FAR struct nxrecorder_s *precorder,
                                 FAR char *parg)
{
  int     ret;
  char    path[32];

  /* First try to open the file directly */

  ret = nxrecorder_setdevice(precorder, parg);
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
      ret = nxrecorder_setdevice(precorder, path);
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

/****************************************************************************
 * Name: nxrecorder_cmd_quit
 *
 *   nxrecorder_cmd_quit() terminates the application
 ****************************************************************************/

static int nxrecorder_cmd_quit(FAR struct nxrecorder_s *precorder,
                               FAR char *parg)
{
  /* Stop the record if any */

#ifndef CONFIG_AUDIO_EXCLUDE_STOP
  nxrecorder_stop(precorder);
#endif

  return OK;
}

/****************************************************************************
 * Name: nxrecorder_cmd_help
 *
 *   nxrecorder_cmd_help() display the application's help information on
 *   supported commands and command syntax.
 ****************************************************************************/

#ifdef CONFIG_NXRECORDER_INCLUDE_HELP
static int nxrecorder_cmd_help(FAR struct nxrecorder_s *precorder,
                               FAR char *parg)
{
  int   len;
  int   maxlen = 0;
  int   x;
  int   c;

  /* Calculate length of longest cmd + arghelp */

  for (x = 0; x < g_nxrecorder_cmd_count; x++)
    {
      len = strlen(g_nxrecorder_cmds[x].cmd) +
            strlen(g_nxrecorder_cmds[x].arghelp);
      if (len > maxlen)
        {
          maxlen = len;
        }
    }

  printf("NxRecorder commands\n================\n");
  for (x = 0; x < g_nxrecorder_cmd_count; x++)
    {
      /* Print the command and it's arguments */

      printf("  %s %s",
            g_nxrecorder_cmds[x].cmd, g_nxrecorder_cmds[x].arghelp);

      /* Calculate number of spaces to print before the help text */

      len = maxlen - (strlen(g_nxrecorder_cmds[x].cmd) +
                      strlen(g_nxrecorder_cmds[x].arghelp));
      for (c = 0; c < len; c++)
        {
          printf(" ");
        }

      printf("  : %s\n", g_nxrecorder_cmds[x].help);
    }

  return OK;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxrecorder
 *
 *   nxrecorder() reads in commands from the console using the readline
 *   system add-in and implements a command-line based pcm raw data recorder
 *   that uses the NuttX audio system to record pcm raw data files read in
 *   from the audio device.  Commands are provided for setting volume, base
 *   and other audio features, as well as for pausing and stopping the
 *   record.
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
  int                     len;
  int                     x;
  int                     running;
  char                    *cmd;
  char                    *arg;
  FAR struct nxrecorder_s *precorder;

  printf("NxRecorder version " NXRECORDER_VER "\n");
  printf("h for commands, q to exit\n");
  printf("\n");

  /* Initialize our NxRecorder context */

  precorder = nxrecorder_create();
  if (precorder == NULL)
    {
      printf("Error:  Out of RAM\n");
      return -ENOMEM;
    }

  /* Loop until the user exits */

  running = TRUE;
  while (running)
    {
      /* Print a prompt */

      printf("nxrecorder> ");
      fflush(stdout);

      /* Read a line from the terminal */

      len = readline(buffer, sizeof(buffer), stdin, stdout);
      buffer[len] = '\0';
      if (len > 0)
        {
          if (strncmp(buffer, "!", 1) != 0)
            {
              /* nxrecorder command */

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

              for (x = 0; x < g_nxrecorder_cmd_count; x++)
                {
                  if (strcmp(cmd, g_nxrecorder_cmds[x].cmd) == 0)
                    {
                      /* Command found.  Call it's handler if not NULL */

                      if (g_nxrecorder_cmds[x].pfunc != NULL)
                        {
                          g_nxrecorder_cmds[x].pfunc(precorder, arg);
                        }

                      /* Test if it is a quit command */

                      if (g_nxrecorder_cmds[x].pfunc == nxrecorder_cmd_quit)
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

  /* Release the NxRecorder context */

  nxrecorder_release(precorder);

  return OK;
}
