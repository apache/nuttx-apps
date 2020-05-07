/****************************************************************************
 * apps/system/system/system.c
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
  FAR char *argv[3];
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

  errcode = task_spawnattr_setstacksize(&attr,
                                        CONFIG_SYSTEM_SYSTEM_STACKSIZE);
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

  argv[0] = "-c";
  argv[1] = (FAR char *)cmd;
  argv[2] = NULL;

#ifdef CONFIG_SYSTEM_SYSTEM_SHPATH
  errcode = posix_spawn(&pid, CONFIG_SYSTEM_SYSTEM_SHPATH,  NULL, &attr,
                        argv, (FAR char * const *)NULL);
#else
  errcode = task_spawn(&pid, "system", nsh_system, NULL, &attr,
                       argv, (FAR char * const *)NULL);
#endif

  /* Release the attributes and check for an error from the spawn operation */

  if (errcode != 0)
    {
      serr("ERROR: Spawn failed: %d\n", errcode);
      goto errout_with_attrs;
    }

  /* Wait for the shell to return */

  ret = waitpid(pid, &rc, 0);
  if (ret < 0)
    {
      /* The errno variable has already been set */

      rc = ERROR;
    }

  posix_spawnattr_destroy(&attr);
  return rc;

errout_with_attrs:
  posix_spawnattr_destroy(&attr);

errout:
  errno = errcode;
  return ERROR;
}
