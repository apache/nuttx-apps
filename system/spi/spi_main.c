/****************************************************************************
 * apps/system/spi/spi_main.c
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include "spitool.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int spicmd_help(FAR struct spitool_s *spitool, int argc, char **argv);
static int spicmd_unrecognized(FAR struct spitool_s *spitool, int argc,
                               FAR char **argv);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct spitool_s g_spitool;

static const struct cmdmap_s g_spicmds[] =
{
  { "?",    spicmd_help,  "Show help     ",  NULL },
  { "bus",  spicmd_bus,   "List buses    ",  NULL },
  { "exch",  spicmd_exch, "SPI Exchange  ", "[OPTIONS] [<hex senddata>]" },
  { "help", spicmd_help,  "Show help     ", NULL },
  { NULL,   NULL,         NULL,             NULL }
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Common, message formats */

const char g_spiargrequired[]     =
           "spitool: %s: missing required argument(s)\n";
const char g_spiarginvalid[]      =
           "spitool: %s: argument invalid\n";
const char g_spiargrange[]        =
           "spitool: %s: value out of range\n";
const char g_spicmdnotfound[]     =
           "spitool: %s: command not found\n";
const char g_spitoomanyargs[]     =
           "spitool: %s: too many arguments\n";
const char g_spicmdfailed[]       =
           "spitool: %s: %s failed: %d\n";
const char g_spixfrerror[]        =
           "spitool: %s: Transfer failed: %d\n";
const char g_spiincompleteparam[] =
           "spitool: %s: Odd number or illegal char in tx sequence\n";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: spicmd_help
 ****************************************************************************/

static int spicmd_help(FAR struct spitool_s *spitool, int argc,
                       FAR char **argv)
{
  FAR const struct cmdmap_s *ptr;

  spitool_printf(spitool, "Usage: spi <cmd> [arguments]\n");
  spitool_printf(spitool, "Where <cmd> is one of:\n\n");

  for (ptr = g_spicmds; ptr->cmd; ptr++)
    {
      if (ptr->usage)
        {
          spitool_printf(spitool, "  %s: %s %s\n",
                         ptr->desc, ptr->cmd, ptr->usage);
        }
      else
        {
          spitool_printf(spitool, "  %s: %s\n", ptr->desc, ptr->cmd);
        }
    }

  spitool_printf(spitool, "\nWhere common \"sticky\" OPTIONS include:\n");
  spitool_printf(spitool, "  [-b bus] is the SPI bus number (decimal).  "
                          "Default: %d Current: %d\n",
                 CONFIG_SPITOOL_MINBUS, spitool->bus);
#ifdef CONFIG_SPI_CMDDATA
  spitool_printf(spitool, "  [-c 0|1] Send in command mode.  "
                          "Default: %d Current: %d\n",
                 CONFIG_SPITOOL_DEFCMD, spitool->command);
#endif
  spitool_printf(spitool, "  [-f freq] SPI frequency.  "
                          "Default: %d Current: %d\n",
                 CONFIG_SPITOOL_DEFFREQ, spitool->freq);

  spitool_printf(spitool, "  [-m mode] Mode for transfer.  "
                          "Default: %d Current: %d\n",
                 CONFIG_SPITOOL_DEFMODE, spitool->mode);

  spitool_printf(spitool, "  [-n CSn] chip select number.  "
                          "Default: %d Current: %d\n",
                 0, spitool->csn);

  spitool_printf(spitool, "  [-t devtype] Chip Select type "
                          "(see spi_devtype_e).  "
                          "Default: %d Current: %d\n",
                 SPIDEVTYPE_USER, spitool->devtype);

  spitool_printf(spitool, "  [-u udelay] Delay after transfer in uS.  "
                          "Default: 0 Current: %d\n", spitool->udelay);

  spitool_printf(spitool, "  [-w width] Width of bus.  "
                          "Default: %d Current: %d\n",
                 CONFIG_SPITOOL_DEFWIDTH, spitool->width);

  spitool_printf(spitool, "  [-x count] Words to exchange  "
                          "Default: %d Current: %d Max: %d\n",
                 CONFIG_SPITOOL_DEFWORDS, spitool->count, MAX_XDATA);

  spitool_printf(spitool, "\nNOTES:\n");
#ifndef CONFIG_DISABLE_ENVIRON
  spitool_printf(spitool, "o An environment variable like $PATH may be used "
                          "for any argument.\n");
#endif
  spitool_printf(spitool, "o Arguments are \"sticky\".  "
                          "For example, once the SPI address is\n");
  spitool_printf(spitool, "  specified, that address will be re-used "
                          "until it is changed.\n");
  spitool_printf(spitool, "\nWARNING:\n");
  spitool_printf(spitool, "o The SPI commands may have bad side effects "
                          "on your SPI devices.\n");
  spitool_printf(spitool, "  Use only at your own risk.\n");
  return OK;
}

