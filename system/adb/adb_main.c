/****************************************************************************
 * apps/system/adb/adb_main.c
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

#include "adb.h"

#include <stdio.h>
#include <syslog.h>

#if defined(CONFIG_ADBD_BOARD_INIT) || defined (CONFIG_BOARDCTL_RESET)
#  include <sys/boardctl.h>
#endif

#ifdef CONFIG_ADBD_NET_INIT
#include "netutils/netinit.h"
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void adb_log_impl(FAR const char *func, int line, FAR const char *fmt, ...)
{
  struct va_format vaf;
  va_list ap;

  va_start(ap, fmt);
  vaf.fmt = fmt;
  vaf.va  = &ap;
  syslog(LOG_ERR, "%s (%d): %pV", func, line, &vaf);
  va_end(ap);
}

void adb_reboot_impl(const char *target)
{
#ifdef CONFIG_BOARDCTL_RESET
  if (strcmp(target, "recovery") == 0)
    {
      boardctl(BOARDIOC_RESET, CONFIG_ADBD_RESET_RECOVERY);
    }
  else if (strcmp(target, "bootloader") == 0)
    {
      boardctl(BOARDIOC_RESET, CONFIG_ADBD_RESET_BOOTLOADER);
    }
  else
    {
      boardctl(BOARDIOC_RESET, 0);
    }
#else
  adb_log("reboot not implemented\n");
#endif
}

int main(int argc, FAR char **argv)
{
  adb_context_t *ctx;

#ifdef CONFIG_ADBD_BOARD_INIT
  boardctl(BOARDIOC_INIT, 0);

#if defined(CONFIG_ADBD_USB_SERVER) && \
    defined(CONFIG_USBDEV_COMPOSITE) && \
    defined (CONFIG_BOARDCTL_USBDEVCTRL)

  /* Setup composite USB device */

  struct boardioc_usbdev_ctrl_s ctrl;
  int ret;
  FAR void *handle;

  /* Perform architecture-specific initialization */

  ctrl.usbdev   = BOARDIOC_USBDEV_COMPOSITE;
  ctrl.action   = BOARDIOC_USBDEV_INITIALIZE;
  ctrl.instance = 0;
  ctrl.config   = 0;
  ctrl.handle   = NULL;

  ret = boardctl(BOARDIOC_USBDEV_CONTROL, (uintptr_t)&ctrl);
  if (ret < 0)
    {
      printf("boardctl(BOARDIOC_USBDEV_CONTROL) failed: %d\n", ret);
      return 1;
    }

  /* Initialize the USB composite device device */

  ctrl.usbdev   = BOARDIOC_USBDEV_COMPOSITE;
  ctrl.action   = BOARDIOC_USBDEV_CONNECT;
  ctrl.instance = 0;
  ctrl.config   = 0;
  ctrl.handle   = &handle;

  ret = boardctl(BOARDIOC_USBDEV_CONTROL, (uintptr_t)&ctrl);
  if (ret < 0)
    {
      printf("boardctl(BOARDIOC_USBDEV_CONTROL) failed: %d\n", ret);
      return 1;
    }
#endif /* ADBD_USB_SERVER && USBDEV_COMPOSITE && BOARDCTL_USBDEVCTRL */
#endif /* CONFIG_ADBD_BOARD_INIT */

#ifdef CONFIG_ADBD_NET_INIT
  /* Bring up the network */

  netinit_bringup();
#endif

  ctx = adb_hal_create_context();
  if (!ctx)
    {
      return -1;
    }

  adb_hal_run(ctx);
  adb_hal_destroy_context(ctx);
  return 0;
}
