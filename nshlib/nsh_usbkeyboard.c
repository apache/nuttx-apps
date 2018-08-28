/****************************************************************************
 * apps/nshlib/nsh_usbkeyboard.c
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <debug.h>

#include "nsh.h"
#include "nsh_console.h"

#if defined(HAVE_USB_KEYBOARD) && !defined(HAVE_USB_CONSOLE)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_wait_usbready
 *
 * Description:
 *   Wait for the USB keyboard device to be ready
 *
 ****************************************************************************/

static int nsh_wait_usbready(FAR const char *msg)
{
  int fd;

  /* Don't start the NSH console until the keyboard device is ready.  Chances
   * are, we get here with no functional stdin.  The USB keyboard device will
   * not be available until the device is connected to the host and enumerated.
   */

  /* Close standard fd 0.  Unbeknownst to stdin.  We do this here in case we
   * had the USB keyboard device open.  In that case, the driver will exist
   * we will get an ENODEV error when we try to open it (instead of ENOENT).
   *
   * NOTE: This might not be portable behavior!
   */

  (void)close(0);
  sleep(1);

  /* Open the USB keyboard device for read-only access */

  do
    {
      /* Try to open the console */

      fd = open(NSH_USBKBD_DEVNAME, O_RDONLY);
      if (fd < 0)
        {
#ifdef CONFIG_DEBUG_FEATURES
          int errcode = errno;

          /* ENOENT means that the USB device is not yet connected and,
           * hence, has no entry under /dev.  If the USB driver still
           * exists under /dev (because other threads still have the driver
           * open), then we might also get ENODEV.  Anything else would
           * be really bad.
           */

          DEBUGASSERT(errcode == ENOENT || errcode == ENODEV);
#endif

          /* Let the user know that we are waiting */

          if (msg)
            {
              /* Show the waiting message only one time after the failure
               * to open the keyboard device.
               */

              puts(msg);
              fflush(stdout);
              msg = NULL;
            }

          /* Sleep a bit and try again */

          sleep(2);
        }
    }
  while (fd < 0);

  /* Okay.. we have successfully opened a keyboard device.  Did
   * we just re-open fd 0?
   */

  if (fd != 0)
    {
       /* No..  Dup the fd to create standard fd 0.  stdin should not know. */

      (void)dup2(fd, 0);

      /* Close the keyboard device that we just opened */

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
 *   This function handles generic /dev/console character devices for output
 *   but uses a special USB keyboard device for input.  The USB keyboard
 *   requires some special operations to handle the cases where the session
 *   input is lost when the USB keyboard is unplugged and restarted when the
 *   USB keyboard is plugged in again.
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

int nsh_consolemain(int argc, char *argv[])
{
  FAR struct console_stdio_s *pstate = nsh_newconsole();
  FAR const char *msg;
  int ret;

  DEBUGASSERT(pstate);

  /* Initialize any USB tracing options that were requested */

#ifdef CONFIG_NSH_USBDEV_TRACE
  usbtrace_enable(TRACE_BITSET);
#endif

  /* Execute the one-time start-up script.  Any output will go to /dev/console. */

#ifdef CONFIG_NSH_ROMFSETC
  (void)nsh_initscript(&pstate->cn_vtbl);
#endif

#if defined(CONFIG_NSH_ARCHINIT) && defined(CONFIG_BOARDCTL_FINALINIT)
  /* Perform architecture-specific final-initialization (if configured) */

  (void)boardctl(BOARDIOC_FINALINIT, 0);
#endif

  /* Now loop, executing creating a session for each USB connection */

  msg = "Waiting for a keyboard...\n";
  for (;;)
    {
      /* Wait for the USB to be connected to the host and switch
       * standard I/O to the USB serial device.
       */

      ret = nsh_wait_usbready(msg);

      DEBUGASSERT(ret == OK);
      UNUSED(ret);

      /* Execute the session */

      (void)nsh_session(pstate);

      /* We lost the connection.  Wait for the keyboard to
       * be re-connected.
       */

      msg = "Please re-connect the keyboard...\n";
    }
}

#endif /* HAVE_USB_KEYBOARD && !HAVE_USB_CONSOLE */
