/****************************************************************************
 * apps/nshlib/nsh_usbconsole.c
 *
 *   Copyright (C) 2012-2014, 2016 Gregory Nutt. All rights reserved.
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
#include <fcntl.h>
#include <assert.h>
#include <debug.h>

#ifdef CONFIG_CDCACM
#  include <nuttx/usb/cdcacm.h>
#endif

#ifdef CONFIG_PL2303
#  include <nuttx/usb/pl2303.h>
#endif

#include "nsh.h"
#include "nsh_console.h"

#include "netutils/netinit.h"

#ifdef HAVE_USB_CONSOLE

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_configstdio
 *
 * Description:
 *   Configure standard I/O
 *
 ****************************************************************************/

static void nsh_configstdio(int fd)
{
  /* Make sure the stdout, and stderr are flushed */

  fflush(stdout);
  fflush(stderr);

  /* Dup the fd to create standard fd 0-2 */

  dup2(fd, 0);
  dup2(fd, 1);
  dup2(fd, 2);
}

/****************************************************************************
 * Name: nsh_nullstdio
 *
 * Description:
 *   Use /dev/null for standard I/O
 *
 ****************************************************************************/

static int nsh_nullstdio(void)
{
  int fd;

  /* Open /dev/null for read/write access */

  fd = open("/dev/null", O_RDWR);
  if (fd >= 0)
    {
      /* Configure standard I/O to use /dev/null */

      nsh_configstdio(fd);

      /* We can close the original file descriptor now (unless it was one of
       * 0-2)
       */

      if (fd > 2)
        {
          close(fd);
        }

      return OK;
    }

  return fd;
}

/****************************************************************************
 * Name: nsh_waitusbready
 *
 * Description:
 *   Wait for the USB console device to be ready
 *
 ****************************************************************************/

static int nsh_waitusbready(FAR struct console_stdio_s *pstate)
{
  char inch;
  ssize_t nbytes;
  int nlc;
  int fd;

  /* Don't start the NSH console until the console device is ready.  Chances
   * are, we get here with no functional console.  The USB console will not
   * be available until the device is connected to the host and until the
   * host-side application opens the connection.
   */

restart:

  /* Open the USB serial device for read/write access */

  do
    {
      /* Try to open the console */

      fd = open(CONFIG_NSH_USBCONDEV, O_RDWR);
      if (fd < 0)
        {
          /* ENOTCONN means that the USB device is not yet connected.
           * Anything else is bad.
           */

          DEBUGASSERT(errno == ENOTCONN);

          /* Sleep a bit and try again */

          sleep(2);
        }
    }
  while (fd < 0);

  /* Now wait until we successfully read a carriage return a few times.
   * That is a sure way of know that there is something at the other end of
   * the USB serial connection that is ready to talk with us.  The user needs
   * to hit ENTER a few times to get things started.
   */

  nlc = 0;
  do
    {
      /* Read one byte */

      inch = 0;
      nbytes = read(fd, &inch, 1);

      /* Is it a carriage return (or maybe a newline)? */

      if (nbytes == 1 && (inch == '\n' || inch == '\r'))
        {
          /* Yes.. increment the count */

          nlc++;
        }
      else
        {
          /* No.. Reset the count.  We need to see 3 in a row to continue. */

          nlc = 0;

          /* If a read error occurred (nbytes < 0) or an end-of-file was
           * encountered (nbytes == 0), then close the driver and start
           * over.
           */

          if (nbytes <= 0)
            {
              close(fd);
              goto restart;
            }
        }
    }
  while (nlc < 3);

  /* Configure standard I/O */

  nsh_configstdio(fd);

  /* We can close the original file descriptor (unless it was one of 0-2) */

  if (fd > 2)
    {
      close(fd);
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_consolemain (USB console version)
 *
 * Description:
 *   This interfaces maybe to called or started with task_start to start a
 *   single an NSH instance that operates on stdin and stdout.  This
 *   function does not return.
 *
 *   This function handles generic /dev/console character devices, or
 *   special USB console devices.  The USB console requires some special
 *   operations to handle the cases where the session is lost when the
 *   USB device is unplugged and restarted when the USB device is plugged
 *   in again.
 *
 * Input Parameters:
 *   Standard task start-up arguments.  These are not used.  argc may be
 *   zero and argv may be NULL.
 *
 * Returned Values:
 *   This function does not return nor does it ever exit (unless the user
 *   executes the NSH exit command).
 *
 ****************************************************************************/

int nsh_consolemain(int argc, FAR char *argv[])
{
  FAR struct console_stdio_s *pstate = nsh_newconsole();
  struct boardioc_usbdev_ctrl_s ctrl;
  FAR void *handle;
  int ret;

  DEBUGASSERT(pstate);

  /* Initialize any USB tracing options that were requested */

#ifdef CONFIG_NSH_USBDEV_TRACE
  usbtrace_enable(TRACE_BITSET);
#endif

  /* Initialize the USB serial driver */

#if defined(CONFIG_PL2303) || defined(CONFIG_CDCACM)
#ifdef CONFIG_CDCACM

  ctrl.usbdev   = BOARDIOC_USBDEV_CDCACM;
  ctrl.action   = BOARDIOC_USBDEV_CONNECT;
  ctrl.instance = CONFIG_NSH_USBDEV_MINOR;
  ctrl.handle   = &handle;

#else

  ctrl.usbdev   = BOARDIOC_USBDEV_PL2303;
  ctrl.action   = BOARDIOC_USBDEV_CONNECT;
  ctrl.instance = CONFIG_NSH_USBDEV_MINOR;
  ctrl.handle   = &handle;

#endif

  ret = boardctl(BOARDIOC_USBDEV_CONTROL, (uintptr_t)&ctrl);
  UNUSED(ret); /* Eliminate warning if not used */
  DEBUGASSERT(ret == OK);
#endif

  /* Configure to use /dev/null if we do not have a valid console. */

#ifndef CONFIG_DEV_CONSOLE
  nsh_nullstdio();
#endif

  /* Execute the one-time start-up script (output may go to /dev/null) */

#ifdef CONFIG_NSH_ROMFSETC
  nsh_initscript(&pstate->cn_vtbl);
#endif

#ifdef CONFIG_NSH_NETINIT
  /* Bring up the network */

  netinit_bringup();
#endif

#if defined(CONFIG_NSH_ARCHINIT) && defined(CONFIG_BOARDCTL_FINALINIT)
  /* Perform architecture-specific final-initialization (if configured) */

  boardctl(BOARDIOC_FINALINIT, 0);
#endif

  /* Now loop, executing creating a session for each USB connection */

  for (; ; )
    {
      /* Wait for the USB to be connected to the host and switch
       * standard I/O to the USB serial device.
       */

      ret = nsh_waitusbready(pstate);
      UNUSED(ret); /* Eliminate warning if not used */
      DEBUGASSERT(ret == OK);

      /* Execute the session */

      nsh_session(pstate, true, argc, argv);

      /* Switch to /dev/null because we probably no longer have a
       * valid console device.
       */

      nsh_nullstdio();
    }
}

#endif /* HAVE_USB_CONSOLE */
