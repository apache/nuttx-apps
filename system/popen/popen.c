/****************************************************************************
 * apps/popen/popen/popen.c
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>
#include <spawn.h>
#include <assert.h>
#include <debug.h>

#include "nshlib/nshlib.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* struct popen_file_s is a cast compatible version of FILE that contains
 * the additional PID of the shell processes needed by pclose().
 */

struct popen_file_s
{
  FILE copy;
  FILE *original;
  pid_t shell;
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: popen
 *
 * Description:
 *   The popen() function will execute the command specified by the string
 *   command. It will create a pipe between the calling program and the
 *   executed command, and will return a pointer to a stream that can be
 *   used to either read from or write to the pipe.
 *
 *   The environment of the executed command will be as if a child process
 *   were created within the popen() call using the fork() function, and the
 *   child invoked the sh utility using the call:
 *
 *     execl(shell path, "sh", "-c", command, (char *)0);
 *
 *   where shell path is an unspecified pathname for the sh utility.
 *
 *   The popen() function will ensure that any streams from previous popen()
 *   calls that remain open in the parent process are closed in the new child
 *   process.
 *
 *   The mode argument to popen() is a string that specifies I/O mode:
 *
 *     - If mode is r, when the child process is started, its file descriptor
 *       STDOUT_FILENO will be the writable end of the pipe, and the file
 *       descriptor fileno(stream) in the calling process, where stream is
 *       the stream pointer returned by popen(), will be the readable end of
 *       the pipe.
 *
 *     - If mode is w, when the child process is started its file descriptor
 *       STDIN_FILENO will be the readable end of the pipe, and the file
 *       descriptor fileno(stream) in the calling process, where stream is
 *       the stream pointer returned by popen(), will be the writable end of
 *       the pipe.
 *
 *   If mode is any other value, the result is undefined.
 *
 *   After popen(), both the parent and the child process will be capable of
 *   executing independently before either terminates.
 *
 *   Pipe streams are byte-oriented.
 *
 * Input Parameters:
 *   command
 *
 * Returned Value:
 *   A non-NULLFILE stream connected to the shell instance is returned on
 *   success.  NULL is returned on any failure with the errno variable set
 *   appropriately.
 *
 ****************************************************************************/

FILE *popen(FAR const char *command, FAR const char *mode)
{
  FAR struct popen_file_s *container;
  struct sched_param param;
  posix_spawnattr_t attr;
  posix_spawn_file_actions_t file_actions;
  FAR char *argv[3];
  int fd[2];
  int oldfd;
  int newfd;
  int retfd;
  int errcode;
  int result;

  /* Allocate a container for returned FILE stream */

  container = (FAR struct popen_file_s *)malloc(sizeof(struct popen_file_s));
  if (container == NULL)
    {
      errcode = ENOMEM;
      goto errout;
    }

  /* Create a pipe.  fd[0] refers to the read end of the pipe; fd[1] refers
   * to the write end of the pipe.
   */

  result = pipe(fd);
  if (result < 0)
    {
      errcode = errno;
      goto errout_with_container;
    }

  /* Is the pipe the input to the shell?  Or the output? */

  if (strcmp(mode, "r") == 0)
    {
      /* Pipe is the output from the shell */

      oldfd = 1;     /* Replace stdout with the write side of the pipe */
      newfd = fd[1];
      retfd = fd[0]; /* Use read side of the pipe to create the return stream */
    }
  else if (strcmp(mode, "w") == 0)
    {
      /* Pipe is the input to the shell */

      oldfd = 0;     /* Replace stdin with the read side of the pipe */
      newfd = fd[0];
      retfd = fd[1]; /* Use write side of the pipe to create the return stream */
    }
  else
    {
      errcode = EINVAL;
      goto errout_with_pipe;
    }

  /* Create the FILE stream return reference */

  container->original = fdopen(retfd, mode);
  if (container->original == NULL)
    {
      errcode = errno;
      goto errout_with_pipe;
    }

  /* Initialize attributes for task_spawn() (or posix_spawn()). */

  errcode = posix_spawnattr_init(&attr);
  if (errcode != 0)
    {
      goto errout_with_stream;
    }

  errcode = posix_spawn_file_actions_init(&file_actions);
  if (errcode != 0)
    {
      goto errout_with_attrs;
    }

  /* Set the correct stack size and priority */

  param.sched_priority = CONFIG_SYSTEM_POPEN_PRIORITY;
  errcode = posix_spawnattr_setschedparam(&attr, &param);
  if (errcode != 0)
    {
      goto errout_with_actions;
    }

  errcode = task_spawnattr_setstacksize(&attr,
                                        CONFIG_SYSTEM_POPEN_STACKSIZE);
  if (errcode != 0)
    {
      goto errout_with_actions;
    }

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

  /* Redirect input or output as determined by the mode parameter */

  errcode = posix_spawn_file_actions_adddup2(&file_actions, newfd, oldfd);
  if (errcode != 0)
    {
      goto errout_with_actions;
    }

  /* Call task_spawn() (or posix_spawn), re-directing stdin or stdout
   * appropriately.
   */

  argv[0] = "-c";
  argv[1] = (FAR char *)command;
  argv[2] = NULL;

#ifdef CONFIG_SYSTEM_POPEN_SHPATH
  errcode = posix_spawn(&container->shell, CONFIG_SYSTEM_POPEN_SHPATH,
                        &file_actions, &attr, argv,
                        (FAR char * const *)NULL);
#else
  errcode = task_spawn(&container->shell, "popen", nsh_system, &file_actions,
                       &attr, argv, (FAR char * const *)NULL);
#endif

  if (errcode != 0)
    {
      serr("ERROR: Spawn failed: %d\n", result);
      goto errout_with_actions;
    }

  /* We can close the 'newfd' now.  It is no longer useful on this side of
   * the interface.
   */

  close(newfd);

  /* Free attributes and file actions.  Ignoring return values in the case
   * of an error.
   */

  posix_spawn_file_actions_destroy(&file_actions);
  posix_spawnattr_destroy(&attr);

  /* Finale and return input input/output stream */

  memcpy(&container->copy, container->original, sizeof(FILE));
  return &container->copy;

errout_with_actions:
  posix_spawn_file_actions_destroy(&file_actions);

errout_with_attrs:
  posix_spawnattr_destroy(&attr);

errout_with_stream:
  fclose(container->original);

errout_with_pipe:
  close(fd[0]);
  close(fd[1]);

errout_with_container:
  free(container);

errout:
  errno = errcode;
  return NULL;
}

/****************************************************************************
 * Name: pclose
 *
 * Description:
 *   The pclose() function will close a stream that was opened by popen(),
 *   wait for the command to terminate, and return the termination status of
 *   the process that was running the command language interpreter. However,
 *   if a call caused the termination status to be unavailable to pclose(),
 *   then pclose() will return -1 with errno set to ECHILD to report this
 *   situation. This can happen if the application calls one of the following
 *   functions:
 *
 *     wait()
 *     waitpid() with a pid argument less than or equal to 0 or equal to the
 *               process ID of the command line interpreter
 *
 *   Any other function not defined in this volume of IEEE Std 1003.1-2001
 *   that could do one of the above
 *
 *   In any case, pclose() will not return before the child process created
 *   by popen() has terminated.
 *
 *   If the command language interpreter cannot be executed, the child
 *   termination status returned by pclose() will be as if the command
 *   language interpreter terminated using exit(127) or _exit(127).
 *
 *   The pclose() function will not affect the termination status of any
 *   child of the calling process other than the one created by popen() for
 *   the associated stream.
 *
 *   If the argument stream to pclose() is not a pointer to a stream created
 *   by popen(), the result of pclose() is undefined.
 *
 * Description:
 *   stream - The stream reference returned by a previous call to popen()
 *
 * Returned Value:
 *   Zero (OK) is returned on success; otherwise -1 (ERROR) is returned and
 *   the errno variable is set appropriately.
 *
 ****************************************************************************/

int pclose(FILE *stream)
{
  FAR struct popen_file_s *container = (FAR struct popen_file_s *)stream;
  FILE *original;
  pid_t shell;
#ifdef CONFIG_SCHED_WAITPID
  int status;
  int result;
#endif

  DEBUGASSERT(container != NULL && container->original != NULL);
  original = container->original;

  /* Set the state of the original file descriptor to the state of the
   * working copy
   */

  memcpy(original, &container->copy, sizeof(FILE));

  /* Then close the original and free the container (saving the PID of the
   * shell process)
   */

  fclose(original);

  shell = container->shell;
  free(container);

#ifdef CONFIG_SCHED_WAITPID
  /* Wait for the shell to exit, retrieving the return value if available. */

  result = waitpid(shell, &status, 0);
  if (result < 0)
    {
      /* The errno has already been set */

      return ERROR;
    }

  return status;
#else
  return OK;
#endif
}
