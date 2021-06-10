/****************************************************************************
 * apps/system/cdcacm/cdcacm_main.c
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
 * Private Data
 ****************************************************************************/

/* All global variables used by this add-on are packed into a structure in
 * order to avoid name collisions.
 */

static struct cdcacm_state_s g_cdcacm;

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

  /* Check if there is a non-NULL USB mass storage device handle
   * (meaning that the USB mass storage device is already configured).
   */

  if (g_cdcacm.handle)
    {
      printf("sercon:: ERROR: Already connected\n");
      return EXIT_FAILURE;
    }

  /* Then, in any event, enable trace data collection as configured BEFORE
   * enabling the CDC/ACM device.
   */

  usbtrace_enable(TRACE_BITSET);

  /* Initialize the USB CDC/ACM serial driver */

  printf("sercon: Registering CDC/ACM serial driver\n");

  ctrl.usbdev   = BOARDIOC_USBDEV_CDCACM;
  ctrl.action   = BOARDIOC_USBDEV_CONNECT;
  ctrl.instance = CONFIG_SYSTEM_CDCACM_DEVMINOR;
  ctrl.handle   = &g_cdcacm.handle;

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

/****************************************************************************
 * serdis_main
 *
 * Description:
 *   This is a program entry point that will disconnect the CDC/ACM serial
 *   device.
 ****************************************************************************/

int serdis_main(int argc, char *argv[])
{
  struct boardioc_usbdev_ctrl_s ctrl;

  /* First check if the USB mass storage device is already connected */

  if (!g_cdcacm.handle)
    {
      printf("serdis: ERROR: Not connected\n");
      return EXIT_FAILURE;
    }

  /* Then, in any event, disable trace data collection as configured BEFORE
   * enabling the CDC/ACM device.
   */

  usbtrace_enable(0);

  /* Then disconnect the device and uninitialize the USB mass storage
   * driver
   */

  ctrl.usbdev   = BOARDIOC_USBDEV_CDCACM;
  ctrl.action   = BOARDIOC_USBDEV_DISCONNECT;
  ctrl.instance = CONFIG_SYSTEM_CDCACM_DEVMINOR;
  ctrl.handle   = &g_cdcacm.handle;

  boardctl(BOARDIOC_USBDEV_CONTROL, (uintptr_t)&ctrl);
  g_cdcacm.handle = NULL;
  printf("serdis: Disconnected\n");
  return EXIT_SUCCESS;
}
