/****************************************************************************
 * apps/system/i2c/i2c_main.c
 *
 *   Copyright (C) 2011 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
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
 * 3. Neither the name Gregory Nutt nor the names of its contributors may be
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
 * Pre-processor Definitions
 ****************************************************************************/

/* Are we using the NuttX console for I/O?  Or some other character device? */

#ifdef CONFIG_I2CTOOL_INDEV
#  define INFD(p)      ((p)->ss_infd)
#  define INSTREAM(p)  ((p)->ss_instream)
#else
#  define INFD(p)      0
#  define INSTREAM(p)  stdin
#endif

#ifdef CONFIG_I2CTOOL_OUTDEV
#  define OUTFD(p)     ((p)->ss_outfd)
#  define OUTSTREAM(p) ((p)->ss_outstream)
#else
#  define OUTFD(p)     1
#  define OUTSTREAM(p) stdout
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct i2ctool_s
{
  /* Output streams */

#ifdef CONFIG_I2CTOOL_OUTDEV
  int    ss_outfd;     /* Output file descriptor */
  FILE  *ss_outstream; /* Output stream */
#endif
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int cmd_help(FAR void *handle, int argc, char **argv);
static int cmd_unrecognized(FAR void *handle, int argc, char **argv);

/****************************************************************************
 * Private Data
 ****************************************************************************/
 
struct i2ctool_s g_i2ctool;

static const struct cmdmap_s g_i2ccmds[] =
{
  { "?",        cmd_help,     1, 1, NULL },
  { "detect",   cmd_detect,   1, 1, NULL },
  { "dump",     cmd_dump,     1, 1, NULL },
  { "get",      cmd_get,      1, 1, NULL },
  { "help",     cmd_help,     1, 1, NULL },
  { "set",      cmd_set,      1, 1, NULL },
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Common, message formats */

const char g_syntax[]            = "i2ctool: %s: syntax error\n";
const char g_fmtargrequired[]    = "i2ctool: %s: missing required argument(s)\n";
const char g_fmtarginvalid[]     = "i2ctool: %s: argument invalid\n";
const char g_fmtargrange[]       = "i2ctool: %s: value out of range\n";
const char g_fmtcmdnotfound[]    = "i2ctool: %s: command not found\n";
const char g_fmtnosuch[]         = "i2ctool: %s: no such %s: %s\n";
const char g_fmttoomanyargs[]    = "i2ctool: %s: too many arguments\n";
const char g_fmtdeepnesting[]    = "i2ctool: %s: nesting too deep\n";
const char g_fmtcontext[]        = "i2ctool: %s: not valid in this context\n";
const char g_fmtcmdfailed[]      = "i2ctool: %s: %s failed: %d\n";
const char g_fmtcmdoutofmemory[] = "i2ctool: %s: out of memory\n";
const char g_fmtinternalerror[]  = "i2ctool: %s: Internal error\n";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cmd_help
 ****************************************************************************/

static int cmd_help(FAR void *handle, int argc, char **argv)
{
  const struct cmdmap_s *ptr;

  i2ctool_printf(handle, "Usage: i2ctool <cmd> [arguments]\n");
  i2ctool_printf(handle, "Where <cmd> is one of:\n");
  for (ptr = g_i2ccmds; ptr->cmd; ptr++)
    {
      if (ptr->usage)
        {
          i2ctool_printf(handle, "  %s %s\n", ptr->cmd, ptr->usage);
        }
      else
        {
          i2ctool_printf(handle, "  %s\n", ptr->cmd);
        }
    }

  i2ctool_printf(handle, "NOTES:\n");
#ifndef CONFIG_DISABLE_ENVIRON
  i2ctool_printf(handle, "- An environment variable like $PATH may be used for any argument.\n");
#endif
  i2ctool_printf(handle, "- Arguments are persistent.  For example, once the I2C address is\n");
  i2ctool_printf(handle, "  specified, that address will be re-used until it changes.\n");
  i2ctool_printf(handle, "WARNING:\n");
  i2ctool_printf(handle, "- The I2C detect command may have bad side effects on your I2C devices.\n");
  i2ctool_printf(handle, "  Use only at your own risk.\n");
  return OK;
}

/****************************************************************************
 * Name: cmd_unrecognized
 ****************************************************************************/

static int cmd_unrecognized(FAR void *handle, int argc, char **argv)
{
  i2ctool_printf(handle, g_fmtcmdnotfound, argv[0]);
  return ERROR;
}

/****************************************************************************
 * Name: i2c_execute
 ****************************************************************************/

static int i2c_execute(FAR void *handle, int argc, char *argv[])
{
   const struct cmdmap_s *cmdmap;
   const char            *cmd;
   cmd_t                  handler = cmd_unrecognized;
   int                    ret;

   /* The form of argv is:
    *
    * argv[0]:      The command name.  This is argv[0] when the arguments
    *               are, finally, received by the command vtblr
    * argv[1]:      The beginning of argument (up to MAX_ARGUMENTS)
    * argv[argc]:   NULL terminating pointer
    */

   cmd = argv[0];

   /* See if the command is one that we understand */

   for (cmdmap = g_i2ccmds; cmdmap->cmd; cmdmap++)
     {
       if (strcmp(cmdmap->cmd, cmd) == 0)
         {
           /* Check if a valid number of arguments was provided.  We
            * do this simple, imperfect checking here so that it does
            * not have to be performed in each command.
            */

           if (argc < cmdmap->minargs)
             {
               /* Fewer than the minimum number were provided */

               i2ctool_printf(handle, g_fmtargrequired, cmd);
               return ERROR;
             }
           else if (argc > cmdmap->maxargs)
             {
               /* More than the maximum number were provided */

               i2ctool_printf(handle, g_fmttoomanyargs, cmd);
               return ERROR;
             }
           else
             {
               /* A valid number of arguments were provided (this does
                * not mean they are right).
                */

               handler = cmdmap->handler;
               break;
             }
         }
     }

   ret = handler(handle, argc, argv);
   return ret;
}

/****************************************************************************
 * Name: i2c_argument
 ****************************************************************************/

FAR char *i2c_argument(FAR void *handle, int argc, char *argv[], int *pindex)
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

      FAR char *value = getenv(arg+1);
      if (value)
        {
          return value;
        }
      else
        {
          return (FAR char*)"";
        }
    }
#endif

  /* Return the next argument. */

  return arg;
}

