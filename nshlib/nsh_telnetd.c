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

#include "netutils/netinit.h"
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
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_telnetmain
 ****************************************************************************/

static int nsh_telnetmain(int argc, char *argv[])
{
  FAR struct console_stdio_s *pstate = nsh_newconsole(true);
  FAR struct nsh_vtbl_s *vtbl;
  int ret;

  DEBUGASSERT(pstate != NULL);
  vtbl = &pstate->cn_vtbl;

  ninfo("Session [%d] Started\n", getpid());

#ifdef CONFIG_NSH_TELNET_LOGIN
  /* Login User and Password Check */

  if (nsh_telnetlogin(pstate) != OK)
    {
      nsh_exit(vtbl, 1);
      return -1; /* nsh_exit does not return */
    }
#endif /* CONFIG_NSH_TELNET_LOGIN */

  /* The following logic mostly the same as the login in nsh_session.c.  It
   * differs only in that gets() is called to get the command instead of
   * readline().
   */

  /* Present a greeting and possibly a Message of the Day (MOTD) */

  fputs(g_nshgreeting, pstate->cn_outstream);

#ifdef CONFIG_NSH_MOTD
# ifdef CONFIG_NSH_PLATFORM_MOTD
  /* Output the platform message of the day */

  platform_motd(vtbl->iobuffer, IOBUFFERSIZE);
  fprintf(pstate->cn_outstream, "%s\n", vtbl->iobuffer);

# else
  /* Output the fixed message of the day */

  fprintf(pstate->cn_outstream, "%s\n", g_nshmotd);
# endif
#endif

  fflush(pstate->cn_outstream);

  /* Execute the login script */

#ifdef CONFIG_NSH_ROMFSRC
  nsh_loginscript(vtbl);
#endif

  /* Then enter the command line parsing loop */

  for (; ; )
    {
      /* Get the next line of input from the Telnet client */

#ifdef CONFIG_TELNET_CHARACTER_MODE
#ifdef CONFIG_NSH_CLE
      /* cle() returns a negated errno value on failure (errno is not set) */

      ret = cle(pstate->cn_line, g_nshprompt, CONFIG_NSH_LINELEN,
                INSTREAM(pstate), OUTSTREAM(pstate));
      if (ret < 0)
        {
          fprintf(pstate->cn_errstream, g_fmtcmdfailed, "nsh_telnetmain",
                  "cle", NSH_ERRNO_OF(-ret));
          nsh_exit(vtbl, 1);
        }
#else
      /* Display the prompt string */

      fputs(g_nshprompt, pstate->cn_outstream);
      fflush(pstate->cn_outstream);

      /* readline() returns EOF on failure (errno is not set) */

      ret = readline(pstate->cn_line, CONFIG_NSH_LINELEN,
                     INSTREAM(pstate), OUTSTREAM(pstate));
      if (ret == EOF)
        {
          /* NOTE: readline() does not set the errno variable, but perhaps we
           * will be lucky and it will still be valid.
           */

          fprintf(pstate->cn_errstream, g_fmtcmdfailed, "nsh_telnetmain",
                  "readline", NSH_ERRNO);
          nsh_exit(vtbl, 1);
        }
#endif
#else
      /* Display the prompt string */

      fputs(g_nshprompt, pstate->cn_outstream);
      fflush(pstate->cn_outstream);

      /* fgets() returns NULL on failure (errno will be set) */

      if (fgets(pstate->cn_line, CONFIG_NSH_LINELEN,
                INSTREAM(pstate)) == NULL)
        {
          fprintf(pstate->cn_errstream, g_fmtcmdfailed, "nsh_telnetmain",
                  "fgets", NSH_ERRNO);
          nsh_exit(vtbl, 1);
        }

      ret = strlen(pstate->cn_line);
#endif

      /* Parse process the received Telnet command */

      nsh_parse(vtbl, pstate->cn_line);
      fflush(pstate->cn_outstream);
    }

  /* Clean up */

  nsh_exit(vtbl, 0);

  /* We do not get here, but this is necessary to keep some compilers happy */

  UNUSED(ret);
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

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

int nsh_telnetstart(sa_family_t family)
{
  static enum telnetd_state_e state = TELNETD_NOTRUNNING;
  int ret = OK;

  if (state == TELNETD_NOTRUNNING)
    {
#if defined(CONFIG_NSH_ROMFSETC) && !defined(CONFIG_NSH_CONSOLE)
      FAR struct console_stdio_s *pstate;
#endif
      struct telnetd_config_s config;

      /* There is a tiny race condition here if two tasks were to try to
       * start the Telnet daemon concurrently.
       */

      state = TELNETD_STARTED;

      /* Initialize any USB tracing options that were requested.  If
       * standard console is also defined, then we will defer this step to
       * the standard console.
       */

#if defined(CONFIG_NSH_USBDEV_TRACE) && !defined(CONFIG_NSH_CONSOLE)
      usbtrace_enable(TRACE_BITSET);
#endif

      /* Execute the startup script.  If standard console is also defined,
       * then we will not bother with the initscript here (although it is
       * safe to call nsh_initscript multiple times).
       */

#if defined(CONFIG_NSH_ROMFSETC) && !defined(CONFIG_NSH_CONSOLE)
      pstate = nsh_newconsole();
      nsh_initscript(&pstate->cn_vtbl);
      nsh_release(&pstate->cn_vtbl);
#endif

#if defined(CONFIG_NSH_NETINIT) && !defined(CONFIG_NSH_CONSOLE)
      /* Bring up the network */

      netinit_bringup();
#endif

      /* Perform architecture-specific final-initialization(if configured) */

#if defined(CONFIG_NSH_ARCHINIT) && \
    defined(CONFIG_BOARDCTL_FINALINIT) && \
    !defined(CONFIG_NSH_CONSOLE)
      boardctl(BOARDIOC_FINALINIT, 0);
#endif

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
int cmd_telnetd(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
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
