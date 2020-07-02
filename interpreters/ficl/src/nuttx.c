/****************************************************************************
 * apps/interpreters/minibasic/basic.c
 *
 *   Copyright (C) 2016 Gregory Nutt. All rights reserved.
 *
 * This file was taken from Mini Basic, versino 1.0 developed by Malcolm
 * McLean, Leeds University.  Mini Basic version 1.0 was released the
 * Creative Commons Attribution license which, from my reading, appears to
 * be compatible with the NuttX BSD-style license:
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
  return;
}
