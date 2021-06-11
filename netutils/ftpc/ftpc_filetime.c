/****************************************************************************
 * apps/netutils/ftpc/ftpc_filetime.c
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

#include <string.h>
#include <time.h>

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
 * Name: ftpc_filetime
 *
 * Description:
 *   Return the timestamp on the remote file.  Returned time is UTC
 *   (Universal Coordinated Time).
 *
 ****************************************************************************/

time_t ftpc_filetime(SESSION handle, FAR const char *filename)
{
  FAR struct ftpc_session_s *session = (FAR struct ftpc_session_s *)handle;
  struct tm timestamp;
  int ret;

  /* Make sure that the server is still connected */

  if (!ftpc_connected(session))
    {
      return ERROR;
    }

  /* Does the server support the MDTM command? */

  if (!FTPC_HAS_MDTM(session))
    {
      return ERROR;
    }

  /* Get the file time in UTC */

  memset(&timestamp, 0, sizeof(timestamp));
  ret = ftpc_cmd(session, "MDTM %s", filename);
  if (ret != OK)
    {
      return ERROR;
    }

  /* Check for "202 Command not implemented, superfluous at this site" */

  if (session->code == 202)
    {
      FTPC_CLR_MDTM(session);
      return ERROR;
    }

  /* Check for "213 File status" */

  if (session->code != 213)
    {
      return ERROR;
    }

  /* Time is Universal Coordinated Time */

  sscanf(session->reply, "%*s %04d%02d%02d%02d%02d%02d",
         &timestamp.tm_year, &timestamp.tm_mon, &timestamp.tm_mday,
         &timestamp.tm_hour, &timestamp.tm_min, &timestamp.tm_sec);
  timestamp.tm_year -= 1900;
  timestamp.tm_mon--;
  return mktime(&timestamp);
}
