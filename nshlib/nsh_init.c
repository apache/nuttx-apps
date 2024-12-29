/****************************************************************************
 * apps/nshlib/nsh_init.c
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

#include <sys/boardctl.h>
#include <nuttx/symtab.h>

#include "system/readline.h"
#include "nshlib/nshlib.h"

#include "nsh.h"
#include "nsh_console.h"

#ifdef CONFIG_NSH_NETINIT
#  include "netutils/netinit.h"
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Symbol table is not needed if loadable binary modules are not supported */

#if !defined(CONFIG_LIBC_EXECFUNCS)
#  undef CONFIG_NSH_SYMTAB
#endif

/* boardctl() support is also required for application-space symbol table
 * support.
 */

#if !defined(CONFIG_BOARDCTL) || !defined(CONFIG_BOARDCTL_APP_SYMTAB)
#  undef CONFIG_NSH_SYMTAB
#endif

/* If a symbol table is provided by board-specific logic, then we do not
 * need to do anything from the application space.
 */

#ifdef CONFIG_EXECFUNCS_HAVE_SYMTAB
#  undef CONFIG_NSH_SYMTAB
#endif

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

#if defined(CONFIG_NSH_SYMTAB)
extern const struct symtab_s CONFIG_NSH_SYMTAB_ARRAYNAME[];
extern const int CONFIG_NSH_SYMTAB_COUNTNAME;
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
#if defined (CONFIG_NSH_SYMTAB)
  struct boardioc_symtab_s symdesc;
#endif
#if defined(CONFIG_ETC_ROMFS) && !defined(CONFIG_NSH_DISABLESCRIPT)
  FAR struct console_stdio_s *pstate;
#endif

  /* populate NSH prompt string */

  nsh_update_prompt();

#if defined(CONFIG_NSH_READLINE) && defined(CONFIG_READLINE_TABCOMPLETION)
  /* Configure readline prompt */

  readline_prompt(nsh_prompt());

#  ifdef CONFIG_READLINE_HAVE_EXTMATCH
  /* Set up for tab completion on NSH commands */

  readline_extmatch(&g_nsh_extmatch);
#  endif
#endif

#ifdef CONFIG_NSH_USBDEV_TRACE
  /* Initialize any USB tracing options that were requested */

  usbtrace_enable(TRACE_BITSET);
#endif

#if defined(CONFIG_NSH_SYMTAB)
  /* Make sure that we are using our symbol table */

  symdesc.symtab   = CONFIG_NSH_SYMTAB_ARRAYNAME;
  symdesc.nsymbols = CONFIG_NSH_SYMTAB_COUNTNAME;

  boardctl(BOARDIOC_APP_SYMTAB, (uintptr_t)&symdesc);
#endif

#ifdef CONFIG_NSH_ARCHINIT
  /* Perform architecture-specific initialization (if configured) */

  boardctl(BOARDIOC_INIT, 0);
#endif

#if defined(CONFIG_ETC_ROMFS) && !defined(CONFIG_NSH_DISABLESCRIPT)
  pstate = nsh_newconsole(false);

  /* Execute the system init script */

  nsh_sysinitscript(&pstate->cn_vtbl);
#endif

#ifdef CONFIG_NSH_NETINIT
  /* Bring up the network */

  netinit_bringup();
#endif

#if defined(CONFIG_NSH_ARCHINIT) && defined(CONFIG_BOARDCTL_FINALINIT)
  /* Perform architecture-specific final-initialization (if configured) */

  boardctl(BOARDIOC_FINALINIT, 0);
#endif

#if defined(CONFIG_ETC_ROMFS) && !defined(CONFIG_NSH_DISABLESCRIPT)
  /* Execute the start-up script */

  nsh_initscript(&pstate->cn_vtbl);
  nsh_release(&pstate->cn_vtbl);
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
