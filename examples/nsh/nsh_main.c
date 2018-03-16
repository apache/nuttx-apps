/****************************************************************************
 * examples/nsh/nsh_main.c
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

#if defined(CONFIG_FS_BINFS) && (CONFIG_BUILTIN)
#  include <nuttx/binfmt/builtin.h>
#endif

#if defined(CONFIG_LIBC_EXECFUNCS) && defined(CONFIG_EXECFUNCS_SYMTAB)
#  include <nuttx/binfmt/symtab.h>
#endif

#include "platform/cxxinitialize.h"

#include "nshlib/nshlib.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Kludge needed only for BINFS but should be harmless in other cases.  This
 * setups up an empty symbol table.  You will need to add logic to create
 * a "real" symbol table for your application elsewhere (see, for example
 * apps/system/symtab)
 */

#define HAVE_DUMMY_SYMTAB 1
#if !defined(CONFIG_LIBC_EXECFUNCS) || !defined(CONFIG_EXECFUNCS_SYMTAB)
#  undef HAVE_DUMMY_SYMTAB
#endif

/* boardctl() support is also required  for this "feature" */

#if !defined(CONFIG_LIB_BOARDCTL) || !defined(CONFIG_BOARDCTL_APP_SYMTAB)
#  undef HAVE_DUMMY_SYMTAB
#endif

#if defined(CONFIG_FS_BINFS) && !defined(HAVE_DUMMY_SYMTAB)
#  warning "Prequisites not met for BINFS dummy symbol table"
#endif

/* C++ initialization requires CXX initializer support */

#if !defined(CONFIG_HAVE_CXX) || !defined(CONFIG_HAVE_CXXINITIALIZE)
#  undef CONFIG_EXAMPLES_NSH_CXXINITIALIZE
#endif

/* The NSH telnet console requires networking support (and TCP/IP) */

#ifndef CONFIG_NET
#  undef CONFIG_NSH_TELNET
#endif

/* If Telnet is used and both IPv6 and IPv4 are enabled, then we need to
 * pick one.
 */

#ifdef CONFIG_NET_IPv6
#  define ADDR_FAMILY AF_INET6
#else
#  define ADDR_FAMILY AF_INET
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef HAVE_DUMMY_SYMTAB
/* If posix_spawn() is enabled as required for CONFIG_NSH_FILE_APPS, then
 * a symbol table is needed by the internals of posix_spawn().  The symbol
 * table is needed to support ELF and NXFLAT binaries to dynamically link to
 * the base code.  However, if only the BINFS file system is supported, then
 * no symbol table is needed.
 *
 * This will, of course, have to be replaced with a valid symbol table if
 * you want to support ELF or NXFLAT binaries!
 */

static const struct symtab_s CONFIG_EXECFUNCS_SYMTAB[1];  /* Wasted memory! */
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_main
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int nsh_main(int argc, char *argv[])
#endif
{
#ifdef HAVE_DUMMY_SYMTAB
  struct boardioc_symtab_s symdesc;
#endif
  int exitval = 0;
  int ret;

#if defined(CONFIG_EXAMPLES_NSH_CXXINITIALIZE)
  /* Call all C++ static constructors */

  up_cxxinitialize();
#endif

#ifdef HAVE_DUMMY_SYMTAB
  /* Make sure that we are using our symbol table */

  symdesc.symtab   = (FAR struct symtab_s *)g_dummy_symtab; /* Discard 'const' */
  symdesc.nsymbols = 0;
  (void)boardctl(BOARDIOC_APP_SYMTAB, (uintptr_t)&symdesc);
#endif

#if defined(CONFIG_FS_BINFS) && (CONFIG_BUILTIN)
  /* Register the BINFS file system */

  ret = builtin_initialize();
  if (ret < 0)
    {
     fprintf(stderr, "ERROR: builtin_initialize failed: %d\n", ret);
     exitval = 1;
   }
#endif

  /* Initialize the NSH library */

  nsh_initialize();

#if defined(CONFIG_NSH_TELNET) && !defined(CONFIG_NSH_NETLOCAL)
  /* If the Telnet console is selected as a front-end, then start the
   * Telnet daemon UNLESS network initialization is deferred via
   * CONFIG_NSH_NETLOCAL.  In that case, the telnet daemon must be
   * started manually with the telnetd command after the network has
   * been initialized
   */

  ret = nsh_telnetstart(ADDR_FAMILY);
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

#if CONFIG_NFILE_DESCRIPTORS > 0
  fprintf(stderr, "ERROR: nsh_consolemain() returned: %d\n", ret);
#else
  printf("ERROR: nsh_consolemain() returned: %d\n", ret);
#endif

  exitval = 1;
#endif

  return exitval;
}
