/****************************************************************************
 * apps/nshlib/nsh_system.c
 *
 *   Copyright (C) 2016 Gregory Nutt. All rights reserved.
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
#include <assert.h>

#include "nsh.h"
#include "nsh_console.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_system
 *
 * Description:
 *   This is the NSH-specific implementation of the standard system()
 *   command.
 *
 *   NOTE: This assumes that other NSH instances have previously ran and so
 *   common NSH logic is already initialized.
 *
 * Input Parameters:
 *   Standard task start-up arguments.  Expects argc == 2 with argv[1] being
 *   the command to execute
 *
 * Returned Values:
 *   EXIT_SUCCESS or EXIT_FAILURE
 *
 ****************************************************************************/

int nsh_system(int argc, FAR char *argv[])
{
  FAR struct console_stdio_s *pstate = nsh_newconsole();
  FAR struct nsh_vtbl_s *vtbl;
  int ret = EXIT_FAILURE;

  DEBUGASSERT(pstate != NULL);
  vtbl = &pstate->cn_vtbl;

  if (argc < 2)
    {
      /* Execute the interactive shell */

      ret = nsh_session(pstate, false);
    }
  else if (strcmp(argv[1], "-h") == 0)
    {
      ret = nsh_output(vtbl, "Usage: %s [<script-path>|-c <command>]\n",
                       argv[0]);
    }
  else if (strcmp(argv[1], "-c") != 0)
    {
#if CONFIG_NFILE_STREAMS > 0 && !defined(CONFIG_NSH_DISABLESCRIPT)
      /* Execute the shell script */

      ret = nsh_script(vtbl, argv[0], argv[1]);
#endif
    }
  else if (argc >= 3)
    {
      /* Parse process the command */

      ret = nsh_parse(vtbl, argv[2]);
#if CONFIG_NFILE_STREAMS > 0
      fflush(pstate->cn_outstream);
#endif
    }

  /* Exit upon return */

  nsh_exit(&pstate->cn_vtbl, ret);
  return ret;
}
