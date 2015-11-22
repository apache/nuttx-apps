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
#include <string.h>
#include "cmdargs.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

FAR struct cmdargs_s *cmdargs = NULL;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct cmdargs_s _cmdargs;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Parse the command line arguments */

int parsecmdargs(FAR char *argv[], FAR struct arglist_s *arglist)
{
  int i;
  char *arg;

  /* Set cmdargs */

  if (cmdargs != &_cmdargs)
    {
      cmdargs = &_cmdargs;
      memset(cmdargs, 0, sizeof(struct cmdargs_s));
    }

  cmdargs->argid = 0;

  /* Get next arg */

  if ((cmdargs->flags & CMDARGS_FL_SINGLE) == 0)
    {
      cmdargs->idx++;
      arg = argv[cmdargs->idx];
      cmdargs->flags = 0;

      /* Single step through this arg */
    }
  else
    {
      arg = cmdargs->arg + 1;
      cmdargs->flags &= CMDARGS_FL_RESET;
    }

  /* Error or end of command line */

  if (argv == NULL || arglist == NULL || argv[cmdargs->idx] == NULL)
    {
      memset(cmdargs, 0, sizeof(struct cmdargs_s));
      return -1;
    }

  /* Check for argument */

  cmdargs->arg = arg;
  if (*arg == '-')
    {
      cmdargs->flags |= CMDARGS_FL_SETARG;
      arg++;
      if (*arg == '-')
        {
          arg++;
          cmdargs->flags |= CMDARGS_FL_LONGARG;
        }
    }

  /* Scan arglist */

  if ((cmdargs->flags & CMDARGS_FL_SETARG) != 0)
    {
      for (i = 1; arglist->name != NULL; i++, arglist++)
        {
          /* Skip long args in single step mode */

          if ((cmdargs->flags & CMDARGS_FL_SINGLE) != 0
              && strlen(arglist->name) > 1)
            {
              continue;
            }

          /* Check arg for match in arglist */

          if (strncmp(arg, arglist->name, strlen(arglist->name)) == 0)
            {
              /* The long arg flag is set but we've found a single arg match */

              if (strlen(arglist->name) == 1 &&
                  (cmdargs->flags & CMDARGS_FL_LONGARG) != 0)
                {
                  break;
                }

              cmdargs->argid = i;
              break;
            }
        }
    }

  /* Found argument */

  if (cmdargs->argid > 0)
    {
      cmdargs->arg = arg;
      cmdargs->opt = NULL;
      cmdargs->flags |= CMDARGS_FL_CALLMASK(arglist->flags);
      arg += strlen(arglist->name);

      /* Argument accepts options */

      if ((cmdargs->flags & CMDARGS_FL_OPT) != 0)
        {
          /* Option in same arg */

          if (*arg != '\0')
            {
              /* Option set after ":=" */

              if (strchr(":=", *arg))
                {
                  arg++;
                  cmdargs->opt = arg;

                  /* Error setting arg/option */
                }
              else
                {
                  cmdargs->opt = NULL;
                }

              /* Option in next arg */
            }
          else if (argv[cmdargs->idx + 1] != NULL &&
                   *argv[cmdargs->idx + 1] != '\0' &&
                   *argv[cmdargs->idx + 1] != '-')
            {
              cmdargs->idx++;
              cmdargs->opt = argv[cmdargs->idx];
            }
        }

      /* Single step through this arg */

      if (strlen(arglist->name) == 1 && *arg != '\0' &&
          (cmdargs->flags & CMDARGS_FL_OPTREQ) != CMDARGS_FL_OPTREQ &&
          cmdargs->opt == NULL)
        {
          cmdargs->flags |= CMDARGS_FL_SINGLE;
        }
      else
        {
          cmdargs->flags &= ~CMDARGS_FL_SINGLE;
        }

      /* No valid argument found */
    }
  else
    {
      cmdargs->flags = 0;
      cmdargs->opt = NULL;
    }

  return cmdargs->argid;
}
