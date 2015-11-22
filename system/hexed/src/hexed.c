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

#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bfile.h"
#include "cmdargs.h"
#include "hexed.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

int g_wordsize;
FAR struct bfile_s *g_hexfile = NULL;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static FAR struct command_s g_cmdtbl[CMD_MAX_CNT];
static FAR struct command_s *g_cmd;

/* Valid args */

static struct arglist_s g_arglist[] =
{
  {"?", 0},
  {"co", CMDARGS_FL_OPTREQ},
  {"c", CMDARGS_FL_OPTREQ},
  {"d", CMDARGS_FL_OPTREQ},
  {"e", CMDARGS_FL_OPTREQ},
  {"f", CMDARGS_FL_OPTREQ},
  {"i", CMDARGS_FL_OPTREQ},
  {"mo", CMDARGS_FL_OPTREQ},
  {"m", CMDARGS_FL_OPTREQ},
  {"r", CMDARGS_FL_OPTREQ},
  {"w", CMDARGS_FL_OPTREQ},
  {NULL, 0}
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Print hexed_error msg and exit */

void hexed_error(int eno, const char *fmt, ...)
{
  va_list va;

  va_start(va, fmt);
  vfprintf(stderr, fmt, va);
  va_end(va);
  exit(-eno);
}

/* Print 1-8 byte hexadecimal number */

void printhex(uint64_t i, int size)
{
  switch (size)
    {
    case WORD_64:
      printf("%016llx", (unsigned long long) i);
      break;

    case WORD_32:
      printf("%08lx", (unsigned long) i);
      break;

    case WORD_16:
      printf("%04x", (unsigned int) i);
      break;

    case WORD_8:
      printf("%02x", (unsigned int) i);
      break;
    }
}

/* Load a Buffered File hexfile */

int loadfile(char *name)
{
  /* No file name */

  if (name == NULL)
    {
      errno = ENOENT;
      return -errno;
    }

  /* Open file */

  g_hexfile = bfopen(name, "r+b");
  if (g_hexfile == NULL && errno == ENOENT)
    {
      /* Create file */

      g_hexfile = bfopen(name, "w+b");
    }

  /* File didn't open */

  if (g_hexfile == NULL)
    {
      return -errno;
    }

  bfread(g_hexfile);
  return 0;
}

/* Save a buffered file hexfile */
 
int savefile(FAR struct bfile_s *file)
{
  if (file == NULL || file->fp == NULL)
    {
      errno = EBADF;
      return -errno;
    }

  bftruncate(file, file->size);
  return bfflush(file);
}

/* Set or run a command */

int hexcmd(FAR struct command_s *cmd, int optc, char *opt)
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
      RETURN_ERR(EINVAL);
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
      errno = E2BIG;
      perror(PNAME);
      exit(-errno);
    }

  /* Set defaults */

  cmd = &g_cmdtbl[cmdno];
  memset(cmd, sizeof(struct command_s), 0);
  cmd->id = cmdargs->argid;
  cmd->cmd = cmdargs->arg;
  cmd->flags = CMD_FL_CMDLINE;
  cmd->opts.word = g_wordsize;
  return cmd;
}

/* Parse the command line arguments */

int parseargs(char *argv[])
{
  FAR char *fname;
  FAR char *opt;
  int cmdcnt;
  int optc;

  fname = NULL;
  g_cmd = g_cmdtbl;                 /* 1st command is reserved */
  cmdcnt = 0;
  memset(g_cmd, sizeof(struct command_s), 0);
  while (parsecmdargs(argv, g_arglist) != -1)
    {
      /* Set new command */

      if (cmdargs->argid > 0)
        {
          cmdcnt++;
          optc = 0;
          opt = cmdargs->opt;
          g_cmd = newcmd(cmdcnt);

          /* Unknown arg */
        }
      else
        {
          opt = cmdargs->arg;
        }

      /* Set command options */

      if ((optc = hexcmd(g_cmd, optc, opt)) < 0)
        {
          /* End of range option? */

          if (*opt == '-')
            {
              /* Unknown option */

              if (strlen(opt) > 1 && *(opt + 1) != '-')
                {
                  hexed_error(EINVAL, "Unknown option: %s\n", opt);
                }

               /* Might be file name */
            }
          else if (fname == NULL)
            {
              fname = opt;
              loadfile(fname);

              /* Unexpected option */
            }
          else
            {
              hexed_error(EINVAL, "Unexpected option: %s\n", opt);
            }
        }

      /* End of command options */

      if (optc <= 0)
        {
          g_cmd = g_cmdtbl;
          optc = 0;
        }
    }

  return 0;
}

/* Run arguments from command line */

int runargs(void)
{
  int quit;
  int i;

  /* Scan for help command */

  quit = 0;
  g_cmd = &g_cmdtbl[1];             /* Skip 1st reserved command */

  for (i = 1; i < CMD_MAX_CNT && g_cmd->id > 0; i++, g_cmd++)
    {
      if (g_cmd->id == CMD_HELP)
        {
          hexcmd(g_cmd, -1, NULL);
          quit = CMD_QUIT;
          break;
        }
    }

  /* Loop through command list */

  if (quit != CMD_QUIT)
    {
      g_cmd = &g_cmdtbl[1];         /* Skip 1st reserved command */
      for (i = 1; i < CMD_MAX_CNT && g_cmd->id > 0; i++, g_cmd++)
        {
          hexcmd(g_cmd, -1, NULL);

          if ((g_cmd->flags & CMD_FL_QUIT) != 0)
            {
              quit = CMD_QUIT;
            }
        }
    }

  /* Save file */

  if (quit == CMD_QUIT && g_hexfile->flags == BFILE_FL_DIRTY)
    {
      savefile(g_hexfile);
    }

  return quit;
}

/* hexed - Hexadecimal File Editor */

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int hexed_main(int argc, char *argv[])
#endif
{
  g_wordsize = WORD_8;

  /* Check command line arguments */

  parseargs(argv);

  /* Open file */

  if (g_hexfile == NULL)
    {
      loadfile(TEMP_FILE);
    }

  /* Run commands */

  runargs();
  bfclose(g_hexfile);
  (void)unlink(TEMP_FILE);
  return 0;
}
