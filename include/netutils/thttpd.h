/****************************************************************************
 * apps/include/netutils/thttpd.h
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

#ifndef __APPS_INCLUDE_NETUTILS_THTTPD_H
#define __APPS_INCLUDE_NETUTILS_THTTPD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <nuttx/symtab.h>

/****************************************************************************
 * Public Data
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

#ifdef CONFIG_THTTPD_NXFLAT
/* These values must be provided by the user before the THTTPD task daemon
 * is started:
 *
 * g_thttpdsymtab:  A symbol table describing all of the symbols exported
 *   from the base system.  These symbols are used to bind address references
 *   in CGI programs to NuttX.
 * g_nsymbols:  The number of symbols in g_thttpdsymtab[].
 *
 * (See examples/nxflat and examples/thttpd for examples of how such a symbol
 *  table may be created.)
 */

EXTERN FAR const struct symtab_s *g_thttpdsymtab;
EXTERN int                        g_thttpdnsymbols;
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Function: thttpd_main
 *
 * Description:
 *   This function is the entrypoint into the THTTPD server.  It does not
 *   return.  It may be called, the normal mechanism for starting the server
 *   is:
 *
 *   1) Set is g_thttpdsymtab and g_thttpdnsymbols.  The user is required
 *      to provide a symbol table to use for binding CGI programs (if CGI
 *      is enabled.  See examples/nxflat and examples/thttpd for examples of
 *      how such a symbol table may be created.)
 *   2) Call task_create() to start thttpd_main()
 *
 ****************************************************************************/

int thttpd_main(int argc, char **argv);

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __APPS_INCLUDE_NETUTILS_THTTPD_H */