/****************************************************************************
 * Name: spicmd_unrecognized
 ****************************************************************************/

static int spicmd_unrecognized(FAR struct spitool_s *spitool, int argc,
                               FAR char **argv)
{
  spitool_printf(spitool, g_spicmdnotfound, argv[0]);
  return ERROR;
}

/****************************************************************************
 * Name: spi_execute
 ****************************************************************************/

static int spi_execute(FAR struct spitool_s *spitool, int argc,
                           FAR char *argv[])
{
  FAR const struct cmdmap_s *cmdmap;
  FAR const char            *cmd;
  cmd_t                  handler;
  int                    ret;

  /* The form of argv is:
   *
   * argv[0]:      The command name.  This is argv[0] when the arguments
   *               are, finally, received by the command vtblr
   * argv[1]:      The beginning of argument (up to MAX_ARGUMENTS)
   * argv[argc]:   NULL terminating pointer
   */

  /* See if the command is one that we understand */

  cmd     = argv[0];
  handler = spicmd_unrecognized;

  for (cmdmap = g_spicmds; cmdmap->cmd; cmdmap++)
    {
      if (strcmp(cmdmap->cmd, cmd) == 0)
        {
          handler = cmdmap->handler;
          break;
        }
    }

  ret = handler(spitool, argc, argv);
  return ret;
}

/****************************************************************************
 * Name: spi_argument
 ****************************************************************************/

static FAR char *spi_argument(FAR struct spitool_s *spitool, int argc,
                              FAR char *argv[], FAR int *pindex)
{
  FAR char *arg;
  int  index = *pindex;

  /* If we are at the end of the arguments with nothing, then return NULL */

  if (index >= argc)
    {
      return NULL;
    }

  /* Get the return parameter */

  arg     = argv[index];
  *pindex = index + 1;

#ifndef CONFIG_DISABLE_ENVIRON
  /* Check for references to environment variables */

  if (arg[0] == '$')
    {
      /* Return the value of the environment variable with this name */

      FAR char *value = getenv(arg + 1);
      if (value)
        {
          return value;
        }
      else
        {
          return (FAR char *)"";
        }
    }
#endif

  /* Return the next argument. */

  return arg;
}

/****************************************************************************
 * Name: spi_parse
 ****************************************************************************/

static int spi_parse(FAR struct spitool_s *spitool, int argc,
                          FAR char *argv[])
{
  FAR char *newargs[MAX_ARGUMENTS + 2];
  FAR char *cmd;
  int       nargs;
  int       index;

  /* Parse out the command, skipping the first argument (the program name) */

  index = 1;
  cmd = spi_argument(spitool, argc, argv, &index);

  /* Check if any command was provided */

  if (!cmd)
    {
      /* An empty line is not an error and an unprocessed command cannot
       * generate an error, but neither should they change the last
       * command status.
       */

      return spicmd_help(spitool, 0, NULL);
    }

  /* Parse all of the arguments following the command name. */

  newargs[0] = cmd;
  for (nargs = 1; nargs <= MAX_ARGUMENTS; nargs++)
    {
      newargs[nargs] = spi_argument(spitool, argc, argv, &index);
      if (!newargs[nargs])
        {
          break;
        }
    }

  newargs[nargs] = NULL;

  /* Then execute the command */

  return spi_execute(spitool, nargs, newargs);
}

