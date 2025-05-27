/****************************************************************************
 * apps/system/nxcamera/nxcamera_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
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
#include <nuttx/video/video.h>

#include <sys/types.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include <system/readline.h>
#include <system/nxcamera.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NXCAMERA_VER    "1.00"

#ifdef CONFIG_NXCAMERA_INCLUDE_HELP
#  define NXCAMERA_HELP_TEXT(x)  x
#else
#  define NXCAMERA_HELP_TEXT(x)
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef CODE int (*nxcamera_func)(FAR struct nxcamera_s *cam, FAR char *arg);

struct nxcamera_cmd_s
{
  FAR const char *cmd;       /* The command text */
  FAR const char *arghelp;   /* Text describing the args */
  nxcamera_func  pfunc;      /* Pointer to command handler */
  FAR const char *help;      /* The help text */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int nxcamera_cmd_quit(FAR struct nxcamera_s *pcam, FAR char *parg);
static int nxcamera_cmd_stream(FAR struct nxcamera_s *pcam, FAR char *parg);
static int nxcamera_cmd_input(FAR struct nxcamera_s *pcam, FAR char *parg);
static int nxcamera_cmd_output(FAR struct nxcamera_s *pcam, FAR char *parg);
static int nxcamera_cmd_stop(FAR struct nxcamera_s *pcam, FAR char *parg);
#ifdef CONFIG_NXCAMERA_INCLUDE_HELP
static int nxcamera_cmd_help(FAR struct nxcamera_s *pcam, FAR char *parg);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct nxcamera_cmd_s g_nxcamera_cmds[] =
{
  {
    "input",
    "videodev",
    nxcamera_cmd_input,
    NXCAMERA_HELP_TEXT("Specify a preferred capture device")
  },
  {
    "output",
    "fbdev|filename",
    nxcamera_cmd_output,
    NXCAMERA_HELP_TEXT("Specify a display device or filename")
  },
#ifdef CONFIG_NXCAMERA_INCLUDE_HELP
  {
    "h",
    "",
    nxcamera_cmd_help,
    NXCAMERA_HELP_TEXT("Display help for commands")
  },
  {
    "help",
    "",
    nxcamera_cmd_help,
    NXCAMERA_HELP_TEXT("Display help for commands")
  },
#endif
  {
    "stream",
    "width height framerate format",
    nxcamera_cmd_stream,
    NXCAMERA_HELP_TEXT("Video stream test (format is fourcc)")
  },
  {
    "stop",
    "",
    nxcamera_cmd_stop,
    NXCAMERA_HELP_TEXT("Stop stream")
  },
  {
    "q",
    "",
    nxcamera_cmd_quit,
    NXCAMERA_HELP_TEXT("Exit NxCamera")
  },
  {
    "quit",
    "",
    nxcamera_cmd_quit,
    NXCAMERA_HELP_TEXT("Exit NxCamera")
  }
};

static const int g_nxcamera_cmd_count = sizeof(g_nxcamera_cmds) /
                                        sizeof(struct nxcamera_cmd_s);

/****************************************************************************
 * Name: nxcamera_cmd_stream
 *
 *   nxcamera_cmd_loop() play and record the raw data file using the nxcamera
 *   context.
 *
 ****************************************************************************/

static int nxcamera_cmd_stream(FAR struct nxcamera_s *pcam, FAR char *parg)
{
  uint16_t width = 0;
  uint16_t height = 0;
  uint32_t framerate = 0;
  uint32_t format = 0;
  int      ret;
  char     cc[5] =
    {
      0
    };

  sscanf(parg, "%"SCNu16" %"SCNu16" %"SCNu32" %4s",
                &width, &height, &framerate, cc);
  format = v4l2_fourcc(cc[0], cc[1], cc[2], cc[3]);

  /* Try to stream raw data with settings specified */

  ret = nxcamera_stream(pcam, width, height, framerate, format);

  /* nxcamera_stream returned values:
   *
   *   OK         Stream is being run
   *   -EBUSY     The media device is busy
   *   -ENOSYS    The video format is not unsupported
   *   -ENODEV    No video device suitable to capture media
   */

  switch (ret)
    {
      case OK:
        break;

      case -ENODEV:
        printf("No suitable Video Device found\n");
        break;

      case -EBUSY:
        printf("Video device busy\n");
        break;

      case -ENOSYS:
        printf("Unknown video format\n");
        break;

      default:
        printf("Error stream test: %d\n", ret);
        break;
    }

  return ret;
}

/****************************************************************************
 * Name: nxcamera_cmd_stop
 *
 *   nxcamera_cmd_stop() stops stream context.
 *
 ****************************************************************************/

static int nxcamera_cmd_stop(FAR struct nxcamera_s *pcam, FAR char *parg)
{
  /* Stop the stream */

  return nxcamera_stop(pcam);
}

/****************************************************************************
 * Name: nxcamera_cmd_input
 *
 *   nxcamera_cmd_input() sets the preferred capture device for stream.
 *
 ****************************************************************************/

static int nxcamera_cmd_input(FAR struct nxcamera_s *pcam, FAR char *parg)
{
  int  ret;
  char path[PATH_MAX];

  /* First try to open the file directly */

  ret = nxcamera_setdevice(pcam, parg);
  if (ret < 0)
    {
      /* Append the /dev path and try again */

      snprintf(path, sizeof(path), "/dev/%s", parg);
      ret = nxcamera_setdevice(pcam, path);
    }

  /* Test if the device file exists */

  if (ret == -ENOENT)
    {
      /* Device doesn't exit.  Report error */

      printf("Device %s not found\n", parg);
    }
  else if (ret == -ENODEV)
    {
      /* Test if is is an video device */

      printf("Device %s is not an video device\n", parg);
    }

  /* Return error value */

  return ret;
}

