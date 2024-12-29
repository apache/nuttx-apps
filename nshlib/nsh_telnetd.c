/****************************************************************************
 * apps/nshlib/nsh_telnetd.c
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

#include <assert.h>
#include <debug.h>

#include <sys/socket.h>

#include "nsh.h"
#include "nsh_console.h"

#ifdef CONFIG_NSH_TELNET

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_telnetmain
 ****************************************************************************/

int nsh_telnetmain(int argc, FAR char *argv[])
{
  FAR struct console_stdio_s *pstate = nsh_newconsole(true);
  int ret;

  DEBUGASSERT(pstate != NULL);

  /* Execute the session */

  ret = nsh_session(pstate, NSH_LOGIN_TELNET, argc, argv);

  /* Exit upon return */

  nsh_exit(&pstate->cn_vtbl, ret);
  return ret;
}

/****************************************************************************
 * Name: nsh_telnetstart
 *
 * Description:
 *   nsh_telnetstart() starts the Telnet daemon that will allow multiple
 *   NSH connections via Telnet.  This function returns immediately after
 *   the daemon has been started.
 *
 * Input Parameters:
 *   family - Provides the IP family to use by the server.  May be either
 *     AF_INET or AF_INET6.  This is needed because both both may be
 *     enabled in the configuration.
 *
 *   All of the other properties of the Telnet daemon are controlled by
 *   NuttX configuration settings.
 *
 * Returned Values:
 *   The task ID of the Telnet daemon was successfully started.  A negated
 *   errno value will be returned on failure.
 *
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_TELNETSTART
int nsh_telnetstart(sa_family_t family)
{
  FAR struct console_stdio_s *pstate = nsh_newconsole(false);
  int ret;

  DEBUGASSERT(pstate != NULL);

  /* Start the telnet daemon */

  ninfo("Starting the Telnet daemon\n");

#ifdef CONFIG_NET_IPv4
  if (family == AF_INET)
    {
      char cmdline[] = CONFIG_SYSTEM_TELNETD_PROGNAME " -4 &";
      ret = nsh_parse(&pstate->cn_vtbl, cmdline);
    }
  else
#endif
#ifdef CONFIG_NET_IPv6
  if (family == AF_INET6)
    {
      char cmdline[] = CONFIG_SYSTEM_TELNETD_PROGNAME " -6 &";
      ret = nsh_parse(&pstate->cn_vtbl, cmdline);
    }
  else
#endif
    {
      char cmdline[] = CONFIG_SYSTEM_TELNETD_PROGNAME " &";
      ret = nsh_parse(&pstate->cn_vtbl, cmdline);
    }

  if (ret < 0)
    {
      nerr("ERROR: Failed to start the Telnet daemon: %d\n", ret);
    }

  nsh_release(&pstate->cn_vtbl);
  return ret;
}
#endif

#endif /* CONFIG_NSH_TELNET */
