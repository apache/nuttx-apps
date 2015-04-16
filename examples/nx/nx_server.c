/****************************************************************************
 * examples/nx/nx_server.c
 *
 *   Copyright (C) 2008-2009, 2015 Gregory Nutt. All rights reserved.
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

#include <sys/boardctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/board.h>
#include <nuttx/nx/nx.h>

#ifdef CONFIG_NX_LCDDRIVER
#  include <nuttx/lcd/lcd.h>
#else
#  include <nuttx/video/fb.h>
#endif

#include "nx_internal.h"

#ifdef CONFIG_NX_MULTIUSER

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
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
 * Name: nx_servertask
 ****************************************************************************/

int nx_servertask(int argc, char *argv[])
{
  FAR NX_DRIVERTYPE *dev;
  int ret;

#if defined(CONFIG_EXAMPLES_NX_EXTERNINIT)
  struct boardioc_graphics_s devinfo;
  int ret;

  /* Use external graphics driver initialization */

  printf("nx_servertask: Initializing external graphics device\n");

  devinfo.devno = CONFIG_EXAMPLES_NX_DEVNO;
  devinfo.dev = NULL;

  ret = boardctl(BOARDIOC_GRAPHICS_SETUP, (uintptr_t)&devinfo);
  if (ret < 0)
    {
      printf("nx_servertask: boardctl failed, devno=%d: %d\n",
             CONFIG_EXAMPLES_NX_DEVNO, errno);
      g_exitcode = NXEXIT_EXTINITIALIZE;
      return ERROR;
    }

  dev = devinfo.dev;

#elif defined(CONFIG_NX_LCDDRIVER)
  /* Initialize the LCD device */

  printf("nx_servertask: Initializing LCD\n");
  ret = board_lcd_initialize();
  if (ret < 0)
    {
      printf("nx_servertask: board_lcd_initialize failed: %d\n", -ret);
      return 1;
    }

  /* Get the device instance */

  dev = board_lcd_getdev(CONFIG_EXAMPLES_NX_DEVNO);
  if (!dev)
    {
      printf("nx_servertask: board_lcd_getdev failed, devno=%d\n",
             CONFIG_EXAMPLES_NX_DEVNO);
      return 2;
    }

  /* Turn the LCD on at 75% power */

  (void)dev->setpower(dev, ((3*CONFIG_LCD_MAXPOWER + 3)/4));
#else
  /* Initialize the frame buffer device */

  printf("nx_servertask: Initializing framebuffer\n");
  ret = up_fbinitialize();
  if (ret < 0)
    {
      printf("nx_servertask: up_fbinitialize failed: %d\n", -ret);
      return 1;
    }

  dev = up_fbgetvplane(CONFIG_EXAMPLES_NX_VPLANE);
  if (!dev)
    {
      printf("nx_servertask: up_fbgetvplane failed, vplane=%d\n", CONFIG_EXAMPLES_NX_VPLANE);
      return 2;
    }
#endif

  /* Then start the server */

  ret = nx_run(dev);
  printf("nx_servertask: nx_run returned: %d\n", errno);
  return 3;
}

#endif /* CONFIG_NX_MULTIUSER */
