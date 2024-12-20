/****************************************************************************
 * apps/interpreters/ficl/src/nuttx.c
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
 ***************************************************************************/

/****************************************************************************
 *
 * This file was taken from Mini Basic, versino 1.0 developed by Malcolm
 * McLean, Leeds University.  Mini Basic version 1.0 was released the
 * Creative Commons Attribution license which, from my reading, appears to
 * be compatible with the NuttX license.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "ficl.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void *ficlMalloc(size_t size)
{
  return malloc(size);
}

void *ficlRealloc(void *p, size_t size)
{
  return realloc(p, size);
}

void ficlFree(void *p)
{
  free(p);
}

void ficlCallbackDefaultTextOut(ficlCallback *callback, char *message)
{
  FICL_IGNORE(callback);
  if (message != NULL)
    {
      fputs(message, stdout);
    }
  else
    {
      fflush(stdout);
    }
}

int ficlFileStatus(char *filename, int *status)
{
  struct stat statbuf;
  if (stat(filename, &statbuf) == 0)
    {
      *status = statbuf.st_mode;
      return 0;
    }

  *status = ENOENT;
  return -1;
}

long ficlFileSize(ficlFile *ff)
{
  struct stat statbuf;
  if (ff == NULL)
    {
      return -1;
    }

  statbuf.st_size = -1;
  if (fstat(fileno(ff->f), &statbuf) != 0)
    {
      return -1;
    }

  return statbuf.st_size;
}

void ficlSystemCompilePlatform(ficlSystem *system)
{
}
