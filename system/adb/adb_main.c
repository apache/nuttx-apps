/****************************************************************************
 * apps/system/adb/adb_main.c
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

#include "adb.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <nuttx/streams.h>

#if defined(CONFIG_ADBD_BOARD_INIT) || defined (CONFIG_BOARDCTL_RESET) || \
    defined(CONFIG_ADBD_USB_BOARDCTL)
#  include <sys/boardctl.h>
#endif

#ifdef CONFIG_ADBD_NET_INIT
#  include "netutils/netinit.h"
#endif

#define ADB_WAIT_EP_READY(ep)             \
  {                                       \
    struct stat sb;                       \
                                          \
    while (stat(ep, &sb) != 0)            \
      {                                   \
        usleep(500000);                   \
      };                                  \
  }

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void adb_log_impl(int priority, FAR const char *func, int line,
                  FAR const char *fmt, ...)
{
  struct va_format vaf;
  va_list ap;

  va_start(ap, fmt);
  vaf.fmt = fmt;
  vaf.va  = &ap;

  switch (priority)
    {
      case ADB_INFO:
        priority = LOG_INFO;
        break;
      case ADB_ERR:
        priority = LOG_ERR;
        break;
      case ADB_WARN:
        priority = LOG_WARNING;
        break;
      default:
        priority = LOG_INFO;
        break;
    }

  syslog(priority, "%s (%d): %pV", func, line, &vaf);
  va_end(ap);
}

void adb_reboot_impl(FAR const char *target)
{
#ifdef CONFIG_BOARDCTL_RESET
  if (strcmp(target, "recovery") == 0)
    {
      boardctl(BOARDIOC_RESET, BOARDIOC_SOFTRESETCAUSE_ENTER_RECOVERY);
    }
  else if (strcmp(target, "bootloader") == 0)
    {
      boardctl(BOARDIOC_RESET, BOARDIOC_SOFTRESETCAUSE_ENTER_BOOTLOADER);
    }
  else
    {
      boardctl(BOARDIOC_RESET, BOARDIOC_SOFTRESETCAUSE_USER_REBOOT);
    }
#else
  adb_log("reboot not implemented\n");
#endif
}

int main(int argc, FAR char **argv)
{
  adb_context_t *ctx;

#ifdef CONFIG_ADBD_USB_BOARDCTL
  struct boardioc_usbdev_ctrl_s ctrl;
#  ifdef CONFIG_USBDEV_COMPOSITE
  uint8_t usbdev = BOARDIOC_USBDEV_COMPOSITE;
#  else
  uint8_t usbdev = BOARDIOC_USBDEV_ADB;
#  endif
  FAR void *handle;
  int ret;
#endif

#ifdef CONFIG_ADBD_BOARD_INIT
  boardctl(BOARDIOC_INIT, 0);
#endif /* CONFIG_ADBD_BOARD_INIT */

#ifdef CONFIG_ADBD_USB_BOARDCTL

  /* Setup USBADB device */

  /* Perform architecture-specific initialization */

  ctrl.usbdev   = usbdev;
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

  /* Connect the USB composite device device */

  ctrl.usbdev   = usbdev;
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
#endif /* ADBD_USB_BOARDCTL */

  ADB_WAIT_EP_READY("/dev/adb0/ep0");
  ADB_WAIT_EP_READY("/dev/adb0/ep1");
  ADB_WAIT_EP_READY("/dev/adb0/ep2");

#ifdef CONFIG_ADBD_NET_INIT
  /* Bring up the network */

  netinit_bringup();
#endif

  ctx = adb_hal_create_context();
  if (!ctx)
    {
      return 1;
    }

  adb_hal_run(ctx);
  adb_hal_destroy_context(ctx);
  return 0;
}
