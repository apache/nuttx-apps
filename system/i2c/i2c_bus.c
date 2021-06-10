/****************************************************************************
 * apps/system/i2c/i2c_bus.c
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

#include <nuttx/i2c/i2c_master.h>

#include "i2ctool.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: i2ccmd_bus
 ****************************************************************************/

int i2ccmd_bus(FAR struct i2ctool_s *i2ctool, int argc, char **argv)
{
  int bus;

  i2ctool_printf(i2ctool, " BUS   EXISTS?\n");
  for (bus = CONFIG_I2CTOOL_MINBUS; bus <= CONFIG_I2CTOOL_MAXBUS; bus++)
    {
      if (i2cdev_exists(bus))
        {
          i2ctool_printf(i2ctool, "Bus %d: YES\n", bus);
        }
      else
        {
          i2ctool_printf(i2ctool, "Bus %d: NO\n", bus);
        }
    }

  return OK;
}
