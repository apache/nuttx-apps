/****************************************************************************
 * apps/netutils/ftpc/ftpc_idle.c
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

#include <debug.h>
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
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ftpc_idle
 *
 * Description:
 *  This command will change the FTP server's idle time limit with the site
 *  idle ftp command. This is useful if the default time limit is too short
 *  for the transmission of files).
 *
 ****************************************************************************/

int ftpc_idle(SESSION handle, unsigned int idletime)
{
  FAR struct ftpc_session_s *session = (FAR struct ftpc_session_s *)handle;
  int ret = OK;

  /* Check if the server supports the SITE IDLE command */

  if (!FTPC_HAS_IDLE(session))
    {
      nwarn("WARNING: Server does not support SITE IDLE\n");
      return ERROR;
    }

  /* Did the caller provide an IDLE time?  Or is this just a query for the
   * current IDLE time setting?
   */

  if (idletime)
    {
      ret = ftpc_cmd(session, "SITE IDLE %u", idletime);
    }
  else
    {
      ret = ftpc_cmd(session, "SITE IDLE");
    }

  /* Check for "502 Command not implemented" or 500 "Unknown SITE command" */

  if (session->code == 500 || session->code == 502)
    {
      /* Server does not support SITE IDLE */

      nwarn("WARNING: Server does not support SITE IDLE\n");
      FTPC_CLR_IDLE(session);
    }

  return ret;
}
