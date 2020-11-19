/****************************************************************************
 * examples/watcher/wdt.c
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
#include <sys/boardctl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "wdt.h"
#include "task_mn.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Definitions
 ****************************************************************************/

static int wdt_print_handler(int irq, FAR void *context, FAR void *arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct wdog_params_s wdog =
{.timeout = CONFIG_EXAMPLES_WATCHER_TIMEOUT,
  .handlers.oldhandler = NULL,
  .handlers.newhandler = wdt_print_handler
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: wdt_print_handler
 *
 ****************************************************************************/

static int wdt_print_handler(int irq, FAR void *context, FAR void *arg)
{
  if (watched_tasks.head != NULL)
    {
      printf("*** Printing Tasks Status ***\n");
      task_mn_print_tasks_status();
      task_mn_reset_all();
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int wdt_init(void)
{
  int fd;
  int ret;
  strcpy(wdog.devname, CONFIG_EXAMPLES_WATCHER_DEVPATH);

  /* Open the watchdog device for reading */

  fd = open(wdog.devname, O_RDONLY);
  if (fd < 0)
    {
      int errcode = errno;
      printf("wdt_init: open %s failed: %d\n", wdog.devname, errcode);
      ret = errcode;
      goto errout;
    }

  /* Set the watchdog timeout */

  ret = ioctl(fd, WDIOC_SETTIMEOUT, (unsigned long)wdog.timeout);
  if (ret < 0)
    {
      int errcode = errno;
      printf("wdt_init: ioctl(WDIOC_SETTIMEOUT) failed: %d\n", errcode);
      ret = errcode;
      goto errout;
    }

  /* Register the expiration callback to be triggered on timeout */

  ret =
    ioctl(fd, WDIOC_CAPTURE, (unsigned long)((uintptr_t) & (wdog.handlers)));
  if (ret < 0)
    {
      int errcode = errno;
      printf("wdt_init: ioctl(WDIOC_CAPTURE) failed: %d\n", errcode);
      ret = errcode;
      goto errout;
    }

  /* Then start the watchdog timer. */

  ret = ioctl(fd, WDIOC_START, 0);
  if (ret < 0)
    {
      int errcode = errno;
      printf("wdt_init: ioctl(WDIOC_START) failed: %d\n", errcode);
      ret = errcode;
      goto errout;
    }

  close(fd);

errout:
  return ret;
}

void wdt_feed_the_dog(void)
{
  int fd;
  int ret;

  fd = open(wdog.devname, O_RDONLY);
  if (fd < 0)
    {
      fprintf(stderr, "trace: cannot open %s\n", wdog.devname);
      return;
    }

  ret = ioctl(fd, WDIOC_KEEPALIVE, 0);
  if (ret < 0)
    {
      printf("watcher: ioctl(WDIOC_KEEPALIVE) failed: %d\n", ret);
    }

  close(fd);
}
