/****************************************************************************
 * apps/nshlib/nsh_system.c
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
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "nsh.h"
#include "nsh_console.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_system
 *
 * Description:
 *   This is the NSH-specific implementation of the standard system()
 *   command.
 *
 *   NOTE: This assumes that other NSH instances have previously ran and so
 *   common NSH logic is already initialized.
 *
 * Input Parameters:
 *   Standard task start-up arguments.  Expects argc == 2 with argv[1] being
 *   the command to execute
 *
 * Returned Values:
 *   EXIT_SUCCESS or EXIT_FAILURE
 *
 ****************************************************************************/

int nsh_system(int argc, FAR char *argv[])
{
  FAR struct console_stdio_s *pstate = nsh_newconsole(false);
  int ret;

  DEBUGASSERT(pstate != NULL);

  /* Execute the session */

  ret = nsh_session(pstate, false, argc, argv);

  /* Exit upon return */

  nsh_exit(&pstate->cn_vtbl, ret);
  return ret;
}
