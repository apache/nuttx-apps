/****************************************************************************
 * apps/system/popen/dpopen.c
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
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>
#include <spawn.h>
#include <debug.h>
#include <fcntl.h>
#include <errno.h>

#if defined(CONFIG_NET_LOCAL) && defined(CONFIG_NET_LOCAL_STREAM)
#  include <sys/socket.h>
#endif

#ifdef CONFIG_NSH_LIBRARY
#  include "nshlib/nshlib.h"
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_NSH_LIBRARY
#  define DPOPEN_MAX_ARGV (CONFIG_SYSTEM_POPEN_MAXARGUMENTS + 1)
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: dpopen
 *
 * Description:
 *   Execute a command and return a raw file descriptor connected to the
 *   child process via a pipe, along with the child PID.
 *
 *   This is the descriptor-based counterpart of popen(), analogous to how
 *   dprintf() relates to fprintf().  It avoids the FILE stream layer,
 *   making it suitable for callers that only need a raw fd and want to
 *   avoid the stdio.h dependency.
 *
 *   When NSH is available, commands are executed through the shell
 *   (sh -c command), supporting full shell syntax.
 *
 *   When NSH is not available, the command is split by whitespace and
 *   executed directly via posix_spawnp().  Shell syntax (pipes, redirects,
 *   globbing) is not supported in this mode.
 *
 * Input Parameters:
 *   command - The command string to execute
 *   oflag   - O_RDONLY to read child stdout, O_WRONLY to write child stdin,
 *             O_RDWR for bidirectional socket mode
 *   pid     - Location to return the child process ID
 *
 * Returned Value:
 *   A valid file descriptor on success, or -1 on failure with errno set.
 *
 ****************************************************************************/

int dpopen(FAR const char *command, int oflag, FAR pid_t *pid)
{
  struct sched_param param;
  posix_spawnattr_t attr;
  posix_spawn_file_actions_t file_actions;
#ifdef CONFIG_NSH_LIBRARY
  FAR char *argv[4];
#else
  char cmdbuf[PATH_MAX];
  FAR char *argv[DPOPEN_MAX_ARGV];
  FAR char *saveptr;
  int argc = 0;
#endif
  int fd[2];
  int childfd;
  int parentfd;
  int errcode;

  if (command == NULL || pid == NULL)
    {
      errno = EINVAL;
      return -1;
    }

  /* Create a pipe or socketpair.  fd[0] refers to the read end of the
   * pipe; fd[1] refers to the write end of the pipe.
   */

  if ((oflag & O_ACCMODE) == O_RDWR)
    {
#if defined(CONFIG_NET_LOCAL) && defined(CONFIG_NET_LOCAL_STREAM)
      /* Create a socketpair for bidirectional communication */

      if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, fd) < 0)
        {
          return -1;
        }

      childfd  = fd[0];
      parentfd = fd[1];
#else
      errno = ENOTSUP;
      return -1;
#endif
    }
  else if (pipe2(fd, O_CLOEXEC) < 0)
    {
      return -1;
    }
  else if ((oflag & O_ACCMODE) == O_RDONLY)
    {
      /* Pipe is the output from the child */

      childfd  = fd[1];  /* Child writes to pipe */
      parentfd = fd[0];  /* Parent reads from pipe */
    }
  else if ((oflag & O_ACCMODE) == O_WRONLY)
    {
      /* Pipe is the input to the child */

      childfd  = fd[0];  /* Child reads from pipe */
      parentfd = fd[1];  /* Parent writes to pipe */
    }
  else
    {
      errcode = EINVAL;
      goto errout_with_pipe;
    }

  /* Initialize attributes for task_spawn() (or posix_spawn()). */

  errcode = posix_spawnattr_init(&attr);
  if (errcode != 0)
    {
      goto errout_with_pipe;
    }

  errcode = posix_spawn_file_actions_init(&file_actions);
  if (errcode != 0)
    {
      goto errout_with_attr;
    }

  /* Set the correct stack size and priority */

  param.sched_priority = CONFIG_SYSTEM_POPEN_PRIORITY;
  errcode = posix_spawnattr_setschedparam(&attr, &param);
  if (errcode != 0)
    {
      goto errout_with_actions;
    }

#ifndef CONFIG_SYSTEM_POPEN_SHPATH
  errcode = posix_spawnattr_setstacksize(&attr,
                                         CONFIG_SYSTEM_POPEN_STACKSIZE);
  if (errcode != 0)
    {
      goto errout_with_actions;
    }
#endif

  /* If robin robin scheduling is enabled, then set the scheduling policy
   * of the new task to SCHED_RR before it has a chance to run.
   */

