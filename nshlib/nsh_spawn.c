/****************************************************************************
 * apps/nshlib/nsh_spawn.c
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

#ifdef CONFIG_SCHED_WAITPID
#  include <sys/ioctl.h>
#  include <sys/wait.h>
#endif

#include <stdbool.h>
#include <errno.h>
#include <sched.h>
#include <string.h>
#include <spawn.h>
#include <debug.h>
#include <fcntl.h>

#include "nsh.h"
#include "nshlib/nshlib.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_spawn
 *
 * Description:
 *    Attempt to execute the application task foreground or background
 *
 * Returned Value:
 *    0 (OK)     if the application task corresponding to 'appname' was
 *               and successfully started.
 *               If wait enabled, indicates that the
 *               application returned successful status (EXIT_SUCCESS)
 *               If wait not enabled, should not be here.
 *   >0          If wait enabled, PID of the exec'ed module.
 *               If wait not enabled, indicates the application
 *               returned status.
 *
 ****************************************************************************/

int nsh_spawn(FAR const char *appname, FAR main_t main,
              FAR char * const *argv, int priority, size_t stacksize,
              FAR const char *redirfile_in, FAR const char *redirfile_out,
              int oflags, bool wait)
{
  posix_spawnattr_t attr;
  posix_spawn_file_actions_t file_actions;
  struct sched_param param;
  pid_t pid;
  int ret;

  /* Initialize attributes for task_spawn(). */

  ret = posix_spawnattr_init(&attr);
  if (ret != 0)
    {
      goto errout_with_errno;
    }

  ret = posix_spawn_file_actions_init(&file_actions);
  if (ret != 0)
    {
      goto errout_with_attrs;
    }

  /* Set the correct task size and priority */

  param.sched_priority = priority;
  ret = posix_spawnattr_setschedparam(&attr, &param);
  if (ret != 0)
    {
      goto errout_with_actions;
    }

  ret = posix_spawnattr_setstacksize(&attr, stacksize);
  if (ret != 0)
    {
      goto errout_with_actions;
    }

  /* If robin robin scheduling is enabled, then set the scheduling policy
   * of the new task to SCHED_RR before it has a chance to run.
   */

#if CONFIG_RR_INTERVAL > 0
  ret = posix_spawnattr_setschedpolicy(&attr, SCHED_RR);
  if (ret != 0)
    {
      goto errout_with_actions;
    }

  ret = posix_spawnattr_setflags(&attr,
                                 POSIX_SPAWN_SETSCHEDPARAM |
                                 POSIX_SPAWN_SETSCHEDULER);
  if (ret != 0)
    {
      goto errout_with_actions;
    }

#else
  ret = posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSCHEDPARAM);
  if (ret != 0)
    {
      goto errout_with_actions;
    }

#endif

  /* Is output being redirected? */

  if (redirfile_in)
    {
      /* Set up to close open redirfile_out and set to stdout (1) */

      ret = posix_spawn_file_actions_addopen(&file_actions, STDIN_FILENO,
                                             redirfile_in, O_RDONLY, 0400);
      if (ret != 0)
        {
          serr("ERROR: posix_spawn_file_actions_addopen failed: %d\n", ret);
          goto errout_with_actions;
        }
    }

  if (redirfile_out)
    {
      /* Set up to close open redirfile_out and set to stdout (1) */

      ret = posix_spawn_file_actions_addopen(&file_actions, STDOUT_FILENO,
                                             redirfile_out, oflags, 0200);
      if (ret != 0)
        {
          serr("ERROR: posix_spawn_file_actions_addopen failed: %d\n", ret);
          goto errout_with_actions;
        }
    }

#ifdef CONFIG_LIBC_EXECFUNCS
  /* Load and execute the application. */

  ret = posix_spawnp(&pid, appname, &file_actions, &attr, argv, NULL);
  if (ret != 0 && main != NULL)
#endif
    {
#ifndef CONFIG_BUILD_KERNEL
      /* Start the built-in */

      pid = task_spawn(appname, main, &file_actions,
                       &attr, argv ? &argv[1] : NULL, NULL);
      if (pid < 0)
        {
          ret = -pid;
        }
      else
        {
          ret = OK;
        }
#endif
    }

  /* Free attributes and file actions.  Ignoring return values in the case
   * of an error.
   */

  if (ret != 0)
    {
      serr("ERROR: task_spawn failed: %d\n", ret);
      goto errout_with_actions;
    }

  /* Return the task ID of the new task if the task was successfully
   * started.  Otherwise, ret will be ERROR (and the errno value will
   * be set appropriately).
   */

  posix_spawn_file_actions_destroy(&file_actions);
  posix_spawnattr_destroy(&attr);

#if defined(CONFIG_SCHED_WAITPID)
  if (wait)
    {
      int rc;
      ret = waitpid(pid, &rc, 0);
      if (ret < 0)
        {
          ret = ERROR;
        }
      else
        {
          ret = rc;
        }
    }
  else
#endif
    {
      ret = pid;
    }

  return ret;

errout_with_actions:
  posix_spawn_file_actions_destroy(&file_actions);

errout_with_attrs:
  posix_spawnattr_destroy(&attr);

errout_with_errno:
  errno = ret;
  return ERROR;
}
