/****************************************************************************
 * apps/fsutils/flash_eraseall/flash_eraseall.c
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
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/mtd/mtd.h>
#include "fsutils/flash_eraseall.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: flash_eraseall
 *
 * Description:
 *   Call a block driver with the MDIOC_BULKERASE ioctl command.  This will
 *   cause the MTD driver to erase all of the flash.
 *
 ****************************************************************************/

int flash_eraseall(FAR const char *driver)
{
  int errcode;
  int fd;
  int ret;

  /* Open the block driver */

  fd = open(driver, O_RDONLY);
  if (fd < 0)
    {
      errcode = errno;
      ferr("ERROR: Failed to open '%s': %d\n", driver, errcode);
      ret = -errcode;
    }
  else
    {
      /* Invoke the block driver ioctl method */

      ret = ioctl(fd, MTDIOC_BULKERASE, 0);
      if (ret < 0)
        {
          errcode = errno;
          ferr("ERROR: MTD ioctl(%04x) failed: %d\n", MTDIOC_BULKERASE, errcode);
          ret = -errcode;
        }

      /* Close the block driver */

     close(fd);
    }

  return ret;
}
