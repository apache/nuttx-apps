/****************************************************************************
 * apps/system/hexed/src/cmdargs.c
 * Command line argument parsing
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cmdargs.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

FAR struct cmdargs_s *g_cmdargs = NULL;

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: parsecmdargs
 *
 * Description:
 *   Parse the command line arguments
 *
 * Input parmeters:
 *   argv    - The command line arguments
 *   arglist - The array or supported arguments
 *
 * Returned Value:
 *   One success, the argument ID is returned.  EOF is returned on any
 *   failure.
 *
 ****************************************************************************/

int parsecmdargs(FAR char *argv[], FAR const struct arglist_s *arglist)
{
  FAR char *arg;
  int i;

  /* Get next arg */

  g_cmdargs->argid = 0;
  if ((g_cmdargs->flags & CMDARGS_FL_SINGLE) == 0)
    {
      g_cmdargs->idx++;
      arg = argv[g_cmdargs->idx];
      g_cmdargs->flags = 0;
    }
  else
    {
      /* Skip over this argument */

      arg = g_cmdargs->arg + 1;
      g_cmdargs->flags &= CMDARGS_FL_RESET;
    }

  /* End of command line */

  if (argv[g_cmdargs->idx] == NULL)
    {
      memset(g_cmdargs, 0, sizeof(struct cmdargs_s));
      return EOF;
    }

  /* Check for argument */

  g_cmdargs->arg = arg;
  if (*arg == '-')
    {
      g_cmdargs->flags |= CMDARGS_FL_SETARG;
      arg++;
      if (*arg == '-')
        {
          arg++;
          g_cmdargs->flags |= CMDARGS_FL_LONGARG;
        }
    }

  /* Scan arglist */

  if ((g_cmdargs->flags & CMDARGS_FL_SETARG) != 0)
    {
      for (i = 1; arglist->name != NULL; i++, arglist++)
        {
          /* Skip long args in single step mode */

          if ((g_cmdargs->flags & CMDARGS_FL_SINGLE) != 0 &&
              strlen(arglist->name) > 1)
            {
              continue;
            }

          /* Check arg for match in arglist */

          if (strncmp(arg, arglist->name, strlen(arglist->name)) == 0)
            {
              /* The long arg flag is set but we've found a single arg match */

              if (strlen(arglist->name) == 1 &&
                  (g_cmdargs->flags & CMDARGS_FL_LONGARG) != 0)
                {
                  break;
                }

              g_cmdargs->argid = i;
              break;
            }
        }
    }

  /* Found argument */

  if (g_cmdargs->argid > 0)
    {
      g_cmdargs->arg = arg;
      g_cmdargs->opt = NULL;
      g_cmdargs->flags |= CMDARGS_FL_CALLMASK(arglist->flags);
      arg += strlen(arglist->name);

      /* Argument accepts options */

      if ((g_cmdargs->flags & CMDARGS_FL_OPT) != 0)
        {
          /* Option in same arg */

          if (*arg != '\0')
            {
              /* Option set after ":=" */

              if (strchr(":=", *arg))
                {
                  arg++;
                  g_cmdargs->opt = arg;

                  /* Error setting arg/option */
                }
              else
                {
                  g_cmdargs->opt = NULL;
                }

              /* Option in next arg */
            }
          else if (argv[g_cmdargs->idx + 1] != NULL &&
                   *argv[g_cmdargs->idx + 1] != '\0' &&
                   *argv[g_cmdargs->idx + 1] != '-')
            {
              g_cmdargs->idx++;
              g_cmdargs->opt = argv[g_cmdargs->idx];
            }
        }

      /* Single step through this arg */

      if (strlen(arglist->name) == 1 && *arg != '\0' &&
          (g_cmdargs->flags & CMDARGS_FL_OPTREQ) != CMDARGS_FL_OPTREQ &&
          g_cmdargs->opt == NULL)
        {
          g_cmdargs->flags |= CMDARGS_FL_SINGLE;
        }
      else
        {
          g_cmdargs->flags &= ~CMDARGS_FL_SINGLE;
        }
    }

  /* No valid argument found */

  else
    {
      g_cmdargs->flags = 0;
      g_cmdargs->opt = NULL;
    }

  return g_cmdargs->argid;
}
