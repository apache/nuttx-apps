/****************************************************************************
 * apps/netutils/ftpc/ftpc_rpwd.c
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
#include <debug.h>

#include "netutils/ftpc.h"

#include "ftpc_internal.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ftpc_rpwd
 *
 * Description:
 *   Returns the current working directory on the remote server.
 *
 ****************************************************************************/

FAR char *ftpc_rpwd(SESSION handle)
{
  FAR struct ftpc_session_s *session = (FAR struct ftpc_session_s *)handle;
  FAR char *start;
  FAR char *end;
  FAR char *pwd;
  FAR char *ptr;
  int len;

  /* Send the PWD command */

  ftpc_cmd(session, "PWD");

  /* Response is like: 257 "/home/gnutt" (from vsftpd).
   *
   * Extract the quoted path name into allocated memory.
   */

  start = strchr(session->reply, '\"');
  if (!start)
    {
      nwarn("WARNING: Opening quote not found\n");
      return NULL;
    }

  start++;

  end = strchr(start, '\"');
  if (!end)
    {
      nwarn("WARNING: Closing quote not found\n");
      return NULL;
    }

  /* Allocate memory for the path name:
   *
   *   Reply: 257 "/home/gnutt"
   *               ^start     ^end
   *
   *   len = end - start + 1 = 11 (+ NUL terminator)
   */

  len = end - start + 1;
  pwd = (char *)malloc(len + 1);
  if (!pwd)
    {
      nerr("ERROR: Failed to allocate string\n");
      return NULL;
    }

  /* Copy the string into the allocated memory */

  memcpy(pwd, start, len);
  pwd[len] = '\0';

  /* Remove any trailing slash that the server may have added */

  ftpc_stripslash(pwd);

  /* Change DOS style directory separator ('\') to UNIX style ('/') */

  for (ptr = pwd; *ptr; ptr++)
    {
      if (*ptr == '\\')
        {
          *ptr = '/';
        }
    }

  return pwd;
}
