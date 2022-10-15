/****************************************************************************
 * apps/nshlib/nsh_init.c
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

#include "system/readline.h"
#include "nshlib/nshlib.h"

#include "nsh.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

#if defined(CONFIG_NSH_READLINE) && defined(CONFIG_READLINE_TABCOMPLETION) && \
    defined(CONFIG_READLINE_HAVE_EXTMATCH)
static const struct extmatch_vtable_s g_nsh_extmatch =
{
  nsh_extmatch_count,  /* count_matches */
  nsh_extmatch_getname /* getname */
};
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_initialize
 *
 * Description:
 *   This interface is used to initialize the NuttShell (NSH).
 *   nsh_initialize() should be called once during application start-up prior
 *   to executing nsh_consolemain().
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void nsh_initialize(void)
{
#if defined(CONFIG_NSH_READLINE) && defined(CONFIG_READLINE_TABCOMPLETION)
  /* Configure the NSH prompt */

  readline_prompt(g_nshprompt);

#ifdef CONFIG_READLINE_HAVE_EXTMATCH
  /* Set up for tab completion on NSH commands */

  readline_extmatch(&g_nsh_extmatch);
#endif
#endif

  /* Mount the /etc filesystem */

  (void)nsh_romfsetc();

#ifdef CONFIG_NSH_ARCHINIT
  /* Perform architecture-specific initialization (if configured) */

  boardctl(BOARDIOC_INIT, 0);
#endif

#if defined(CONFIG_NSH_TELNET) && !defined(CONFIG_NSH_DISABLE_TELNETSTART) && \
  !defined(CONFIG_NETINIT_NETLOCAL)
  /* If the Telnet console is selected as a front-end, then start the
   * Telnet daemon UNLESS network initialization is deferred via
   * CONFIG_NETINIT_NETLOCAL.  In that case, the telnet daemon must be
   * started manually with the telnetd command after the network has
   * been initialized
   */

  nsh_telnetstart(AF_UNSPEC);
#endif
}
