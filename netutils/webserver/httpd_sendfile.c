/****************************************************************************
 * apps/netutils/webserver/httpd_sendfile.c
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <debug.h>

#include "netutils/httpd.h"

#include "httpd.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int httpd_sendfile_open(const char *name, struct httpd_fs_file *file)
{
  struct stat st;

  if (sizeof(file->path) < snprintf(file->path, sizeof(file->path), "%s%s",
      CONFIG_NETUTILS_HTTPD_PATH, name))
    {
      errno = ENAMETOOLONG;
      return ERROR;
    }

  /* XXX: awaiting fstat to avoid a race */

  if (-1 == stat(file->path, &st))
    {
      return ERROR;
    }

#ifndef CONFIG_NETUTILS_HTTPD_DIRLIST
  if (S_ISDIR(st.st_mode))
    {
      errno = EISDIR;
      return ERROR;
    }

  if (!S_ISREG(st.st_mode))
    {
      errno = ENOENT;
      return ERROR;
    }
#endif

  if (st.st_size > INT_MAX || st.st_size > SIZE_MAX)
    {
      errno = EFBIG;
      return ERROR;
    }

  file->len = (int) st.st_size;

  file->fd = open(file->path, O_RDONLY);

#ifndef CONFIG_NETUTILS_HTTPD_DIRLIST
  if (file->fd == -1)
    {
      return ERROR;
    }
#endif

  return OK;
}

int httpd_sendfile_close(struct httpd_fs_file *file)
{
#ifdef CONFIG_NETUTILS_HTTPD_DIRLIST
  if (-1 == file->fd)
    {
      /* we assume that it's a directory */

      return OK;
    }
#endif

  if (-1 == close(file->fd))
    {
      return ERROR;
    }

  return OK;
}

int httpd_sendfile_send(int outfd, struct httpd_fs_file *file)
{
#ifdef CONFIG_NETUTILS_HTTPD_DIRLIST
  if (-1 == file->fd)
    {
      /* we assume that it's a directory */

      if (-1 == httpd_dirlist(outfd, file))
        {
          return ERROR;
        }

      return OK;
    }
#endif

  if (-1 == sendfile(outfd, file->fd, 0, file->len))
    {
      return ERROR;
    }

  return OK;
}
