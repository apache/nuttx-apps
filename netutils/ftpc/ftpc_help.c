/****************************************************************************
 * apps/netutils/ftpc/ftpc_help.c
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

#include "ftpc_config.h"

#include "netutils/ftpc.h"

#include "ftpc_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ftpc_help
 *
 * Description:
 *   Request a list of available help commands.  This implementation is
 *   fragementary and no ready for any real use at this time.
 *
 ****************************************************************************/

int ftpc_help(SESSION handle, FAR const char *arg)
{
  FAR struct ftpc_session_s *session = (FAR struct ftpc_session_s *)handle;
  int ret;

  /* Send the HELP command with or without an argument */

  if (arg)
    {
      ret = ftpc_cmd(session, "HELP %s", arg);
    }
  else
    {
      ret = ftpc_cmd(session, "HELP");
    }

  /* Logic is missing here to return the help string to the caller.
   * The caller needs to call ftpc_getreply.
   */

  return ret;
}
