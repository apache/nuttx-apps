/****************************************************************************
 * apps/netutils/ftpc/ftpc_chmod.c
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
 * Name: ftpc_chmod
 *
 * Description:
 *   Change the protections on the remote file.
 *
 ****************************************************************************/

int ftpc_chmod(SESSION handle, FAR const char *path, FAR const char *mode)
{
  FAR struct ftpc_session_s *session = (FAR struct ftpc_session_s *)handle;

  /* Does the server support the size CHMOD command? */

  if (FTPC_HAS_CHMOD(session))
    {
      ftpc_cmd(session, "SITE CHMOD %s %s", path, mode);

      /* Check for "502 Command not implemented" */

      if (session->code == 502)
        {
          /* No.. the server does not support the SITE CHMOD command */

          FTPC_CLR_CHMOD(session);
        }

      return OK;
    }
  else
    {
      nwarn("WARNING: Server does not support SITE CHMOD\n");
    }

  return ERROR;
}
