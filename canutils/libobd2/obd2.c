/****************************************************************************
 * canutils/libobd2/obd2.c
 *
 *   Copyright (C) 2017 Alan Carvalho de Assis. All rights reserved.
 *   Author: Alan Carvalho de Assis <acassis@gmail.com>
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

#include <sys/ioctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <nuttx/can/can.h>

#include "canutils/obd.h"
#include "canutils/obd_pid.h"
#include "canutils/obd_frame.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: obd_init
 *
 * Description:
 *   Initialize the OBD-II with initial baudrate
 *
 *   Returns a obd_dev_s with initial values or NULL if error.
 *
 ****************************************************************************/

struct obd_dev_s *obd_init(char *devfile, int baudate, int mode)
{
  struct obd_dev_s *dev;
  int ret;

  /* Alloc memory for this device */

  dev = malloc(sizeof(struct obd_dev_s));
  if (!dev)
    {
      printf("ERROR: Failed to alloc memory for obd_dev!\n");
      return NULL;
    }

  /* Open the CAN device for reading/writing */

  dev->can_fd = open(devfile, O_RDWR);
  if (dev->can_fd < 0)
    {
      printf("ERROR: open %s failed: %d\n", devfile, errno);
      return NULL;
    }

  /* Show bit timing information if provided by the driver.  Not all CAN
   * drivers will support this IOCTL.
   */

  ret = ioctl(dev->can_fd, CANIOC_GET_BITTIMING,
              (unsigned long)((uintptr_t)&dev->can_bt));
  if (ret < 0)
    {
      printf("Bit timing not available: %d\n", errno);
      return NULL;
    }
  else
    {
      printf("Bit timing:\n");
      printf("   Baud: %lu\n", (unsigned long)dev->can_bt.bt_baud);
      printf("  TSEG1: %u\n", dev->can_bt.bt_tseg1);
      printf("  TSEG2: %u\n", dev->can_bt.bt_tseg2);
      printf("    SJW: %u\n", dev->can_bt.bt_sjw);
    }

  /* FIXME: Setup the baudrate */

  /* Setup the initial mode */

  if (mode != CAN_STD && mode != CAN_EXT)
    {
      printf("ERROR: Invalid mode, it needs to be CAN_STD or CAN_EXT!\n");
      return NULL;
    }

  dev->can_mode = mode;

  printf("OBD-II device initialized!\n");

  return dev;
}
