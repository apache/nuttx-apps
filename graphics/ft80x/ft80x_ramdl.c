/****************************************************************************
 * apps/graphics/ft80x/ft80x_ramdl.c
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

#include <sys/ioctl.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#include <nuttx/lcd/ft80x.h>

#include "graphics/ft80x.h"
#include "ft80x.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ft80x_ramdl_rewind
 *
 * Description:
 *   Reset to the start of RAM DL memory
 *
 * Input Parameters:
 *   fd - The file descriptor of the FT80x device.  Opened by the caller
 *        with write access.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_ramdl_rewind(int fd)
{
  off_t pos;

  /* Reposition the VFSso that subsequent writes will be to the beginning of
   * the hardware display list.
   */

  pos = lseek(fd, 0, SEEK_SET);
  if (pos < 0)
    {
      int errcode = errno;
      ft80x_err("ERROR: lseek failed: %d\n", errcode);
      return -errcode;
    }

  return OK;
}

/****************************************************************************
 * Name: ft80x_ramdl_append
 *
 * Description:
 *   Append new display list data to RAM DL
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *   data   - A pointer to the start of the data to be written.
 *   len    - The number of bytes to be written.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_ramdl_append(int fd, FAR const void *data, size_t len)
{
  size_t nwritten;

  DEBUGASSERT(data != NULL && ((uintptr_t)data & 3) == 0 &&
              len > 0 && (len & 3) == 0);

  /* Write the aligned data directly to the FT80x hardware display list.
   *
   * NOTE:  We have inside knowledge that the write will complete in a
   * single operation so that no piecewise writes will ever be necessary.
   */

  nwritten = write(fd, data, len);
  if (nwritten < 0)
    {
      int errcode = errno;
      ft80x_err("ERROR: write failed: %d\n", errcode);
      return -errcode;
    }

  DEBUGASSERT(nwritten == len);
  return OK;
}
