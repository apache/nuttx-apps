/****************************************************************************
 * apps/nshlib/nsh_telnetd.c
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
#include <string.h>
#include <assert.h>
#include <debug.h>

#include <arpa/inet.h>

#include "netutils/telnetd.h"

#ifdef CONFIG_TELNET_CHARACTER_MODE
#ifdef CONFIG_NSH_CLE
#  include "system/cle.h"
#else
#  include "system/readline.h"
#endif
#endif

#include "nsh.h"
#include "nsh_console.h"

#ifdef CONFIG_NSH_TELNET

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum telnetd_state_e
{
  TELNETD_NOTRUNNING = 0,
  TELNETD_STARTED,
  TELNETD_RUNNING
};

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
  static enum telnetd_state_e state = TELNETD_NOTRUNNING;
  int ret = OK;

  if (state == TELNETD_NOTRUNNING)
    {
      struct telnetd_config_s config;

      /* There is a tiny race condition here if two tasks were to try to
       * start the Telnet daemon concurrently.
       */

      state = TELNETD_STARTED;

      /* Configure the telnet daemon */

      config.d_port      = HTONS(CONFIG_NSH_TELNETD_PORT);
      config.d_priority  = CONFIG_NSH_TELNETD_DAEMONPRIO;
      config.d_stacksize = CONFIG_NSH_TELNETD_DAEMONSTACKSIZE;
      config.t_priority  = CONFIG_NSH_TELNETD_CLIENTPRIO;
      config.t_stacksize = CONFIG_NSH_TELNETD_CLIENTSTACKSIZE;
      config.t_entry     = nsh_telnetmain;

      /* Start the telnet daemon */

      ninfo("Starting the Telnet daemon\n");

#ifdef CONFIG_NET_IPv4
      if (family == AF_UNSPEC || family == AF_INET)
        {
          config.d_family = AF_INET;
          ret = telnetd_start(&config);
          if (ret < 0)
            {
              _err("ERROR: Failed to start the Telnet IPv4 daemon: %d\n",
                   ret);
            }
          else
            {
              state = TELNETD_RUNNING;
            }
        }
#endif

#ifdef CONFIG_NET_IPv6
      if (family == AF_UNSPEC || family == AF_INET6)
        {
          config.d_family = AF_INET6;
          ret = telnetd_start(&config);
          if (ret < 0)
            {
              _err("ERROR: Failed to start the Telnet IPv6 daemon: %d\n",
                   ret);
            }
          else
            {
              state = TELNETD_RUNNING;
            }
        }
#endif

      if (state == TELNETD_STARTED)
        {
          state = TELNETD_NOTRUNNING;
        }
    }

  return ret;
}
#endif

/****************************************************************************
 * Name: cmd_telnetd
 *
 * Description:
 *   The Telnet daemon may be started either programmatically by calling
 *   nsh_telnetstart() or it may be started from the NSH command line using
 *   this telnetd command.
 *
 *   This command would be suppressed with CONFIG_NSH_DISABLE_TELNETD
 *   normally because the Telnet daemon is automatically started in
 *   nsh_main.c. The exception is when CONFIG_NETINIT_NETLOCAL is selected.
 *   IN that case, the network is not enabled at initialization but rather
 *   must be enabled from the NSH command line or via other applications.
 *
 *   In that case, calling nsh_telnetstart() before the the network is
 *   initialized will fail.
 *
 * Input Parameters:
 *   None.  All of the properties of the Telnet daemon are controlled by
 *   NuttX configuration setting.
 *
 * Returned Values:
 *   OK is returned on success; ERROR is return on failure.
 *
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_TELNETD
int cmd_telnetd(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  UNUSED(vtbl);
  UNUSED(argc);
  UNUSED(argv);

  sa_family_t family = AF_UNSPEC;

  /* If both IPv6 and IPv4 are enabled, then the address family must
   * be specified on the command line.
   */

#if defined(CONFIG_NET_IPv4) && defined(CONFIG_NET_IPv6)
  if (argc >= 2)
    {
      family = (strcmp(argv[1], "ipv6") == 0) ? AF_INET6 : AF_INET;
    }
#endif

  return nsh_telnetstart(family) < 0 ? ERROR : OK;
}
#endif

#endif /* CONFIG_NSH_TELNET */
