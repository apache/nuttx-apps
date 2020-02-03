/****************************************************************************
 * system/nsh/nsh_main.c
 *
 *   Copyright (C) 2007-2013, 2017-2018 Gregory Nutt. All rights reserved.
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

#include <sys/stat.h>
#include <sys/boardctl.h>
#include <stdint.h>
#include <stdio.h>
#include <sched.h>
#include <errno.h>

#if defined(CONFIG_LIBC_EXECFUNCS)
#  include <nuttx/symtab.h>
#endif

#include "platform/cxxinitialize.h"

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

#if !defined(CONFIG_LIB_BOARDCTL) || !defined(CONFIG_BOARDCTL_APP_SYMTAB)
#  undef CONFIG_SYSTEM_NSH_SYMTAB
#endif

/* If a symbol table is provided by board-specific logic, then we do not
 * need to do anything from the application space.
 */

#ifdef CONFIG_EXECFUNCS_HAVE_SYMTAB
#  undef CONFIG_SYSTEM_NSH_SYMTAB
#endif

/* Check if we need to build in support for the system() and/or popen()
 * functions.  In the KERNEL build mode (only), NSH is build as a ELF
 * program and must be capable of executing a single command provided
 * on the command line.
 */

#undef HAVE_NSH_COMMAND
#if defined(CONFIG_SYSTEM_SYSTEM) || defined(CONFIG_SYSTEM_POPEN)
#  define HAVE_NSH_COMMAND 1
#endif

/* C++ initialization requires CXX initializer support */

#if !defined(CONFIG_HAVE_CXX) || !defined(CONFIG_HAVE_CXXINITIALIZE)
#  undef CONFIG_SYSTEM_NSH_CXXINITIALIZE
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
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_task
 *
 * Description:
 *   This is the main logic for the case of the NSH task.  It will perform
 *   one-time NSH initialization and start an interactive session on the
 *   current console device.
 *
 ****************************************************************************/

static int nsh_task(void)
{
#if defined (CONFIG_SYSTEM_NSH_SYMTAB)
  struct boardioc_symtab_s symdesc;
#endif
  int exitval = 0;
  int ret;

#if defined(CONFIG_SYSTEM_NSH_CXXINITIALIZE)
  /* Call all C++ static constructors */

  up_cxxinitialize();
#endif

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
  /* If the serial console front end is selected, then run it on this thread */

  ret = nsh_consolemain(0, NULL);

  /* nsh_consolemain() should not return.  So if we get here, something
   * is wrong.
   */

  fprintf(stderr, "ERROR: nsh_consolemain() returned: %d\n", ret);
  exitval = 1;
#endif

  return exitval;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct sched_param param;

  /* Check the task priority that we were started with */

  sched_getparam(0, &param);
  if (param.sched_priority != CONFIG_SYSTEM_NSH_PRIORITY)
    {
      /* If not then set the priority to the configured priority */

      param.sched_priority = CONFIG_SYSTEM_NSH_PRIORITY;
      sched_setparam(0, &param);
    }

  /* There are two modes that NSH can be executed in:
   *
   * 1) As a normal, interactive shell.  In this case, no arguments are
   *    expected on the command line.  OR
   * 2) As a single command processor.  In this case, the single command is
   *    is provided in argv[1].
   *
   * NOTE:  The latter mode is only available if CONFIG_SYSTEM_NSH=m.
   * In that case, this main() function will be built as a process.  The
   * process will be started with a command by the implementations of the
   * system() and popen() interfaces.
   */

#ifdef HAVE_NSH_COMMAND
  if (argc > 1)
    {
      return nsh_system(argc, argv);
    }
  else
#endif
    {
      return nsh_task();
    }
}
