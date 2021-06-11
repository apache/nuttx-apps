/****************************************************************************
 * apps/netutils/ftpc/ftpc_disconnect.c
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

#include <stdlib.h>

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
 * Name: ftpc_disconnect
 *
 * Description:
 *   Disconnect from the server and destroy the session handle..
 *
 ****************************************************************************/

void ftpc_disconnect(SESSION handle)
{
  FAR struct ftpc_session_s *session = (FAR struct ftpc_session_s *)handle;
  if (session)
    {
      /* Release sockets */

      ftpc_sockclose(&session->data);
      ftpc_sockclose(&session->dacceptor);
      ftpc_sockclose(&session->cmd);

      /* Free strings */

      free(session->uname);
      free(session->pwd);
      free(session->initrdir);
      free(session->homerdir);
      free(session->homeldir);
      free(session->currdir);

      /* Then destroy the session */

      free(session);
    }
}
