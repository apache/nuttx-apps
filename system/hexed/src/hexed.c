/****************************************************************************
 * apps/system/hexed/src/hexed.c
 * Command line HEXadecimal file EDitor
 *
 *   Copyright (c) 2010, B.ZaaR, All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 *   Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 *   The names of contributors may not be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "bfile.h"
#include "cmdargs.h"
#include "hexed.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

FAR struct bfile_s *g_hexfile = NULL;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static FAR struct command_s g_cmdtbl[CMD_MAX_CNT];
static int g_ncmds;

/* Valid args */

static const struct arglist_s g_arglist[] =
{
  {"h",  0},
  {"co", CMDARGS_FL_OPTREQ},
  {"c",  CMDARGS_FL_OPTREQ},
  {"d",  CMDARGS_FL_OPTREQ},
  {"e",  CMDARGS_FL_OPTREQ},
  {"f",  CMDARGS_FL_OPTREQ},
  {"i",  CMDARGS_FL_OPTREQ},
  {"mo", CMDARGS_FL_OPTREQ},
  {"m",  CMDARGS_FL_OPTREQ},
  {"r",  CMDARGS_FL_OPTREQ},
  {"w",  CMDARGS_FL_OPTREQ},
  {NULL, 0}
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Print error msg and exit */

void hexed_fatal(FAR const char *fmt, ...)
{
  va_list va;

  va_start(va, fmt);
  vfprintf(stderr, fmt, va);
  va_end(va);
  exit(1);
}

/* Load a Buffered File hexfile */

int loadfile(FAR char *name)
{
  /* No file name */

  if (name == NULL)
    {
      return -ENOENT;
    }

  /* Open file */

  g_hexfile = bfopen(name, "r+b");
  if (g_hexfile == NULL)
    {
      /* Create file */

      g_hexfile = bfopen(name, "w+b");
    }

  /* File didn't open */

  if (g_hexfile == NULL)
    {
      fprintf(stderr, "ERROR: Failed to open %s\n", name);
      return -ENOENT;
    }

  bfread(g_hexfile);
  return 0;
}

/* Save a buffered file hexfile */

int savefile(FAR struct bfile_s *file)
{
  if (file == NULL || file->fp == NULL)
    {
      return -EBADF;
    }

  bftruncate(file, file->size);
  return bfflush(file);
}

/* Set or run a command */

int hexcmd(FAR struct command_s *cmd, int optc, FAR char *opt)
{
  switch (cmd->id)
    {
    case CMD_COPY:
      optc = hexcopy(cmd, optc, opt);
      break;

    case CMD_COPY_OVER:
      optc = hexcopy(cmd, optc, opt);
      break;

    case CMD_DUMP:
      optc = hexdump(cmd, optc, opt);
      break;

    case CMD_ENTER:
      optc = hexenter(cmd, optc, opt);
      break;

    case CMD_FIND:
      optc = 0;
      break;

    case CMD_HELP:
      optc = hexhelp(cmd, optc, opt);
      break;

    case CMD_INSERT:
      optc = hexinsert(cmd, optc, opt);
      break;

    case CMD_MOVE:
      optc = hexmove(cmd, optc, opt);
      break;

    case CMD_MOVE_OVER:
      optc = hexmove(cmd, optc, opt);
      break;

    case CMD_REMOVE:
      optc = hexremove(cmd, optc, opt);
      break;

    case CMD_WORD:
      optc = hexword(cmd, optc, opt);
      break;

    default:
      return -EINVAL;
    }

  return optc;
}

/* New command found in the args */

FAR struct command_s *newcmd(int cmdno)
{
  FAR struct command_s *cmd;

  /* Command buffer overflow */

  if (cmdno >= CMD_MAX_CNT)
    {
      fprintf(stderr, "ERROR: Command buffer overflow\n");
      exit(EXIT_FAILURE);
    }

  /* Set defaults */

  cmd            = &g_cmdtbl[cmdno];
  cmd->id        = g_cmdargs->argid;
  cmd->cmd       = g_cmdargs->arg;
  cmd->flags     = CMD_FL_CMDLINE;
  cmd->opts.word = g_wordsize;
  return cmd;
}

/* Parse the command line arguments */

int parseargs(FAR char *argv[])
{
  FAR struct command_s *cmd;
  FAR char *fname;
  FAR char *opt;
  int optc  = 0;

  fname     = NULL;
  cmd       = g_cmdtbl;
  g_ncmds   = 0;
  g_hexfile = NULL;

  memset(g_cmdtbl, 0, CMD_MAX_CNT * sizeof(struct command_s));

  while (parsecmdargs(argv, g_arglist) != EOF)
    {
      /* Set new command */

      if (g_cmdargs->argid > 0)
        {
          g_ncmds++;
          optc = 0;
          opt  = g_cmdargs->opt;
          cmd  = newcmd(g_ncmds);
        }
      else
        {
          /* Unknown arg */

          opt = g_cmdargs->arg;
        }

      /* Set command options */

      optc = hexcmd(cmd, optc, opt);
      if (optc < 0)
        {
          /* End of range option? */

          if (*opt == '-')
            {
              /* Unknown option */

              if (strlen(opt) > 1 && *(opt + 1) != '-')
                {
                  hexed_fatal("ERROR: Unknown option: %s\n", opt);
                }
            }

          /* Might be file name */

          else if (fname == NULL)
            {
              fname = opt;
              loadfile(fname);
            }

          /* Unexpected option */

          else
            {
              hexed_fatal("ERROR: Unexpected option: %s\n", opt);
            }
        }

      /* End of command options */

      if (optc <= 0)
        {
          cmd = g_cmdtbl;
          optc = 0;
        }
    }

  return 0;
}

/* Run arguments from command line */

int runargs(void)
{
  FAR struct command_s *cmd;
  int quit;
  int i;

  /* Check for no commands */

  if (g_ncmds == 0)
    {
      return CMD_QUIT;
    }

  /* Scan for help command */

  quit  = 0;
  cmd = &g_cmdtbl[1];             /* Skip 1st reserved command */

  for (i = 1; i < g_ncmds + 1; i++, cmd++)
    {
      if (cmd->id == CMD_HELP)
        {
          hexcmd(cmd, -1, NULL);
          quit = CMD_QUIT;
          break;
        }
    }

  /* Loop through command list */

  if (quit != CMD_QUIT)
    {
      /* All other commands require a filename argument */

      if (g_hexfile == NULL)
        {
          fprintf(stderr, "ERROR: Missing filename\n");
          return -EINVAL;
        }

      /* Process every command on the command line */

      cmd = &g_cmdtbl[1];         /* Skip 1st reserved command */
      for (i = 1; i < g_ncmds + 1; i++, cmd++)
        {
          hexcmd(cmd, -1, NULL);

          if ((cmd->flags & CMD_FL_QUIT) != 0)
            {
              quit = CMD_QUIT;
            }
        }
    }

  /* Save file */

  if (quit == CMD_QUIT &&
      g_hexfile != NULL &&
      g_hexfile->flags == BFILE_FL_DIRTY)
    {
      savefile(g_hexfile);
    }

  return quit;
}

/* hexed - Hexadecimal File Editor */

int main(int argc, FAR char *argv[])
{
  struct cmdargs_s args;

  /* Set cmdargs global reference (if already not set) */

  memset(&args, 0, sizeof(struct cmdargs_s));
  g_cmdargs = &args;

  /* Check command line arguments */

  g_wordsize = WORD_8;
  g_hexfile  = NULL;

  parseargs(argv);

  /* Run commands */

  runargs();
  if (g_hexfile)
    {
      bfclose(g_hexfile);
      g_hexfile = NULL;
    }

  return 0;
}
