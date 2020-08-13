/****************************************************************************
 * apps/nshlib/nsh_stdsession.c
 *
 *   Copyright (C) 2013-2014, 2016, 2019 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
 *   This is the common session logic or an NSH session that uses only stdin
 *   and stdout.
 *
 *   This function:
 *   - Executes the NSH logic script
 *   - Presents a greeting
 *   - Then provides a prompt then gets and processes the command line.
 *   - This continues until an error occurs, then the session returns.
 *
 * Input Parameters:
 *   pstate - Abstracts the underlying session.
 *
 ****************************************************************************/

int nsh_session(FAR struct console_stdio_s *pstate,
                bool login, int argc, FAR char *argv[])
{
  FAR struct nsh_vtbl_s *vtbl;
  int ret = EXIT_FAILURE;

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
    }

  if (argc < 2)
    {
      /* Then enter the command line parsing loop */

      for (; ; )
        {
          /* For the case of debugging the USB console...
           * dump collected USB trace data
           */

#ifdef CONFIG_NSH_USBDEV_TRACE
          nsh_usbtrace();
#endif

          /* Get the next line of input. */

#ifdef CONFIG_NSH_CLE
          /* cle() normally returns the number of characters read, but will
           * return a negated errno value on end of file or if an error
           * occurs. Either will cause the session to terminate.
           */

          ret = cle(pstate->cn_line, g_nshprompt, CONFIG_NSH_LINELEN,
                    stdin, stdout);
          if (ret < 0)
            {
              printf(g_fmtcmdfailed,
                     "nsh_session", "cle", NSH_ERRNO_OF(-ret));
              continue;
            }
#else
          /* Display the prompt string */

          printf("%s", g_nshprompt);

          /* readline () normally returns the number of characters read, but
           * will return EOF on end of file or if an error occurs.  Either
           * will cause the session to terminate.
           */

          ret = std_readline(pstate->cn_line, CONFIG_NSH_LINELEN);
          if (ret == EOF)
            {
              /* NOTE: readline() does not set the errno variable, but
               * perhaps we will be lucky and it will still be valid.
               */

              printf(g_fmtcmdfailed, "nsh_session", "readline", NSH_ERRNO);
              ret = EXIT_SUCCESS;
              break;
            }
#endif

          /* Parse process the command */

          nsh_parse(vtbl, pstate->cn_line);
        }
    }
  else if (strcmp(argv[1], "-h") == 0)
    {
      ret = nsh_output(vtbl, "Usage: %s [<script-path>|-c <command>]\n",
                       argv[0]);
    }
  else if (strcmp(argv[1], "-c") != 0)
    {
#if defined(CONFIG_FILE_STREAM) && !defined(CONFIG_NSH_DISABLESCRIPT)
      /* Execute the shell script */

      ret = nsh_script(vtbl, argv[0], argv[1]);
#endif
    }
  else if (argc >= 3)
    {
      /* Parse process the command */

      ret = nsh_parse(vtbl, argv[2]);
#ifdef CONFIG_FILE_STREAM
      fflush(pstate->cn_outstream);
#endif
    }

  return ret;
}
