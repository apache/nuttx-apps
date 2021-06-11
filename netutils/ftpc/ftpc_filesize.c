/****************************************************************************
 * apps/netutils/ftpc/ftpc_filesize.c
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

#include <stdint.h>
#include <stdio.h>

#include "netutils/ftpc.h"

#include "ftpc_internal.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ftpc_filesize
 *
 * Description:
 *   Return the size of the given file on the remote server.
 *
 ****************************************************************************/

off_t ftpc_filesize(SESSION handle, FAR const char *path)
{
  FAR struct ftpc_session_s *session = (FAR struct ftpc_session_s *)handle;
  unsigned long ret;
  uint8_t mode = FTPC_XFRMODE_ASCII;

  /* Check if the host supports the SIZE command */

  if (!FTPC_HAS_SIZE(session))
    {
      return ERROR;
    }

#ifdef CONFIG_FTP_SIZE_CMD_MODE_BINARY
  mode = FTPC_XFRMODE_BINARY;
#endif

  if (ftpc_xfrmode(session, mode) != 0)
    {
      return ERROR;
    }

  ret = ftpc_cmd(session, "SIZE %s", path);

  /* Check for "502 Command not implemented" */

  if (session->code == 502)
    {
      /* No.. the host does not support the SIZE command */

      FTPC_CLR_SIZE(session);
      return ERROR;
    }

  sscanf(session->reply, "%*s %lu", &ret);
  return (off_t)ret;
}
