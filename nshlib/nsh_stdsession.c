/****************************************************************************
 * apps/nshlib/nsh_stdsession.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef CONFIG_NSH_CLE
#  include "system/cle.h"
#else
#  include "system/readline.h"
#endif

#include "nsh.h"
#include "nsh_console.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_session
 *
 * Description:
 *   This is the common session login on any NSH session that uses only stdin
 *   and stdout.
 *
 *   This function:
 *   - Performs the login sequence if so configured
 *   - Executes the NSH login script
 *   - Presents a greeting
 *   - Then provides a prompt then gets and processes the command line.
 *   - This continues until an error occurs, then the session returns.
 *
 * Input Parameters:
 *   pstate - Abstracts the underlying session.
 *
 * Returned Values:
 *   EXIT_SUCCESS or EXIT_FAILURE is returned.
 *
 ****************************************************************************/

int nsh_session(FAR struct console_stdio_s *pstate,
                bool login, int argc, FAR char *argv[])
{
  FAR struct nsh_vtbl_s *vtbl;
  int ret = EXIT_FAILURE;
  int i;

  DEBUGASSERT(pstate);
  vtbl = &pstate->cn_vtbl;

  if (login)
    {
#ifdef CONFIG_NSH_CONSOLE_LOGIN
      /* Login User and Password Check */

      if (nsh_stdlogin(pstate) != OK)
        {
          nsh_exit(vtbl, 1);
          return -1; /* nsh_exit does not return */
        }
#endif /* CONFIG_NSH_CONSOLE_LOGIN */

      /* Present a greeting and possibly a Message of the Day (MOTD) */

      printf("%s", g_nshgreeting);

#ifdef CONFIG_NSH_MOTD
# ifdef CONFIG_NSH_PLATFORM_MOTD
      /* Output the platform message of the day */

      platform_motd(vtbl->iobuffer, IOBUFFERSIZE);
      printf("%s\n", vtbl->iobuffer);

# else
      /* Output the fixed message of the day */

      printf("%s\n", g_nshmotd);
# endif
#endif

      /* Execute the login script */

#ifdef CONFIG_NSH_ROMFSRC
      nsh_loginscript(vtbl);
#endif
    }

  /* Process the command line option */

  for (i = 1; i < argc; i++)
    {
      if (strcmp(argv[i], "-h") == 0)
        {
          nsh_output(vtbl, "Usage: %s [<script-path>|-c <command>]\n",
                     argv[0]);
          return EXIT_SUCCESS;
        }
      else if (strcmp(argv[i], "-c") == 0)
        {
          /* Process the inline command */

          if (i + 1 < argc)
            {
              return nsh_parse(vtbl, argv[i + 1]);
            }
          else
            {
              nsh_error(vtbl, g_fmtargrequired, argv[0]);
              return EXIT_FAILURE;
            }
        }
      else if (argv[i][0] != '-')
        {
          break;
        }

      /* Unknown option */

      nsh_error(vtbl, g_fmtsyntax, argv[0]);
      return EXIT_FAILURE;
    }

  if (i < argc)
    {
#ifndef CONFIG_NSH_DISABLESCRIPT
      /* Execute the shell script */

      return nsh_script(vtbl, argv[0], argv[i]);
#else
      return EXIT_FAILURE;
#endif
    }

  /* Then enter the command line parsing loop */

  for (; ; )
    {
      /* For the case of debugging the USB console...
       * dump collected USB trace data
       */

#ifdef CONFIG_NSH_USBDEV_TRACE
      nsh_usbtrace();
#endif

      /* Get the next line of input. readline() returns EOF
       * on end-of-file or any read failure.
       */

#ifdef CONFIG_NSH_CLE
      /* cle() normally returns the number of characters read, but will
       * return a negated errno value on end of file or if an error
       * occurs. Either  will cause the session to terminate.
       */

      ret = cle(pstate->cn_line, g_nshprompt, CONFIG_NSH_LINELEN,
                stdin, stdout);
      if (ret < 0)
        {
          printf(g_fmtcmdfailed, "nsh_session",
                 "cle", NSH_ERRNO_OF(-ret));
          continue;
        }
#else
      /* Display the prompt string */

      printf("%s", g_nshprompt);

      /* readline() normally returns the number of characters read, but
       * will return EOF on end of file or if an error occurs.  EOF
       * will cause the session to terminate.
       */

      ret = std_readline(pstate->cn_line, CONFIG_NSH_LINELEN);
      if (ret == EOF)
        {
          /* NOTE: readline() does not set the errno variable, but
           * perhaps we will be lucky and it will still be valid.
           */

          printf(g_fmtcmdfailed, "nsh_session",
                 "readline", NSH_ERRNO);
          ret = EXIT_SUCCESS;
          break;
        }
#endif

      /* Parse process the command */

      nsh_parse(vtbl, pstate->cn_line);
    }

  return ret;
}
