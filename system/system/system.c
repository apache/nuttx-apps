/****************************************************************************
 * apps/system/system/system.c
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

#include <sys/wait.h>
#include <stdlib.h>
#include <sched.h>
#include <assert.h>

#include "nshlib/nshlib.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: system
 *
 * Description:
 *   Use system to pass a command string to the NSH parser and wait for it
 *   to finish executing.
 *
 *   This is an experimental version with known incompatibilies:
 *
 *   1. It is not a part of libc due to its close association with NSH.  The
 *      function is still prototyped in nuttx/include/stdlib.h, however.
 *   2. It cannot use /bin/sh since that program will not exist in most
 *      embedded configurations.  Rather, it will spawn a shell-specific
 *      system command -- currenly only NSH.
 *   3. REVISIT: There may be some issues with returned values.
 *   4. Of course, only NSH commands will be supported so that still means
 *      that many leveraged system() calls will still not be functional.
 *
 ****************************************************************************/

int system(FAR char *cmd)
{
  FAR char *nshargv[3];
  int pid;
  int rc;
  int ret;

  DEBUGASSERT(cmd != NULL);

  /* Spawn nsh_system() which will execute the command under the shell */

  nshargv[0] = "system";
  nshargv[1] = cmd;
  nshargv[2] = NULL;

  pid = task_create("system", CONFIG_SYSTEM_SYSTEM_PRIORITY,
                    CONFIG_SYSTEM_SYSTEM_STACKSIZE, nsh_system,
                    (FAR char * const *)nshargv);
  if (pid < 0)
    {
      return EXIT_FAILURE;
    }

  /* Wait for the shell to return */

  ret = waitpid(pid, &rc, 0);
  if (ret < 0)
    {
      return EXIT_FAILURE;
    }

  return rc;
}
