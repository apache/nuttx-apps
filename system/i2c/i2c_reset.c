/****************************************************************************
 * apps/system/i2c/i2c_reset.c
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

#include <unistd.h>

#include <nuttx/i2c/i2c_master.h>

#include "i2ctool.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: i2ccmd_reset
 ****************************************************************************/

int i2ccmd_reset(FAR struct i2ctool_s *i2ctool, int argc, char **argv)
{
  int ret;
  int fd;

  /* Get a handle to the I2C bus */

  fd = i2cdev_open(i2ctool->bus);
  if (fd < 0)
    {
      i2ctool_printf(i2ctool, "Failed to get bus %d\n", i2ctool->bus);
      return ERROR;
    }

  ret = i2cdev_reset(fd);
  if (ret == OK)
    {
      i2ctool_printf(i2ctool, "Reset command sent successfully\n");
    }
  else
    {
      i2ctool_printf(i2ctool, "Failed to send the reset command\n");
    }

  ret = close(fd);
  if (ret < 0)
    {
      i2ctool_printf(i2ctool, "Failed to close i2c device\n");
    }

  return ret;
}

