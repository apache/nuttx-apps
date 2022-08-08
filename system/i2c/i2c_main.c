/****************************************************************************
 * apps/system/i2c/i2c_main.c
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

#include "i2ctool.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int i2ccmd_help(FAR struct i2ctool_s *i2ctool, int argc, char **argv);
static int i2ccmd_unrecognized(FAR struct i2ctool_s *i2ctool, int argc,
                               FAR char **argv);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct i2ctool_s g_i2ctool;

static const struct cmdmap_s g_i2ccmds[] =
{
  { "?",     i2ccmd_help,  "Show help     ", NULL },
  { "bus",   i2ccmd_bus,   "List buses    ", NULL },
#ifdef CONFIG_I2C_RESET
  { "reset", i2ccmd_reset, "Reset bus     ", NULL },
#endif
  { "dev",   i2ccmd_dev,   "List devices  ", "[OPTIONS] <first> <last>" },
  { "get",   i2ccmd_get,   "Read register ", "[OPTIONS] [<repetitions>]" },
  { "dump",  i2ccmd_dump,  "Dump register ", "[OPTIONS] [<num bytes>]" },
  { "help",  i2ccmd_help,  "Show help     ", NULL },
  {
    "set",   i2ccmd_set,   "Write register",
      "[OPTIONS] <value> [<repetitions>]"
  },
  {
    "verf",  i2ccmd_verf,  "Verify access ",
      "[OPTIONS] [<value>] [<repetitions>]"
  },
  { NULL,   NULL,        NULL,             NULL }
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Common, message formats */

const char g_i2cargrequired[] =
                    "i2ctool: %s: missing required argument(s)\n";
const char g_i2carginvalid[]  = "i2ctool: %s: argument invalid\n";
const char g_i2cargrange[]    = "i2ctool: %s: value out of range\n";
const char g_i2ccmdnotfound[] = "i2ctool: %s: command not found\n";
const char g_i2ctoomanyargs[] = "i2ctool: %s: too many arguments\n";
const char g_i2ccmdfailed[]   = "i2ctool: %s: %s failed: %d\n";
const char g_i2cxfrerror[]    = "i2ctool: %s: Transfer failed: %d\n";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: i2ccmd_help
 ****************************************************************************/

static int i2ccmd_help(FAR struct i2ctool_s *i2ctool, int argc, char **argv)
{
  const struct cmdmap_s *ptr;

  i2ctool_printf(i2ctool, "Usage: i2c <cmd> [arguments]\n");
  i2ctool_printf(i2ctool, "Where <cmd> is one of:\n\n");
  for (ptr = g_i2ccmds; ptr->cmd; ptr++)
    {
      if (ptr->usage)
        {
          i2ctool_printf(i2ctool, "  %s: %s %s\n",
                         ptr->desc, ptr->cmd, ptr->usage);
        }
      else
        {
          i2ctool_printf(i2ctool, "  %s: %s\n", ptr->desc, ptr->cmd);
        }
    }

  i2ctool_printf(i2ctool,
                 "\nWhere common \"sticky\" OPTIONS include:\n");
  i2ctool_printf(i2ctool,
                 "  [-a addr] is the I2C device address (hex)."
                 "  Default: %02x Current: %02x\n",
                 CONFIG_I2CTOOL_MINADDR, i2ctool->addr);
  i2ctool_printf(i2ctool,
                 "  [-b bus] is the I2C bus number (decimal)."
                 "  Default: %d Current: %d\n",
                 CONFIG_I2CTOOL_MINBUS, i2ctool->bus);
  i2ctool_printf(i2ctool,
                 "  [-w width] is the data width (8 or 16 decimal)."
                 "  Default: 8 Current: %d\n",
                 i2ctool->width);
  i2ctool_printf(i2ctool,
                 "  [-s|n], send/don't send start between command and data."
                 "  Default: -n Current: %s\n",
                 i2ctool->start ? "-s" : "-n");
  i2ctool_printf(i2ctool,
                 "  [-i|j], Auto increment|don't increment regaddr on "
                 "repetitions."
                 "  Default: NO Current: %s\n",
                 i2ctool->autoincr ? "YES" : "NO");
  i2ctool_printf(i2ctool,
                 "  [-f freq] I2C frequency."
                 "  Default: %d Current: %" PRIu32 "\n",
                 CONFIG_I2CTOOL_DEFFREQ, i2ctool->freq);

  i2ctool_printf(i2ctool, "\nSpecial non-sticky options:\n");
  i2ctool_printf(i2ctool,
                 "  [-r regaddr] is the I2C device register index (hex)."
                 "  Default: not used/sent\n");

  i2ctool_printf(i2ctool, "\nNOTES:\n");
#ifndef CONFIG_DISABLE_ENVIRON
  i2ctool_printf(i2ctool, "o An environment variable like $PATH may be used "
                          "for any argument.\n");
#endif
  i2ctool_printf(i2ctool,
                 "o Arguments are \"sticky\". For example, once "
                 "the I2C address is\n");
  i2ctool_printf(i2ctool,
                 "  specified, that address will be re-used until "
                 "it is changed.\n");
  i2ctool_printf(i2ctool, "\nWARNING:\n");
  i2ctool_printf(i2ctool,
                 "o The I2C dev command may have bad side effects "
                 "on your I2C devices.\n");
  i2ctool_printf(i2ctool, "  Use only at your own risk.\n");
  return OK;
}

