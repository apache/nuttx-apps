/****************************************************************************
 * apps/examples/ble/dummy.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to you under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "sensors.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Returns a dummy temperature value */

float get_temperature(void)
{
  return 40;
}

/* Returns a dummy pressure value */

float get_pressure(void)
{
  return 1000;
}

/* Returns a dummy gas resistance value */

float get_gas_resistance(void)
{
  return 100;
}

/* Returns a dummy humidity value */

float get_humidity(void)
{
  return 30;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Monitors sensors indefinitely (dummy implementation) */

void *monitor_sensors(void *arg)
{
  (void)arg;
  while (1)
    {
      /* Infinite loop as a placeholder */
    }

  return NULL;
}

/* Initializes sensors (dummy implementation) */

int init_sensors(void)
{
  return 0;
}

/* Closes sensor file descriptors (dummy implementation) */

void close_sensors_fd(void)
{
  return;
}
