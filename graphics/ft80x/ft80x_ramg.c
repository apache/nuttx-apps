/****************************************************************************
 * apps/graphics/ft80x/ft80x_ramg.c
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
#include <assert.h>
#include <errno.h>

#include <nuttx/lcd/ft80x.h>

#include "graphics/ft80x.h"
#include "ft80x.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ft80x_ramg_write
 *
 * Description:
 *   Write to graphics memory
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *   offset - Offset in graphics memory to write to (dest)
 *   data   - Pointer to a data to be written (src)
 *   nbytes - The number of bytes to write to graphics memory.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_ramg_write(int fd, unsigned int offset, FAR const void *data,
                     unsigned int nbytes)
{
  struct ft80x_relmem_s ramg;
  int ret;

  DEBUGASSERT(data != NULL && nbytes > 0 &&
              (offset + nbytes) < FT80X_RAM_G_SIZE);

  /* Perform the IOCTL to write data to graphics memory */

  ramg.offset = offset;
  ramg.nbytes = nbytes;
  ramg.value  = (FAR void *)data;  /* Need to discard const qualifier */

  ret = ioctl(fd, FT80X_IOC_PUTRAMG, (unsigned long)((uintptr_t)&ramg));
  if (ret < 0)
    {
      int errcode = errno;
      ft80x_err("ERROR:  ioctl(FT80X_IOC_PUTRAMG) failed: %d\n", errcode);
      return -errcode;
    }

  return OK;
}
