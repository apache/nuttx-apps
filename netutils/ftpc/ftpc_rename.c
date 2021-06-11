/****************************************************************************
 * apps/netutils/ftpc/ftpc_rename.c
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
#include <string.h>

#include "netutils/ftpc.h"

#include "ftpc_internal.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ftpc_rename
 *
 * Description:
 *   Rename a file on the remote server.
 *
 ****************************************************************************/

int ftpc_rename(SESSION handle, FAR const char *oldname,
                FAR const char *newname)
{
  FAR struct ftpc_session_s *session = (FAR struct ftpc_session_s *)handle;
  char *oldcopy;
  char *newcopy;
  int ret;

  oldcopy = strdup(oldname);
  if (!oldcopy)
    {
      return ERROR;
    }

  ftpc_stripslash(oldcopy);

  /* A RNFR request asks the server to begin renaming a file. A typical
   * server accepts RNFR with:
   *
   * - "350 Requested file action pending further information" if the file
   *   exists
   *
   * Or rejects RNFR with:
   *
   * - "450 Requested file action not taken"
   * - "550 Requested action not taken"
   */

  ret = ftpc_cmd(session, "RNFR %s", oldcopy);
  if (ret != OK)
    {
      free(oldcopy);
      return ERROR;
    }

  free(oldcopy);
  newcopy = strdup(newname);
  if (!newcopy)
    {
      return ERROR;
    }

  ftpc_stripslash(newcopy);

  /* A RNTO request asks the server to finish renaming a file. RNTO must
   * come immediately after RNFR; otherwise the server may reject RNTO with:
   *
   * - "503 Bad sequence of commands"
   *
   * A typical server accepts RNTO with:
   *
   * - "250 Requested file action okay, completed" if the file was renamed
   *   successfully
   *
   * Or rejects RMD with:
   *
   * - "550 Requested action not taken"
   * - "553 Requested action not taken"
   */

  ret = ftpc_cmd(session, "RNTO %s", newcopy);

  free(newcopy);
  return ret;
}
