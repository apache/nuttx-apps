/****************************************************************************
 * apps/system/spi/spi_devif.c
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

#include <nuttx/spi/spi_transfer.h>

#include "spitool.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Device naming */

#define DEVNAME_FMT    "/dev/spi%d"
#define DEVNAME_FMTLEN (8 + 3 + 1)

/****************************************************************************
 * Private Data
 ****************************************************************************/

static char g_devname[DEVNAME_FMTLEN];

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: spidev_path
 ****************************************************************************/

FAR char *spidev_path(int bus)
{
  snprintf(g_devname, DEVNAME_FMTLEN, DEVNAME_FMT, bus);
  return g_devname;
}

/****************************************************************************
 * Name: spidev_exists
 ****************************************************************************/

bool spidev_exists(int bus)
{
  struct stat buf;
  FAR char *devpath;
  int ret;

  /* Get the device path */

  devpath = spidev_path(bus);

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
 * Name: spidev_open
 ****************************************************************************/

int spidev_open(int bus)
{
  FAR char *devpath;

  /* Get the device path */

  devpath = spidev_path(bus);

  /* Open the file for read-only access (we need only IOCTLs) */

  return open(devpath, O_RDONLY);
}

/****************************************************************************
 * Name: spidev_transfer
 ****************************************************************************/

int spidev_transfer(int fd, FAR struct spi_sequence_s *seq)
{
  /* Perform the IOCTL */

  return ioctl(fd, SPIIOC_TRANSFER, (unsigned long)((uintptr_t)seq));
}
