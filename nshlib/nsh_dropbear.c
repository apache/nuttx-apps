/****************************************************************************
 * apps/nshlib/nsh_dropbear.c
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

#include "nsh.h"
#include "nsh_console.h"

#ifdef CONFIG_NSH_DROPBEAR

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_dropbearstart
 *
 * Description:
 *   nsh_dropbearstart() starts the Dropbear SSH server.  This function
 *   returns immediately after the daemon has been started.
 *
 * Returned Values:
 *   Zero is returned if Dropbear was started.  A negated errno value will be
 *   returned on failure.
 *
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_DROPBEARSTART
int nsh_dropbearstart(void)
{
  FAR struct console_stdio_s *pstate = nsh_newconsole(false);
  char cmdline[] = CONFIG_NETUTILS_DROPBEAR_PROGNAME " &";
  int ret;

  DEBUGASSERT(pstate != NULL);

  ninfo("Starting the Dropbear SSH server\n");

  ret = nsh_parse(&pstate->cn_vtbl, cmdline);
  if (ret < 0)
    {
      nerr("ERROR: Failed to start Dropbear: %d\n", ret);
    }

  nsh_release(&pstate->cn_vtbl);
  return ret;
}
#endif

#endif /* CONFIG_NSH_DROPBEAR */
