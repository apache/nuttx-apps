/****************************************************************************
 * apps/system/i2c/i2c_devif.c
 *
 *   Copyright (C) 2019 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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