#if CONFIG_RR_INTERVAL > 0
  errcode = posix_spawnattr_setschedpolicy(&attr, SCHED_RR);
  if (errcode != 0)
    {
      goto errout_with_actions;
    }

  errcode = posix_spawnattr_setflags(&attr,
                                     POSIX_SPAWN_SETSCHEDPARAM |
                                     POSIX_SPAWN_SETSCHEDULER);
  if (errcode != 0)
    {
      goto errout_with_actions;
    }

#else
  errcode = posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSCHEDPARAM);
  if (errcode != 0)
    {
      goto errout_with_actions;
    }

#endif

  /* Redirect child's stdin or stdout to the pipe */

  if ((oflag & O_ACCMODE) == O_RDWR)
    {
      errcode = posix_spawn_file_actions_adddup2(&file_actions, childfd,
                                                 STDIN_FILENO);
      if (errcode != 0)
        {
          goto errout_with_actions;
        }

      errcode = posix_spawn_file_actions_adddup2(&file_actions, childfd,
                                                 STDOUT_FILENO);
      if (errcode != 0)
        {
          goto errout_with_actions;
        }
    }
  else
    {
      errcode = posix_spawn_file_actions_adddup2(&file_actions, childfd,
                   (oflag & O_ACCMODE) == O_RDONLY ?
                   STDOUT_FILENO : STDIN_FILENO);
      if (errcode != 0)
        {
          goto errout_with_actions;
        }
    }

  /* Call task_spawn() (or posix_spawn), re-directing stdin or stdout
   * appropriately.
   */

#ifdef CONFIG_NSH_LIBRARY
  /* Shell mode: execute command through sh -c */

  argv[1] = "-c";
  argv[2] = (FAR char *)command;
  argv[3] = NULL;

#  ifdef CONFIG_SYSTEM_POPEN_SHPATH
  argv[0] = CONFIG_SYSTEM_POPEN_SHPATH;
  errcode = posix_spawn(pid, argv[0], &file_actions,
                        &attr, argv, NULL);
#  else
  *pid = task_spawn("dpopen", nsh_system, &file_actions,
                    &attr, argv + 1, NULL);
  if (*pid < 0)
    {
      errcode = -*pid;
    }
#  endif
#else
  /* No-shell mode: split command and execute directly via posix_spawnp.
   * The command must be in "program arg1 arg2" form -- no shell syntax.
   */

  if (strlcpy(cmdbuf, command, sizeof(cmdbuf)) >= sizeof(cmdbuf))
    {
      errcode = ENAMETOOLONG;
      goto errout_with_actions;
    }

  do
    {
      argv[argc] = strtok_r(argc ? NULL : cmdbuf, " \t", &saveptr);
    }
  while (argv[argc] != NULL && ++argc < DPOPEN_MAX_ARGV - 1);

  argv[argc] = NULL;

  if (argc == 0)
    {
      errcode = EINVAL;
      goto errout_with_actions;
    }

  errcode = posix_spawnp(pid, argv[0], &file_actions,
                         &attr, argv, NULL);
#endif

  if (errcode != 0)
    {
      serr("ERROR: dpopen spawn failed: %d\n", errcode);
      goto errout_with_pipe;
    }

  /* Free attributes and file actions */

  posix_spawn_file_actions_destroy(&file_actions);
  posix_spawnattr_destroy(&attr);

  /* Close the child's end in the parent */

  if (!(oflag & O_CLOEXEC))
    {
      ioctl(parentfd, FIONCLEX, 0);
    }

  close(childfd);

  return parentfd;

errout_with_actions:
  posix_spawn_file_actions_destroy(&file_actions);

errout_with_attr:
  posix_spawnattr_destroy(&attr);

errout_with_pipe:
  close(fd[0]);
  close(fd[1]);
  errno = errcode;
  return -1;
}

/****************************************************************************
 * Name: dpclose
 *
 * Description:
 *   Close a file descriptor opened by dpopen() and wait for the child
 *   process to terminate.
 *
 *   This is the descriptor-based counterpart of pclose().
 *
 * Input Parameters:
 *   fd  - The file descriptor returned by dpopen()
 *   pid - The child process ID returned by dpopen()
 *
 * Returned Value:
 *   The child termination status on success, or -1 on failure with
 *   errno set.
 *
 ****************************************************************************/

int dpclose(int fd, pid_t pid)
{
#ifdef CONFIG_SCHED_WAITPID
  int status;
  int ret;
#endif

  if (fd >= 0)
    {
      close(fd);
    }

#ifdef CONFIG_SCHED_WAITPID
  ret = waitpid(pid, &status, 0);
  if (ret < 0)
    {
      return ERROR;
    }

  return status;
#else
  return 0;
#endif
}
