/****************************************************************************
 * apps/system/spi/spi_bus.c
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

#include <nuttx/spi/spi_transfer.h>

#include "spitool.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: spicmd_bus
 ****************************************************************************/

int spicmd_bus(FAR struct spitool_s *spitool, int argc, char **argv)
{
  int bus;

  spitool_printf(spitool, " BUS   EXISTS?\n");
  for (bus = CONFIG_SPITOOL_MINBUS; bus <= CONFIG_SPITOOL_MAXBUS; bus++)
    {
      if (spidev_exists(bus))
        {
          spitool_printf(spitool, "Bus %d: YES\n", bus);
        }
      else
        {
          spitool_printf(spitool, "Bus %d: NO\n", bus);
        }
    }

  return OK;
}
