/****************************************************************************
 * system/cdcacm/cdcacm_main.c
 *
 *   Copyright (C) 2012, 2016 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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

#ifdef CONFIG_BUILD_LOADABLE
int main(int argc, FAR char *argv[])
#else
int sercon_main(int argc, char *argv[])
#endif
{
  struct boardioc_usbdev_ctrl_s ctrl;
  int ret;

  /* Check if there is a non-NULL USB mass storage device handle (meaning that the
   * USB mass storage device is already configured).
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
      printf("sercon: ERROR: Failed to create the CDC/ACM serial device: %d\n", -ret);
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

#ifdef CONFIG_BUILD_LOADABLE
int main(int argc, FAR char **argv)
#else
int serdis_main(int argc, char *argv[])
#endif
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

  /* Then disconnect the device and uninitialize the USB mass storage driver */

  ctrl.usbdev   = BOARDIOC_USBDEV_CDCACM;
  ctrl.action   = BOARDIOC_USBDEV_DISCONNECT;
  ctrl.instance = CONFIG_SYSTEM_CDCACM_DEVMINOR;
  ctrl.handle   = &g_cdcacm.handle;

  (void)boardctl(BOARDIOC_USBDEV_CONTROL, (uintptr_t)&ctrl);
  g_cdcacm.handle = NULL;
  printf("serdis: Disconnected\n");
  return EXIT_SUCCESS;
}
