/****************************************************************************
 * apps/system/popen/popen.c
 *
 * SPDX-License-Identifier: Apache-2.0
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
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

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
 *   This is a thin wrapper around dpopen() that wraps the returned file
 *   descriptor in a FILE stream, analogous to how fprintf() relates to
 *   dprintf().
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
 * Input Parameters:
 *   command - The command string to execute
 *   mode    - "r" or "w"
 *
 * Returned Value:
 *   A non-NULL FILE stream connected to the child process is returned on
 *   success.  NULL is returned on any failure with the errno variable set
 *   appropriately.
 *
 ****************************************************************************/

FILE *popen(FAR const char *command, FAR const char *mode)
{
  FAR struct popen_file_s *container;
  int oflag;
  int fd;

  /* Allocate a container for returned FILE stream */

  container = (FAR struct popen_file_s *)malloc(sizeof(struct popen_file_s));
  if (container == NULL)
    {
      errno = ENOMEM;
      return NULL;
    }

  /* Map mode string to open flags */

  if (strstr(mode, "r+") || strstr(mode, "w+"))
    {
      oflag = O_RDWR;
    }
  else if (strstr(mode, "r"))
    {
      oflag = O_RDONLY;
    }
  else if (strstr(mode, "w"))
    {
      oflag = O_WRONLY;
    }
  else
    {
      free(container);
      errno = EINVAL;
      return NULL;
    }

  if (strchr(mode, 'e') != NULL)
    {
      oflag |= O_CLOEXEC;
    }

  /* Use dpopen() to do the real work */

  fd = dpopen(command, oflag, &container->shell);
  if (fd < 0)
    {
      free(container);
      return NULL;
    }

  /* Wrap the raw fd in a FILE stream */

  container->original = fdopen(fd, mode);
  if (container->original == NULL)
    {
      int errcode = errno;
      dpclose(fd, container->shell);
      free(container);
      errno = errcode;
      return NULL;
    }

  memcpy(&container->copy, container->original, sizeof(FILE));
  return &container->copy;
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
 * Input Parameters:
 *   stream - The stream reference returned by a previous call to popen()
 *
 * Returned Value:
 *   The child termination status on success, or -1 (ERROR) on failure
 *   with errno set.
 *
 ****************************************************************************/

int pclose(FILE *stream)
{
  FAR struct popen_file_s *container = (FAR struct popen_file_s *)stream;
  FILE *original;
  pid_t shell;

  original = container->original;

  /* Set the state of the original file descriptor to the state of the
   * working copy
   */

  memcpy(original, &container->copy, sizeof(FILE));

  /* Then close the original and free the container (saving the PID of the
   * shell process).  Pass -1 to dpclose since fclose already closed the fd.
   */

  fclose(original);

  shell = container->shell;
  free(container);

  return dpclose(-1, shell);
}
