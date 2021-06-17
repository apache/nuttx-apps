/****************************************************************************
 * apps/nshlib/nsh_fileapps.c
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
#include <spawn.h>
#include <errno.h>
#include <string.h>

#include "nsh.h"
#include "nsh_console.h"

#ifdef CONFIG_NSH_FILE_APPS

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_fileapp
 *
 * Description:
 *    Attempt to execute the application task whose name is 'cmd'
 *
 * Returned Value:
 *   <0          If posix_spawn() fails, then the negated errno value
 *               is returned.
 *   -1 (ERROR)  if the application task corresponding to 'cmd' could not
 *               be started (possibly because it doesn not exist).
 *    0 (OK)     if the application task corresponding to 'cmd' was
 *               and successfully started.  If CONFIG_SCHED_WAITPID is
 *               defined, this return value also indicates that the
 *               application returned successful status (EXIT_SUCCESS)
 *    1          If CONFIG_SCHED_WAITPID is defined, then this return value
 *               indicates that the application task was spawned successfully
 *               but returned failure exit status.
 *
 ****************************************************************************/

int nsh_fileapp(FAR struct nsh_vtbl_s *vtbl, FAR const char *cmd,
                FAR char **argv, FAR const char *redirfile, int oflags)
{
  posix_spawn_file_actions_t file_actions;
  posix_spawnattr_t attr;
  pid_t pid;
  int rc = 0;
  int ret;

  /* Initialize the attributes file actions structure */

  ret = posix_spawn_file_actions_init(&file_actions);
  if (ret != 0)
    {
      /* posix_spawn_file_actions_init returns a positive errno value on
       * failure.
       */

      nsh_error(vtbl, g_fmtcmdfailed, cmd, "posix_spawn_file_actions_init",
                NSH_ERRNO_OF(ret));
      goto errout;
    }

  ret = posix_spawnattr_init(&attr);
  if (ret != 0)
    {
      /* posix_spawnattr_init returns a positive errno value on failure. */

      nsh_error(vtbl, g_fmtcmdfailed, cmd, "posix_spawnattr_init",
                NSH_ERRNO);
      goto errout_with_actions;
    }

  /* Handle re-direction of output */

  if (redirfile)
    {
      ret = posix_spawn_file_actions_addopen(&file_actions, 1, redirfile,
                                             oflags, 0644);
      if (ret != 0)
        {
          /* posix_spawn_file_actions_addopen returns a positive errno
           * value on failure.
           */

          nsh_error(vtbl, g_fmtcmdfailed, cmd,
                     "posix_spawn_file_actions_addopen",
                     NSH_ERRNO);
          goto errout_with_attrs;
        }
    }

  /* Lock the scheduler in an attempt to prevent the application from
   * running until waitpid() has been called.
   */

  sched_lock();

  /* Execute the program. posix_spawnp returns a positive errno value on
   * failure.
   */

  ret = posix_spawnp(&pid, cmd, &file_actions, &attr, argv, NULL);
  if (ret == OK)
    {
      /* The application was successfully started with pre-emption disabled.
       * In the simplest cases, the application will not have run because the
       * the scheduler is locked.  But in the case where I/O was redirected,
       * a proxy task ran and broke our lock.  As result, the application may
       * have aso ran if its priority was higher than than the priority of
       * this thread.
       *
       * If the application did not run to completion and if the application
       * was not backgrounded, then we need to wait here for the application
       * to exit.  This only works works with the following options:
       *
       * - CONFIG_NSH_DISABLEBG - Do not run commands in background
       * - CONFIG_SCHED_WAITPID - Required to run external commands in
       *     foreground
       */

#ifdef CONFIG_SCHED_WAITPID
      /* CONFIG_SCHED_WAITPID is selected, so we may run the command in
       * foreground unless we were specifically requested to run the command
       * in background (and running commands in background is enabled).
       */

#  ifndef CONFIG_NSH_DISABLEBG
      if (vtbl->np.np_bg == false)
#  endif /* CONFIG_NSH_DISABLEBG */
        {
          int tc = 0;

          if (vtbl->isctty)
            {
              /* Setup up to receive SIGINT if control-C entered. */

              tc = ioctl(stdout->fs_fd, TIOCSCTTY, pid);
            }

          /* Wait for the application to exit.  We did lock the scheduler
           * above, but that does not guarantee that the application did not
           * already run to completion in the case where I/O was redirected.
           * Here the scheduler will be unlocked while waitpid is waiting
           * and if the application has not yet run, it will now be able to
           * do so.
           *
           * NOTE: WUNTRACED does nothing in the default case, but in the
           * case the where CONFIG_SIG_SIGSTOP_ACTION=y, the file app
           * may also be stopped.  In that case WUNTRACED will force
           * waitpid() to return with ECHILD.
           */

          ret = waitpid(pid, &rc, WUNTRACED);
          if (ret < 0)
            {
              /* If the child thread doesn't exist, waitpid() will return the
               * error ECHLD.  Since we know that the task was successfully
               * started, this must be one of the cases described above; we
               * have to assume that the task already exit'ed.  In this case,
               * we have no idea if the application ran successfully or not
               * (because NuttX does not retain exit status of child tasks).
               * Let's assume that is did run successfully.
               */

              int errcode = errno;
              if (errcode == ECHILD)
                {
                  ret = OK;
                }
              else
                {
                  nsh_error(vtbl, g_fmtcmdfailed, cmd, "waitpid",
                            NSH_ERRNO_OF(errcode));
                }
            }

          /* Waitpid completed the wait successfully */

          else
            {
              /* Ignore the returned child pid */

              ret = OK;

              /* TODO:  Set the environment variable '?' to a string
               * corresponding to WEXITSTATUS(rc) so that $? will expand
               * to the exit status of the most recently executed task.
               */
            }

          if (vtbl->isctty && tc == 0)
            {
              ioctl(stdout->fs_fd, TIOCNOTTY);
            }
        }
#  ifndef CONFIG_NSH_DISABLEBG
      else
#  endif /* CONFIG_NSH_DISABLEBG */
#endif /* CONFIG_SCHED_WAITPID */

      /* We get here if either:
       *
       * - CONFIG_SCHED_WAITPID is not selected meaning that all commands
       *   have to be run in background, or
       * - CONFIG_SCHED_WAITPID and CONFIG_NSH_DISABLEBG are both selected,
       *   but the user requested to run the command in background.
       *
       * NOTE that the case of a) CONFIG_SCHED_WAITPID is not selected and
       * b) CONFIG_NSH_DISABLEBG selected cannot be supported. In that event,
       * all commands will have to run in background.  The waitpid() API must
       * be available to support running the command in foreground.
       */

#if !defined(CONFIG_SCHED_WAITPID) || !defined(CONFIG_NSH_DISABLEBG)
        {
          struct sched_param param;
          sched_getparam(ret, &param);
          nsh_output(vtbl, "%s [%d:%d]\n", cmd, ret, param.sched_priority);

          /* Backgrounded commands always 'succeed' as long as we can start
           * them.
           */

          ret = OK;
        }
#endif /* !CONFIG_SCHED_WAITPID || !CONFIG_NSH_DISABLEBG */
    }

  sched_unlock();

  /* Free attributes and file actions.  Ignoring return values in the case
   * of an error.
   */

errout_with_actions:
  posix_spawn_file_actions_destroy(&file_actions);

errout_with_attrs:
  posix_spawnattr_destroy(&attr);

errout:
  /* Most posix_spawn interfaces return a positive errno value on failure
   * and do not set the errno variable.
   */

  if (ret > 0)
    {
      /* Set the errno value and return -1 */

      errno = ret;
      ret = ERROR;
    }
  else if (ret < 0)
    {
      /* Return -1 on failure.  errno should have been set. */

      ret = ERROR;
    }
  else if (rc != 0)
    {
      /* We can't return the exact status (nsh has nowhere to put it)
       * so just pass back zero/nonzero in a fashion that doesn't look
       * like an error.
       */

      ret = 1;
    }

  return ret;
}

#endif /* CONFIG_NSH_FILE_APPS */
