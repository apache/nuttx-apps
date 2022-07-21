/****************************************************************************
 * apps/games/shift/shift_input_gesture.h
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

#include <nuttx/sensors/apds9960.h>

#include "shift_inputs.h"

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

#define APDS9960_DEVNAME "/dev/gest0"

/****************************************************************************
 * dev_input_init
 ****************************************************************************/

int dev_input_init(FAR struct input_state_s *dev)
{
  /* Open the gesture sensor APDS9960 */

  dev->fd_gest = open(APDS9960_DEVNAME, O_RDONLY | O_NONBLOCK);
  if (dev->fd_gest < 0)
    {
      int errcode = errno;
      printf("ERROR: Failed to open %s: %d\n", APDS9960_DEVNAME, errcode);
      return -ENODEV;
    }

  return OK;
}

/****************************************************************************
 * dev_read_input
 ****************************************************************************/

int dev_read_input(FAR struct input_state_s *dev)
{
  int nbytes;
  uint8_t gest;

  nbytes = read(dev->fd_gest, (void *)&gest, sizeof(gest));
  if (nbytes == sizeof(gest))
    {
      dev->dir = gest;
    }
  else
    {
      dev->dir = DIR_NONE;
      return -EINVAL;
    }

  return OK;
}

