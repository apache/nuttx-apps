/****************************************************************************
 * apps/system/nsh/nsh_main.c
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

#include <sys/stat.h>
#include <sys/boardctl.h>
#include <stdint.h>
#include <stdio.h>
#include <sched.h>
#include <errno.h>

#if defined(CONFIG_LIBC_EXECFUNCS)
#  include <nuttx/symtab.h>
#endif

#include "nshlib/nshlib.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Symbol table is not needed if loadable binary modules are not supported */

#if !defined(CONFIG_LIBC_EXECFUNCS)
#  undef CONFIG_SYSTEM_NSH_SYMTAB
#endif

/* boardctl() support is also required for application-space symbol table
 * support.
 */

#if !defined(CONFIG_BOARDCTL) || !defined(CONFIG_BOARDCTL_APP_SYMTAB)
#  undef CONFIG_SYSTEM_NSH_SYMTAB
#endif

/* If a symbol table is provided by board-specific logic, then we do not
 * need to do anything from the application space.
 */

#ifdef CONFIG_EXECFUNCS_HAVE_SYMTAB
#  undef CONFIG_SYSTEM_NSH_SYMTAB
#endif

/* The NSH telnet console requires networking support (and TCP/IP) */

#ifndef CONFIG_NET
#  undef CONFIG_NSH_TELNET
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#if defined(CONFIG_SYSTEM_NSH_SYMTAB)

extern const struct symtab_s CONFIG_SYSTEM_NSH_SYMTAB_ARRAYNAME[];
extern const int CONFIG_SYSTEM_NSH_SYMTAB_COUNTNAME;

#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_main
 *
 * Description:
 *   This is the main logic for the case of the NSH task.  It will perform
 *   one-time NSH initialization and start an interactive session on the
 *   current console device.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
#if defined (CONFIG_SYSTEM_NSH_SYMTAB)
  struct boardioc_symtab_s symdesc;
#endif
  struct sched_param param;
  int exitval = 0;
  int ret;

  /* Check the task priority that we were started with */

  sched_getparam(0, &param);
  if (param.sched_priority != CONFIG_SYSTEM_NSH_PRIORITY)
    {
      /* If not then set the priority to the configured priority */

      param.sched_priority = CONFIG_SYSTEM_NSH_PRIORITY;
      sched_setparam(0, &param);
    }

#if defined(CONFIG_SYSTEM_NSH_SYMTAB)
  /* Make sure that we are using our symbol table */

  symdesc.symtab   = (FAR struct symtab_s *)CONFIG_SYSTEM_NSH_SYMTAB_ARRAYNAME; /* Discard 'const' */
  symdesc.nsymbols = CONFIG_SYSTEM_NSH_SYMTAB_COUNTNAME;

  boardctl(BOARDIOC_APP_SYMTAB, (uintptr_t)&symdesc);
#endif

  /* Initialize the NSH library */

  nsh_initialize();

#if defined(CONFIG_NSH_TELNET) && !defined(CONFIG_NETINIT_NETLOCAL)
  /* If the Telnet console is selected as a front-end, then start the
   * Telnet daemon UNLESS network initialization is deferred via
   * CONFIG_NETINIT_NETLOCAL.  In that case, the telnet daemon must be
   * started manually with the telnetd command after the network has
   * been initialized
   */

  ret = nsh_telnetstart(AF_UNSPEC);
  if (ret < 0)
    {
      /* The daemon is NOT running.  Report the error then fail...
       * either with the serial console up or just exiting.
       */

      fprintf(stderr, "ERROR: Failed to start TELNET daemon: %d\n", ret);
      exitval = 1;
    }
#endif

#ifdef CONFIG_NSH_CONSOLE
  /* If the serial console front end is selected, run it on this thread */

  ret = nsh_consolemain(argc, argv);

  /* nsh_consolemain() should not return.  So if we get here, something
   * is wrong.
   */

  fprintf(stderr, "ERROR: nsh_consolemain() returned: %d\n", ret);
  exitval = 1;
#endif

  return exitval;
}
