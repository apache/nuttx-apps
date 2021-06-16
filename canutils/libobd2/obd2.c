/****************************************************************************
 * apps/canutils/libobd2/obd2.c
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
