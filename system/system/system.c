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
#include <spawn.h>
#include <assert.h>
#include <debug.h>

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
 *   This is an experimental version with known incompatibilities:
 *
 *   1. It is not a part of libc due to its close association with NSH.  The
 *      function is still prototyped in nuttx/include/stdlib.h, however.
 *   2. It cannot use /bin/sh since that program will not exist in most
 *      embedded configurations.  Rather, it will spawn a shell-specific
 *      system command -- currently only NSH.
 *   3. REVISIT: There may be some issues with returned values.
 *   4. Of course, only NSH commands will be supported so that still means
 *      that many leveraged system() calls will still not be functional.
 *
 ****************************************************************************/

int system(FAR const char *cmd)
{
  FAR const char *argv[2];
  struct sched_param param;
  posix_spawnattr_t attr;
  pid_t pid;
  int errcode;
  int rc;
  int ret;

  /* REVISIT: If cmd is NULL, then system() should return a non-zero value to
   * indicate if the command processor is available or zero if it is not.
   */

  DEBUGASSERT(cmd != NULL);

  /* Initialize attributes for task_spawn() (or posix_spawn()). */

  errcode = posix_spawnattr_init(&attr);
  if (errcode != 0)
    {
      goto errout;
    }

  /* Set the correct stack size and priority */

  param.sched_priority = CONFIG_SYSTEM_SYSTEM_PRIORITY;
  errcode = posix_spawnattr_setschedparam(&attr, &param);
  if (errcode != 0)
    {
      goto errout_with_attrs;
    }

  errcode = task_spawnattr_setstacksize(&attr, CONFIG_SYSTEM_SYSTEM_STACKSIZE);
  if (errcode != 0)
    {
      goto errout_with_attrs;
    }

   /* If robin robin scheduling is enabled, then set the scheduling policy
    * of the new task to SCHED_RR before it has a chance to run.
    */

#if CONFIG_RR_INTERVAL > 0
  errcode = posix_spawnattr_setschedpolicy(&attr, SCHED_RR);
  if (errcode != 0)
    {
      goto errout_with_attrs;
    }

  errcode = posix_spawnattr_setflags(&attr,
                                     POSIX_SPAWN_SETSCHEDPARAM |
                                     POSIX_SPAWN_SETSCHEDULER);
  if (errcode != 0)
    {
      goto errout_with_attrs;
    }

#else
  errcode = posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSCHEDPARAM);
  if (errcode != 0)
    {
      goto errout_with_attrs;
    }

#endif

  /* Spawn nsh_system() which will execute the command under the shell. */

  argv[0] = cmd;
  argv[1] = NULL;

#ifdef CONFIG_BUILD_LOADABLE
  errcode = posix_spawn(&pid, CONFIG_SYSTEM_OPEN_SHPATH,  NULL, &attr,
                        argv, (FAR char * const *)NULL);
#else
  errcode = task_spawn(&pid, "popen", nsh_system, NULL, &attr,
                       argv, (FAR char * const *)NULL);
#endif

  /* Release the attributes and check for an error from the spawn operation */

  if (errcode != 0)
    {
      serr("ERROR: Spawn failed: %d\n", result);
      goto errout_with_attrs;
    }

  /* Wait for the shell to return */

  ret = waitpid(pid, &rc, 0);
  if (ret < 0)
    {
      /* The errno variable has already been set */

      rc = ERROR;
    }

  (void)posix_spawnattr_destroy(&attr);
  return rc;

errout_with_attrs:
  (void)posix_spawnattr_destroy(&attr);

errout:
  set_errno(errcode);
  return ERROR;
}
