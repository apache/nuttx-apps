/****************************************************************************
 * apps/testing/nest/nest_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include <stdio.h>

#include <testing/unity.h>

#include "nest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: setUp
 *
 * Description:
 *   Unity required set up function. Not used.
 ****************************************************************************/

void setUp(void)
{
  /* Intentionally empty */

  return;
}

/****************************************************************************
 * Name: tearDown
 *
 * Description:
 *   Unity required tear down function. Not used.
 ****************************************************************************/

void tearDown(void)
{
  /* Intentionally empty */

  return;
}

/****************************************************************************
 * nest_main
 ****************************************************************************/

int main(int argc, FAR char **argv)
{
  UNUSED(argc);
  UNUSED(argv);

  UNITY_BEGIN();

  /* Collections */

  nest_collections_cbuf();
  nest_collections_hmap();
  nest_collections_list();

  /* Devices */

  nest_devices_devascii();
  nest_devices_devconsole();
  nest_devices_devnull();
  nest_devices_devurandom();
  nest_devices_devzero();

  return UNITY_END();
}
