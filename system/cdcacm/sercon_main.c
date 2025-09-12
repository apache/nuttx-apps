/****************************************************************************
 * apps/system/cdcacm/sercon_main.c
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

#include <sys/types.h>
#include <sys/boardctl.h>

#include <stdio.h>
#include <debug.h>

#include <nuttx/usb/usbdev.h>
#include <nuttx/usb/cdcacm.h>

#include "cdcacm.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * sercon_main
 *
 * Description:
 *   This is the main program that configures the CDC/ACM serial device.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct boardioc_usbdev_ctrl_s ctrl;
  int ret;

  /* Then, in any event, enable trace data collection as configured BEFORE
   * enabling the CDC/ACM device.
   */

  usbtrace_enable(TRACE_BITSET);

  /* Initialize the USB CDC/ACM serial driver */

  printf("sercon: Registering CDC/ACM serial driver\n");

  ctrl.usbdev   = BOARDIOC_USBDEV_CDCACM;
  ctrl.action   = BOARDIOC_USBDEV_CONNECT;
  ctrl.instance = CONFIG_SYSTEM_CDCACM_DEVMINOR;
  ctrl.handle   = NULL;

  ret = boardctl(BOARDIOC_USBDEV_CONTROL, (uintptr_t)&ctrl);
  if (ret < 0)
    {
      printf("sercon: ERROR: "
             "Failed to create the CDC/ACM serial device: %d\n", -ret);
      return EXIT_FAILURE;
    }

  printf("sercon: Successfully registered the CDC/ACM serial driver\n");
  return EXIT_SUCCESS;
}