/****************************************************************************
 * Name: i2c_parse
 ****************************************************************************/

int i2c_parse(FAR void *handle, int argc, char *argv[])
{
  FAR char *newargs[MAX_ARGUMENTS+1];
  FAR char *cmd;
  int       nargs;
  int       index;

  /* Initialize parser state */

  memset(argv, 0, MAX_ARGUMENTS*sizeof(FAR char *));

  /* Parse out the command, skipping the first argument (the program name)*/

  index = 1;
  cmd = i2c_argument(handle, argc, argv, &index);

  /* Check if any command was provided */

  if (!cmd)
    {
      /* An empty line is not an error and an unprocessed command cannot
       * generate an error, but neither should they change the last
       * command status.
       */

      return cmd_help(handle, 0, NULL);
    }

  /* Parse all of the arguments following the command name. */

  newargs[0] = cmd;
  for (nargs = 1; nargs < MAX_ARGUMENTS; nargs++)
    {
      newargs[nargs] = i2c_argument(handle, argc, argv, &index);
      if (!newargs[nargs])
        {
          break;
        }
    }
  newargs[nargs] = NULL;

  /* Then execute the command */

  return i2c_execute(handle, nargs, newargs);
}

/****************************************************************************
 * Name: i2c_setup
 ****************************************************************************/

static inline int i2c_setup(void)
{
  /* Initialize the output stream */

#ifdef CONFIG_I2CTOOL_OUTDEV
  g_i2ctool.ss_outfd = open(CONFIG_I2CTOOL_OUTDEV, O_WRONLY);
  if (g_i2ctool.ss_outfd < 0)
    {
      fprintf(stderr, g_fmtcmdfailed, "open", errno);
      return ERROR;
    }

  /* Create a standard C stream on the console device */

  g_i2ctool.ss_outstream = fdopen(g_i2ctool.ss_outfd, "w");
  if (!g_i2ctool.ss_outstream)
    {
      fprintf(stderr, g_fmtcmdfailed, "fdopen", errno);
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

static void i2c_teardown(void)
{
  fflush(OUTSTREAM(&g_i2ctool));

#ifdef CONFIG_I2CTOOL_OUTDEV
  fclose(g_i2ctool.ss_outstream);
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: i2c_main
 ****************************************************************************/

#ifdef CONFIG_I2CTOOL_BUILTIN
#  define MAIN_NAME i2c_main
#  define MAIN_NAME_STRING "i2c_main"
#else
#  define MAIN_NAME user_start
#  define MAIN_NAME_STRING "user_start"
#endif

int MAIN_NAME(int argc, char *argv[])
{
  /* Parse process the command line */

  i2c_setup();
  (void)i2c_parse((FAR void *)&g_i2ctool, argc, argv);

  i2c_teardown();
  return OK;
}

/****************************************************************************
 * Name: i2ctool_printf
 *
 * Description:
 *   Print a string to the currently selected stream.
 *
 ****************************************************************************/

int i2ctool_printf(FAR void *handle, const char *fmt, ...)
{
#ifdef CONFIG_I2CTOOL_OUTDEV
  FAR struct i2ctool_s *pstate = (FAR struct i2ctool_s *)handle;
#endif
  va_list ap;
  int     ret;

  va_start(ap, fmt);
  ret = vfprintf(OUTSTREAM(pstate), fmt, ap);
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

ssize_t i2ctool_write(FAR void *handle, FAR const void *buffer, size_t nbytes)
{
#ifdef CONFIG_I2CTOOL_OUTDEV
  FAR struct i2ctool_s *pstate = (FAR struct i2ctool_s *)handle;
#endif
  ssize_t ret;

  /* Write the data to the output stream */

  ret = fwrite(buffer, 1, nbytes, OUTSTREAM(pstate));
  if (ret < 0)
    {
      dbg("[%d] Failed to send buffer: %d\n", OUTFD(pstate), errno);
    }
  return ret;
}

