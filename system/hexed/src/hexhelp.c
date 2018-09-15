/****************************************************************************
 * apps/system/hexed/src/hexhelp.c
 * hexed help command
 *
 *   Copyright (c) 2011, B.ZaaR, All rights reserved.
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
#include <stdint.h>
#include <stdio.h>
#include "bfile.h"
#include "hexed.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char *helpmsg[] =
{
  NULL,
  "  -h                            Shows this help screen\n",
  "  -co [src] [dest] [len]        Copy data from src overwriting dest for len\n"
  "                                words\n",
  "  -c [src] [dest] [len]         Copy data from src to dest for len words\n",
  "  -d [src] [len]                Display data from src for len words\n",
  "  -e [dest] [...]               Enter hex values [...] at dest\n",
  NULL,
  "  -i [dest] [cnt] [...]         Insert hex values [...] at dest repeating cnt"
  "\n                                times. Defaults to 0 for empty hex values.\n",
  "  -mo [src] [dest] [len]        Move data from src overwriting dest for len\n"
  "                                words\n",
  "  -m [src] [dest] [len]         Move data from src to dest for len words\n",
  "  -r [src] [len]                Remove data from src for len words\n",
  "  -w [bytes]                    Set the word size in bytes: 1 = 8 bits\n"
  "                                2 = 16 bits, 4 = 32 bits, 8 = 64 bits\n",
  NULL,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: runhelp
 *
 * Description:
 *   Display the help page
 *
 ****************************************************************************/

static int runhelp(FAR struct command_s *cmd)
{
  /* Show command line help */

  printf("%s - Hexadecimal File Editor\n", PNAME);
  printf("  v%d.%d.%d\n\n", VER_MAJOR, VER_MINOR, VER_REVISION);
  printf("Usage:\n");
  printf("  %s [options] [file]\n\n", PNAME);
  printf("Options:\n");
  printf("%s", helpmsg[CMD_HELP]);
  printf("%s", helpmsg[CMD_COPY]);
  printf("%s", helpmsg[CMD_COPY_OVER]);
  printf("%s", helpmsg[CMD_DUMP]);
  printf("%s", helpmsg[CMD_ENTER]);
  printf("%s", helpmsg[CMD_INSERT]);
  printf("%s", helpmsg[CMD_MOVE]);
  printf("%s", helpmsg[CMD_MOVE_OVER]);
  printf("%s", helpmsg[CMD_REMOVE]);
  printf("%s", helpmsg[CMD_WORD]);
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: hexhelp
 *
 * Description:
 *   Help command
 *
 ****************************************************************************/

int hexhelp(FAR struct command_s *cmd, int optc, char *opt)
{
  /* Invalid command */

  if (cmd == NULL || cmd->id != CMD_HELP)
    {
      return -EINVAL;
    }

  /* Run help */

  if (optc >= 0)
    {
      optc = 0;                 /* Nothing to set */
    }
  else
    {
      cmd->flags |= CMD_FL_QUIT;
      optc = runhelp(cmd);
    }

  return optc;
}
