/****************************************************************************
 * apps/netutils/ftpc/ftpc_unlink.c
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
 * Name: ftpc_unlink
 *
 * Description:
 *   Delete the given file from the remote server.
 *
 ****************************************************************************/

int ftpc_unlink(SESSION handle, FAR const char *path)
{
  FAR struct ftpc_session_s *session = (FAR struct ftpc_session_s *)handle;
  int ret;

  /* A DELE request asks the server to remove a regular file.
   * A typical server accepts DELE with:
   *
   * - "250 Requested file action okay, completed" if the file was
   *    successfully removed
   *
   * Or rejects RMD with:
   *
   * - "550 Requested action not taken" f the removal failed.
   */

  ret = ftpc_cmd(session, "DELE %s", path);
  return ret;
}