/****************************************************************************
 * Name: spi_setup
 ****************************************************************************/

static inline int spi_setup(FAR struct spitool_s *spitool)
{
  /* Initialize the output stream */

#ifdef CONFIG_SPITOOL_OUTDEV
  spitool->ss_outfd = open(CONFIG_SPITOOL_OUTDEV, O_WRONLY);
  if (spitool->ss_outfd < 0)
    {
      fprintf(stderr, g_spicmdfailed, "open", errno);
      return ERROR;
    }

  /* Create a standard C stream on the console device */

  spitool->ss_outstream = fdopen(spitool->ss_outfd, "w");
  if (!spitool->ss_outstream)
    {
      fprintf(stderr, g_spicmdfailed, "fdopen", errno);
      return ERROR;
    }
#endif

  return OK;
}

/****************************************************************************
 * Name: spi_teardown
 *
 * Description:
 *   Close the output stream if it is not the standard output stream.
 *
 ****************************************************************************/

static void spi_teardown(FAR struct spitool_s *spitool)
{
  fflush(OUTSTREAM(&g_spitool));

#ifdef CONFIG_SPITOOL_OUTDEV
  fclose(spitool->ss_outstream);
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: spi_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  /* Verify settings */

  if (g_spitool.bus < CONFIG_SPITOOL_MINBUS ||
      g_spitool.bus > CONFIG_SPITOOL_MAXBUS)
    {
      g_spitool.bus = CONFIG_SPITOOL_MINBUS;
    }

  if (g_spitool.freq == 0)
    {
      g_spitool.freq = CONFIG_SPITOOL_DEFFREQ;
    }

  if (g_spitool.mode == 0)
    {
      g_spitool.mode = CONFIG_SPITOOL_DEFMODE;
    }

  if (g_spitool.width == 0)
    {
      g_spitool.width = CONFIG_SPITOOL_DEFWIDTH;
    }

  if (g_spitool.count == 0)
    {
      g_spitool.count = CONFIG_SPITOOL_DEFWORDS;
    }

  if (g_spitool.devtype == 0)
    {
      g_spitool.devtype = SPIDEVTYPE_USER;
    }

  /* Parse and process the command line */

  spi_setup(&g_spitool);
  spi_parse(&g_spitool, argc, argv);

  spitool_flush(&g_spitool);
  spi_teardown(&g_spitool);
  return OK;
}

/****************************************************************************
 * Name: spitool_printf
 *
 * Description:
 *   Print a string to the currently selected stream.
 *
 ****************************************************************************/

int spitool_printf(FAR struct spitool_s *spitool, FAR const char *fmt, ...)
{
  va_list ap;
  int     ret;

  va_start(ap, fmt);
  ret = vfprintf(OUTSTREAM(spitool), fmt, ap);
  va_end(ap);

  return ret;
}

/****************************************************************************
 * Name: spitool_write
 *
 * Description:
 *   write a buffer to the currently selected stream.
 *
 ****************************************************************************/

ssize_t spitool_write(FAR struct spitool_s *spitool, FAR const void *buffer,
                      size_t nbytes)
{
  ssize_t ret;

  /* Write the data to the output stream */

  ret = fwrite(buffer, 1, nbytes, OUTSTREAM(spitool));
  if (ret < 0)
    {
      _err("ERROR: [%d] Failed to send buffer: %d\n", OUTFD(spitool), errno);
    }

  return ret;
}

/****************************************************************************
 * Name: spitool_flush
 *
 * Description:
 *   Flush buffered I/O to the currently selected stream.
 *
 ****************************************************************************/

void spitool_flush(FAR struct spitool_s *spitool)
{
  fflush(OUTSTREAM(spitool));
}