/****************************************************************************
 * Name: nxcamera_cmd_output
 *
 *   nxcamera_cmd_device() sets the output device/file for display/save.
 *
 ****************************************************************************/

static int nxcamera_cmd_output(FAR struct nxcamera_s *pcam, FAR char *parg)
{
  int      ret;
  char     path[PATH_MAX];
  FAR char *ext;
  bool     isimage = false;

  /* First try to open the device directly */

  ret = nxcamera_setfb(pcam, parg);
  if (ret < 0)
    {
      /* Append the /dev path and try again */

      snprintf(path, sizeof(path), "/dev/%s", parg);
      ret = nxcamera_setfb(pcam, path);
    }

  /* Device doesn't exist or is not a video device. Treat as file */

  if (ret < 0)
    {
      if (ret == -ENODEV)
        {
          printf("Device %s is not an video device\n", parg);
          return ret;
        }

      ext = strrchr(parg, '.');
      if (ext && (ext != parg))
        {
          ext++;
          isimage = strncmp(ext, "jpg", sizeof("jpg")) == 0 ||
                    strncmp(ext, "jpeg", sizeof("jpeg")) == 0;
        }

      ret = nxcamera_setfile(pcam, parg, isimage);
    }

  if (ret < 0)
    {
      /* Create file error. Report error */

      printf("Error outputting to %s\n", parg);
    }

  /* Output device or file set successfully */

  return ret;
}

/****************************************************************************
 * Name: nxcamera_cmd_quit
 *
 *   nxcamera_cmd_quit() terminates the application.
 *
 ****************************************************************************/

static int nxcamera_cmd_quit(FAR struct nxcamera_s *pcam, FAR char *parg)
{
  /* Stop the stream if any */

  return nxcamera_stop(pcam);
}

/****************************************************************************
 * Name: nxcamera_cmd_help
 *
 *   nxcamera_cmd_help() displays the application's help information on
 *   supported commands and command syntax.
 *
 ****************************************************************************/

#ifdef CONFIG_NXCAMERA_INCLUDE_HELP
static int nxcamera_cmd_help(FAR struct nxcamera_s *pcam, FAR char *parg)
{
  int len;
  int maxlen = 0;
  int x;
  int c;

  /* Calculate length of longest cmd + arghelp */

  for (x = 0; x < g_nxcamera_cmd_count; x++)
    {
      len = strlen(g_nxcamera_cmds[x].cmd) +
            strlen(g_nxcamera_cmds[x].arghelp);
      if (len > maxlen)
        {
          maxlen = len;
        }
    }

  printf("NxCamera commands\n================\n");
  for (x = 0; x < g_nxcamera_cmd_count; x++)
    {
      /* Print the command and it's arguments */

      printf("  %s %s", g_nxcamera_cmds[x].cmd, g_nxcamera_cmds[x].arghelp);

      /* Calculate number of spaces to print before the help text */

      len = maxlen - (strlen(g_nxcamera_cmds[x].cmd) +
                      strlen(g_nxcamera_cmds[x].arghelp));
      for (c = 0; c < len; c++)
        {
          printf(" ");
        }

      printf("  : %s\n", g_nxcamera_cmds[x].help);
    }

  return OK;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxcamera
 *
 *   nxcamera reads in commands from the console using the readline
 *   system add-in and impalements a command-line based media loop
 *   tester that uses the NuttX video system to capture and then display
 *   or save video from the lower video driver. Commands are provided for
 *   setting width, height, framerate, pixel format and other video controls,
 *   as well as for stopping the test.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  char                  buffer[LINE_MAX];
  int                   len;
  int                   x;
  bool                  running = true;
  FAR char              *cmd;
  FAR char              *arg;
  FAR struct nxcamera_s *pcam;

  printf("NxCamera version " NXCAMERA_VER "\n");
  printf("h for commands, q to exit\n");
#ifndef CONFIG_LIBYUV
  printf("Libyuv is not enabled, won't output to RGB framebuffer\n");
#endif
  printf("\n");

  /* Initialize our NxCamera context */

  pcam = nxcamera_create();
  if (pcam == NULL)
    {
      printf("Error: Out of RAM\n");
      return -ENOMEM;
    }

  /* Loop until the user exits */

  while (running)
    {
      /* Print a prompt */

      printf("nxcamera> ");
      fflush(stdout);

      /* Read a line from the terminal */

      len = readline_stream(buffer, sizeof(buffer),
                            stdin, stdout);
      if (len > 0)
        {
          buffer[len] = '\0';
          if (strncmp(buffer, "!", 1) != 0)
            {
              /* nxcamera command */

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

              for (x = 0; x < g_nxcamera_cmd_count; x++)
                {
                  if (strcmp(cmd, g_nxcamera_cmds[x].cmd) == 0)
                    {
                      /* Command found.  Call it's handler if not NULL */

                      if (g_nxcamera_cmds[x].pfunc != NULL)
                        {
                          g_nxcamera_cmds[x].pfunc(pcam, arg);
                        }

                      /* Test if it is a quit command */

                      if (g_nxcamera_cmds[x].pfunc == nxcamera_cmd_quit)
                        {
                          running = false;
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
              printf("%s: unknown nxcamera command\n", buffer);
#endif
            }
        }
    }

  /* Release the NxCamera context */

  nxcamera_release(pcam);

  return OK;
}
