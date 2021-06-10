/****************************************************************************
 * apps/system/i2c/i2c_devif.c
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

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>

#include <nuttx/i2c/i2c_master.h>

#include "i2ctool.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Device naming */

#define DEVNAME_FMT    "/dev/i2c%d"
#define DEVNAME_FMTLEN (8 + 3 + 1)

/****************************************************************************
 * Private Data
 ****************************************************************************/

static char g_devname[DEVNAME_FMTLEN];

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: i2cdev_path
 ****************************************************************************/

FAR char *i2cdev_path(int bus)
{
  snprintf(g_devname, DEVNAME_FMTLEN, DEVNAME_FMT, bus);
  return g_devname;
}

/****************************************************************************
 * Name: i2cdev_exists
 ****************************************************************************/

bool i2cdev_exists(int bus)
{
  struct stat buf;
  FAR char *devpath;
  int ret;

  /* Get the device path */

  devpath = i2cdev_path(bus);

  /* Check if something exists at that path */

  ret = stat(devpath, &buf);
  if (ret >= 0)
    {
      /* Return TRUE only if it is a character driver */

      if (S_ISCHR(buf.st_mode))
        {
          return true;
        }
    }

  return false;
}

/****************************************************************************
 * Name: i2cdev_open
 ****************************************************************************/

int i2cdev_open(int bus)
{
  FAR char *devpath;

  /* Get the device path */

  devpath = i2cdev_path(bus);

  /* Open the file for read-only access (we need only IOCTLs) */

  return open(devpath, O_RDONLY);
}

/****************************************************************************
 * Name: i2cdev_transfer
 ****************************************************************************/

int i2cdev_transfer(int fd, FAR struct i2c_msg_s *msgv, int msgc)
{
  struct i2c_transfer_s xfer;

  /* Set up the IOCTL argument */

  xfer.msgv = msgv;
  xfer.msgc = msgc;

  /* Perform the IOCTL */

  return ioctl(fd, I2CIOC_TRANSFER, (unsigned long)((uintptr_t)&xfer));
}

/****************************************************************************
 * Name: i2cdev_reset
 ****************************************************************************/

#ifdef CONFIG_I2C_RESET
int i2cdev_reset(int fd)
{
  /* Perform the IOCTL */

  return ioctl(fd, I2CIOC_RESET, 0);
}
#endif