/****************************************************************************
 * Name: i2ccmd_unrecognized
 ****************************************************************************/

static int i2ccmd_unrecognized(FAR struct i2ctool_s *i2ctool, int argc,
                               FAR char **argv)
{
  i2ctool_printf(i2ctool, g_i2ccmdnotfound, argv[0]);
  return ERROR;
}

/****************************************************************************
 * Name: i2c_execute
 ****************************************************************************/

static int i2c_execute(FAR struct i2ctool_s *i2ctool, int argc,
                       FAR char *argv[])
{
  FAR const struct cmdmap_s *cmdmap;
  FAR const char            *cmd;
  cmd_t                     handler;
  int                       ret;

  /* The form of argv is:
   *
   * argv[0]:      The command name.  This is argv[0] when the arguments
   *               are, finally, received by the command vtblr
   * argv[1]:      The beginning of argument (up to MAX_ARGUMENTS)
   * argv[argc]:   NULL terminating pointer
   */

  /* See if the command is one that we understand */

  cmd     = argv[0];
  handler = i2ccmd_unrecognized;

  for (cmdmap = g_i2ccmds; cmdmap->cmd; cmdmap++)
    {
      if (strcmp(cmdmap->cmd, cmd) == 0)
        {
          handler = cmdmap->handler;
          break;
        }
    }

  ret = handler(i2ctool, argc, argv);
  return ret;
}

/****************************************************************************
 * Name: i2c_argument
 ****************************************************************************/

static FAR char *i2c_argument(FAR struct i2ctool_s *i2ctool,
                              int argc, char *argv[], int *pindex)
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
 * Name: i2c_parse
 ****************************************************************************/

static int i2c_parse(FAR struct i2ctool_s *i2ctool, int argc, char *argv[])
{
  FAR char *newargs[MAX_ARGUMENTS + 2];
  FAR char *cmd;
  int       nargs;
  int       index;

  /* Parse out the command, skipping the first argument (the program name) */

  index = 1;
  cmd = i2c_argument(i2ctool, argc, argv, &index);

  /* Check if any command was provided */

  if (!cmd)
    {
      /* An empty line is not an error and an unprocessed command cannot
       * generate an error, but neither should they change the last
       * command status.
       */

      return i2ccmd_help(i2ctool, 0, NULL);
    }

  /* Parse all of the arguments following the command name. */

  newargs[0] = cmd;
  for (nargs = 1; nargs <= MAX_ARGUMENTS; nargs++)
    {
      newargs[nargs] = i2c_argument(i2ctool, argc, argv, &index);
      if (!newargs[nargs])
        {
          break;
        }
    }

  newargs[nargs] = NULL;

  /* Then execute the command */

  return i2c_execute(i2ctool, nargs, newargs);
}

