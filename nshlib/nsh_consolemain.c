/****************************************************************************
 * apps/nshlib/nsh_consolemain.c
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
#include <assert.h>

#include <sys/boardctl.h>

#include "nsh.h"
#include "nsh_console.h"

#include "netutils/netinit.h"

#if !defined(CONFIG_NSH_ALTCONDEV) && !defined(HAVE_USB_CONSOLE) && \
    !defined(HAVE_USB_KEYBOARD)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_consolemain (Normal character device version)
 *
 * Description:
 *   This interfaces may be to called or started with task_start to start a
 *   single an NSH instance that operates on stdin and stdout.  This
 *   function does not normally return (see below).
 *
 *   This version of nsh_consolmain handles generic /dev/console character
 *   devices (see nsh_usbconsole.c and usb_usbkeyboard for other versions
 *   for special USB console devices).
 *
 * Input Parameters:
 *   Standard task start-up arguments.  These are not used.  argc may be
 *   zero and argv may be NULL.
 *
 * Returned Values:
 *   This function does not normally return.  exit() is usually called to
 *   terminate the NSH session.  This function will return in the event of
 *   an error.  In that case, a non-zero value is returned (EXIT_FAILURE=1).
 *
 ****************************************************************************/

int nsh_consolemain(int argc, FAR char *argv[])
{
  FAR struct console_stdio_s *pstate = nsh_newconsole(true);
  int ret;

  DEBUGASSERT(pstate != NULL);

#ifdef CONFIG_NSH_USBDEV_TRACE
  /* Initialize any USB tracing options that were requested */

  usbtrace_enable(TRACE_BITSET);
#endif

#ifdef CONFIG_NSH_ROMFSETC
  /* Execute the start-up script */

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

  /* Execute the session */

  ret = nsh_session(pstate, true, argc, argv);

  /* Exit upon return */

  nsh_exit(&pstate->cn_vtbl, ret);
  return ret;
}

#endif /* !HAVE_USB_CONSOLE && !HAVE_USB_KEYBOARD !HAVE_SLCD_CONSOLE */
