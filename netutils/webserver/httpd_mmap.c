/****************************************************************************
 * apps/netutils/webserver/httpd_mmap.c
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

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <debug.h>

#include "netutils/httpd.h"

#include "httpd.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int httpd_mmap_open(const char *name, struct httpd_fs_file *file)
{
  char path[PATH_MAX];
  struct stat st;

  if (sizeof path < snprintf(path, sizeof path, "%s%s",
      CONFIG_NETUTILS_HTTPD_PATH, name))
    {
      errno = ENAMETOOLONG;
      return ERROR;
    }

  /* XXX: awaiting fstat to avoid a race */

  if (-1 == stat(path, &st))
    {
      return ERROR;
    }

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

  if (st.st_size > INT_MAX)
    {
      errno = EFBIG;
      return ERROR;
    }

  file->len = (int) st.st_size;

  /* SUS3: "If len is zero, mmap() shall fail and no mapping shall
   * be established."
   */

  if (st.st_size == 0)
    {
      return OK;
    }

  file->fd = open(path, O_RDONLY);
  if (file->fd == -1)
    {
      return ERROR;
    }

  file->data = mmap(NULL, st.st_size, PROT_READ,
                    MAP_SHARED | MAP_FILE, file->fd, 0);
  if (file->data == MAP_FAILED)
    {
      close(file->fd);
      return ERROR;
    }

  return OK;
}

int httpd_mmap_close(struct httpd_fs_file *file)
{
  if (file->len == 0)
    {
      return OK;
    }

#ifdef CONFIG_FS_RAMMAP
  if (-1 == munmap(file->data, file->len))
    {
      return ERROR;
    }
#endif

  if (-1 == close(file->fd))
    {
      return ERROR;
    }

  return OK;
}