/****************************************************************************
 * Name: i2c_setup
 ****************************************************************************/

static inline int i2c_setup(FAR struct i2ctool_s *i2ctool)
{
  /* Initialize the output stream */

#ifdef CONFIG_I2CTOOL_OUTDEV
  i2ctool->ss_outfd = open(CONFIG_I2CTOOL_OUTDEV, O_WRONLY);
  if (i2ctool->ss_outfd < 0)
    {
      fprintf(stderr, g_i2ccmdfailed, "open", errno);
      return ERROR;
    }

  /* Create a standard C stream on the console device */

  i2ctool->ss_outstream = fdopen(i2ctool->ss_outfd, "w");
  if (!i2ctool->ss_outstream)
    {
      fprintf(stderr, g_i2ccmdfailed, "fdopen", errno);
      return ERROR;
    }
#endif

  return OK;
}

/****************************************************************************
 * Name: i2c_teardown
 *
 * Description:
 *   Close the output stream if it is not the standard output stream.
 *
 ****************************************************************************/

static void i2c_teardown(FAR struct i2ctool_s *i2ctool)
{
  fflush(OUTSTREAM(&g_i2ctool));

#ifdef CONFIG_I2CTOOL_OUTDEV
  fclose(i2ctool->ss_outstream);
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: i2c_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  /* Verify settings */

  if (g_i2ctool.bus < CONFIG_I2CTOOL_MINBUS ||
      g_i2ctool.bus > CONFIG_I2CTOOL_MAXBUS)
    {
      g_i2ctool.bus = CONFIG_I2CTOOL_MINBUS;
    }

  if (g_i2ctool.addr < CONFIG_I2CTOOL_MINADDR ||
      g_i2ctool.addr > CONFIG_I2CTOOL_MAXADDR)
    {
      g_i2ctool.addr = CONFIG_I2CTOOL_MINADDR;
    }

  if (g_i2ctool.regaddr > CONFIG_I2CTOOL_MAXREGADDR)
    {
      g_i2ctool.regaddr = 0;
    }

  if (g_i2ctool.width != 8 && g_i2ctool.width != 16)
    {
      g_i2ctool.width = 8;
    }

  if (g_i2ctool.freq == 0)
    {
      g_i2ctool.freq = CONFIG_I2CTOOL_DEFFREQ;
    }

  g_i2ctool.hasregindx = false;

  /* Parse and process the command line */

  i2c_setup(&g_i2ctool);
  i2c_parse(&g_i2ctool, argc, argv);

  i2ctool_flush(&g_i2ctool);
  i2c_teardown(&g_i2ctool);
  return OK;
}

/****************************************************************************
 * Name: i2ctool_printf
 *
 * Description:
 *   Print a string to the currently selected stream.
 *
 ****************************************************************************/

int i2ctool_printf(FAR struct i2ctool_s *i2ctool, const char *fmt, ...)
{
  va_list ap;
  int     ret;

  va_start(ap, fmt);
  ret = vfprintf(OUTSTREAM(i2ctool), fmt, ap);
  va_end(ap);

  return ret;
}

/****************************************************************************
 * Name: i2ctool_write
 *
 * Description:
 *   write a buffer to the currently selected stream.
 *
 ****************************************************************************/

ssize_t i2ctool_write(FAR struct i2ctool_s *i2ctool, FAR const void *buffer,
                      size_t nbytes)
{
  ssize_t ret;

  /* Write the data to the output stream */

  ret = fwrite(buffer, 1, nbytes, OUTSTREAM(i2ctool));
  if (ret < 0)
    {
      _err("ERROR: [%d] Failed to send buffer: %d\n", OUTFD(i2ctool), errno);
    }

  return ret;
}

/****************************************************************************
 * Name: i2ctool_flush
 *
 * Description:
 *   Flush buffered I/O to the currently selected stream.
 *
 ****************************************************************************/

void i2ctool_flush(FAR struct i2ctool_s *i2ctool)
{
  fflush(OUTSTREAM(i2ctool));
}
