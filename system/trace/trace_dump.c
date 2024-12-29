/****************************************************************************
 * apps/system/trace/trace_dump.c
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
#include <nuttx/note/noteram_driver.h>

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "trace.h"

/****************************************************************************
 * Name: note_ioctl
 ****************************************************************************/

static void note_ioctl(int cmd, unsigned long arg)
{
  int notefd;

  notefd = open("/dev/note/ram", O_RDONLY);
  if (notefd < 0)
    {
      fprintf(stderr, "trace: cannot open /dev/note/ram\n");
      return;
    }

  ioctl(notefd, cmd, arg);
  close(notefd);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trace_dump
 *
 * Description:
 *   Read notes and dump trace results.
 *
 ****************************************************************************/

int trace_dump(FAR FILE *out)
{
  uint8_t tracedata[1024];
  int ret;
  int fd;

  /* Open note for read */

  fd = open("/dev/note/ram", O_RDONLY);
  if (fd < 0)
    {
      fprintf(stderr, "trace: cannot open /dev/note/ram\n");
      return ERROR;
    }

  /* Read and output all notes */

  while (1)
    {
      ret = read(fd, tracedata, sizeof tracedata);
      if (ret < 0 || ret > sizeof(tracedata))
        {
          fprintf(stderr, "trace: read error: %d, errno:%d\n", ret, errno);
          continue;
        }
      else if (ret == 0)
        {
          break;
        }

      fwrite(tracedata, 1, ret, out);
    }

  /* Close note */

  close(fd);

  return ret;
}

/****************************************************************************
 * Name: trace_dump_clear
 *
 * Description:
 *   Clear all contents of the buffer
 *
 ****************************************************************************/

void trace_dump_clear(void)
{
  note_ioctl(NOTERAM_CLEAR, 0);
}

/****************************************************************************
 * Name: trace_dump_get_overwrite
 *
 * Description:
 *   Get overwrite mode
 *
 ****************************************************************************/

bool trace_dump_get_overwrite(void)
{
  unsigned int mode = 0;

  note_ioctl(NOTERAM_GETMODE, (unsigned long)&mode);

  return mode == NOTERAM_MODE_OVERWRITE_ENABLE;
}

/****************************************************************************
 * Name: trace_dump_set_overwrite
 *
 * Description:
 *   Set overwrite mode
 *
 ****************************************************************************/

void trace_dump_set_overwrite(bool enable)
{
  unsigned int mode;

  mode = enable ? NOTERAM_MODE_OVERWRITE_ENABLE :
                  NOTERAM_MODE_OVERWRITE_DISABLE;

  note_ioctl(NOTERAM_SETMODE, (unsigned long)&mode);
}
