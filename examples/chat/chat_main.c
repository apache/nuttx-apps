/****************************************************************************
 * apps/examples/chat/chat_main.c
 *
 *   Copyright (C) 2016 Vladimir Komendantskiy. All rights reserved.
 *   Author: Vladimir Komendantskiy <vladimir@moixaenergy.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <assert.h>
#include <debug.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "netutils/chat.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CHAT_TTYNAME_SIZE  32

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Chat app private state. A supertype of 'chat_ctl'. */

struct chat_app
{
  struct chat_ctl ctl;             /* Embedded 'chat_ctl' type. */

  /* Private fields */

  int argc;                        /* number of command-line arguments */
  FAR char **argv;                 /* command-line arguments */
  char tty[CHAT_TTYNAME_SIZE];     /* modem TTY device node */
  FAR const char *script;          /* raw chat script - input to the parser */
  bool script_dynalloc;            /* true if the script should be freed */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Preset chat scripts */

FAR const char g_chat_script0[] = CONFIG_EXAMPLES_CHAT_PRESET0;
FAR const char g_chat_script1[] = CONFIG_EXAMPLES_CHAT_PRESET1;
FAR const char g_chat_script2[] = CONFIG_EXAMPLES_CHAT_PRESET2;
FAR const char g_chat_script3[] = CONFIG_EXAMPLES_CHAT_PRESET3;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void chat_show_usage(void)
{
  printf("Usage: chat [options]\n"
         "  where [options] is a possibly empty list of\n"
         "-d<file>   : modem TTY device node\n"
         "-e         : echo modem output to stderr\n"
         "-f<file>   : chat script file\n"
         "-p<number> : preprogrammed script\n"
         "-t<number> : default modem response timeout\n"
         "-v         : verbose mode\n");
}

static int chat_chardev(FAR struct chat_app *priv)
{
  int flags;

  flags = fcntl(priv->ctl.fd, F_GETFL, 0);
  if (flags < 0)
    {
      return flags;
    }

  flags = fcntl(priv->ctl.fd, F_SETFL, flags | O_NONBLOCK);
  if (flags < 0)
    {
      return flags;
    }

  return 0;
}

static int chat_script_preset(FAR struct chat_app *priv, int script_number)
{
  int ret = 0;

  _info("preset script %d\n", script_number);

  switch (script_number)
    {
    case 0:
      priv->script = g_chat_script0;
      break;

    case 1:
      priv->script = g_chat_script1;
      break;

    case 2:
      priv->script = g_chat_script2;
      break;

    case 3:
      priv->script = g_chat_script3;
      break;

    default:
      ret = -ERANGE;
      break;
    }

  return ret;
}

static int chat_script_read(FAR struct chat_app *priv,
                            FAR const char *filepath)
{
  FAR char *scriptp;
  size_t spare_size = CONFIG_EXAMPLES_CHAT_SIZE - 1;
  ssize_t read_size;
  bool eof = false;
  int ret = 0;
  int fd;

  scriptp = malloc(CONFIG_EXAMPLES_CHAT_SIZE);
  if (scriptp == NULL)
    {
      return -ENOMEM;
    }

  priv->script_dynalloc = true;
  priv->script          = scriptp;
  memset(scriptp, 0, CONFIG_EXAMPLES_CHAT_SIZE);

  fd = open(filepath, O_RDONLY);
  if (fd < 0)
    {
      fprintf(stderr, "Cannot open %s, error %d\n", filepath, errno);
      ret = -ENOENT;
    }

  while (!ret && !eof && spare_size > 0)
    {
      read_size = read(fd, scriptp, spare_size);
      if (read_size < 0)
        {
          /* EINTR is not a read error.  It simply means that a signal was
           * received while waiting for the read to complete.
           */

          if (errno != EINTR)
            {
              fprintf(stderr, "Cannot read %s, error %d\n",
                      filepath, errno);
              ret = -EIO;
            }
        }
      else if (read_size == 0)
        {
          eof = true;
        }
      else
        {
          /* read_size > 0 */

          DEBUGASSERT(read_size <= spare_size);
          scriptp += read_size;
          spare_size -= read_size;
        }
    }

  close(fd);
  return ret;
}

static int chat_parse_args(FAR struct chat_app *priv)
{
  /* -d TTY device node (non-Linux feature)
   * -e echo to stderr
   * -f script file
   * -p preprogrammed script (non-Linux feature)
   * -t timeout
   * -v verbose mode
   */

  int numarg;
  int ret = 0;
  int i;

  DEBUGASSERT(priv != NULL);

  if (priv->argc < 2)
    {
      ret = -EINVAL;
    }

  /* Iterate through command-line arguments and parse those */

  for (i = 1; !ret && i < priv->argc && priv->argv[i]; i++)
    {
      if (priv->argv[i][0] != '-')
        {
          ret = -EINVAL;
        }
      else
        {
          switch (priv->argv[i][1])
            {
            case 'd':

              /* set the TTY device node */

              strncpy(priv->tty,
                      (FAR char *)priv->argv[i] + 2,
                      CHAT_TTYNAME_SIZE - 1);
              break;

            case 'e':
              priv->ctl.echo = true;
              break;

            case 'f':
              ret = chat_script_read(priv,
                                     (FAR char *)priv->argv[i] + 2);
              break;

            case 'p':
              numarg = strtol((FAR char *)priv->argv[i] + 2,
                              NULL, 10);
              if (errno < 0)
                {
                  ret = -EINVAL;
                  break;
                }

              ret = chat_script_preset(priv, numarg);
              break;

            case 't':
              numarg = strtol((FAR char *)priv->argv[i] + 2,
                              NULL, 10);

              if (errno < 0 || numarg < 0)
                {
                  ret = -EINVAL;
                }
              else
                {
                  priv->ctl.timeout = numarg;
                }

              break;

            case 'v':
              priv->ctl.verbose = true;
              break;

            default:
              ret = -EINVAL;
              break;
            }
        }
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: chat_main
 *
 * Description:
 *   Chat command entry point.
 *
 ****************************************************************************/

int main(int argc, FAR char **argv)
{
  struct chat_app priv;
  int ret;
  int exit_code = EXIT_SUCCESS;

  priv.argc = argc;
  priv.argv = argv;
  priv.ctl.echo = false;
  priv.ctl.verbose = false;
  priv.ctl.timeout = CONFIG_EXAMPLES_CHAT_TIMEOUT_SECONDS;
  priv.script = NULL;
  priv.script_dynalloc = false;
  strncpy(priv.tty, CONFIG_EXAMPLES_CHAT_TTY_DEVNODE, CHAT_TTYNAME_SIZE - 1);

  _info("parsing the arguments\n");
  ret = chat_parse_args((FAR struct chat_app *)&priv);
  if (ret < 0)
    {
      _info("Command line parsing failed: code %d, errno %d\n", ret, errno);
      chat_show_usage();
      exit_code = EXIT_FAILURE;
      goto with_script;
    }

  if (priv.script == NULL)
    {
      fprintf(stderr, "No chat script given\n");
      exit_code = EXIT_FAILURE;
      goto no_script;
    }

  _info("opening %s\n", priv.tty);
  priv.ctl.fd = open(priv.tty, O_RDWR);
  if (priv.ctl.fd < 0)
    {
      _info("Failed to open %s: %d\n", priv.tty, errno);
      exit_code = EXIT_FAILURE;
      goto with_script;
    }

  _info("setting up character device\n");
  ret = chat_chardev(&priv);
  if (ret < 0)
    {
      _info("Failed to open %s: %d\n", priv.tty, errno);
      exit_code = EXIT_FAILURE;
      goto with_tty_dev;
    }

  ret = chat((FAR struct chat_ctl *)&priv.ctl, priv.script);

with_tty_dev:
  close(priv.ctl.fd);

with_script:
  if (priv.script_dynalloc)
    {
      free((FAR char *)priv.script);
    }

no_script:
  fflush(stderr);
  fflush(stdout);

  _info("Exit code %d\n", exit_code);
  return exit_code;
}
