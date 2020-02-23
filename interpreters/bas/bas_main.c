/****************************************************************************
 * apps/interpreters/bas/bas_main.c
 *
 *   Copyright (c) 1999-2014 Michael Haardt
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Adapted to NuttX and re-released under a 3-clause BSD license:
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
 *   Authors: Alan Carvalho de Assis <Alan Carvalho de Assis>
 *            Gregory Nutt <gnutt@nuttx.org>
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

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "bas_fs.h"
#include "bas.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define _(String) String

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  char *runFile = (char *)0;
  const char *lp = "/dev/null";
  int usage = 0;
  int o;
  int backslash_colon = 0;
  int uppercase = 0;
  int restricted = 0;
  int lpfd;

  /* parse arguments */

  while ((o = getopt(argc, argv, ":bl:ruVh")) != EOF)
    {
      switch (o)
        {
        case 'b':
          backslash_colon = 1;
          break;

        case 'l':
          lp = optarg;
          break;

        case 'u':
          uppercase = 1;
          break;

        case 'r':
          restricted = 1;
          break;

        case 'V':
          printf("bas %s\n", CONFIG_INTERPRETER_BAS_VERSION);
          exit(0);
          break;

        case 'h':
          usage = 2;
          break;

        default:
          usage = 1;
          break;
        }
    }

  if (optind < argc)
    {
      runFile = argv[optind++];
    }

  if (usage == 1)
    {
      fputs(_("Usage: bas [-b] [-l file] [-r] [-u] [program [argument ...]]\n"),
            stderr);
      fputs(_("       bas -h\n"), stderr);
      fputs(_("       bas -V\n"), stderr);
      fputs("\n", stderr);
      fputs(_("Try `bas -h' for more information.\n"), stderr);
      exit(1);
    }

  if (usage == 2)
    {
      fputs(_("Usage: bas [-b] [-l file] [-u] [program [argument ...]]\n"),
            stdout);
      fputs(_("       bas -h\n"), stdout);
      fputs(_("       bas -V\n"), stdout);
      fputs("\n", stdout);
      fputs(_("BASIC interpreter.\n"), stdout);
      fputs("\n", stdout);
      fputs(_("-b  Convert backslashes to colons\n"), stdout);
      fputs(_("-l  Write LPRINT output to file\n"), stdout);
      fputs(_("-r  Forbid SHELL\n"), stdout);
      fputs(_("-u  Output all tokens in uppercase\n"),
            stdout);
      fputs(_("-h  Display this help and exit\n"), stdout);
      fputs(_("-V  Ooutput version information and exit\n"),
            stdout);
      exit(0);
    }

  if ((lpfd = open(lp, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1)
    {
      fprintf(stderr,
              _("bas: Opening `%s' for line printer output failed (%s).\n"), lp,
              strerror(errno));
      exit(2);
    }

  g_bas_argc  = argc - optind;
  g_bas_argv  = &argv[optind];
  g_bas_argv0 = runFile;
  g_bas_end   = false;

  bas_init(backslash_colon, restricted, uppercase, lpfd);
  if (runFile)
    {
      bas_runFile(runFile);
    }
  else
    {
      bas_interpreter();
    }

  /* Terminate the output stream with a newline BEFORE closing devices */

  FS_putChar(STDCHANNEL, '\n');

  /* Release resources and close files and devices */

  bas_exit();
  return 0;
}
