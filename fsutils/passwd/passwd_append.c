/****************************************************************************
 * apps/fsutils/passwd/passwd_append.c
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

#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include "passwd.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: passwd_append
 *
 * Description:
 *  Append a new record to the end of the /etc/passwd file
 *
 * Input Parameters:
 *
 * Returned Value:
 *   Zero (OK) is returned on success; a negated errno value is returned on
 *   failure.
 *
 ****************************************************************************/

int passwd_append(FAR const char *username, FAR const char *password)
{
  char encrypted[MAX_ENCRYPTED + 1];
  FILE *stream;
  int ret;

  /* Encrypt the raw password */

  ret = passwd_encrypt(password, encrypted);
  if (ret < 0)
    {
      return ret;
    }

  /* Append the new user record to the end of the password file */

  stream = fopen(CONFIG_FSUTILS_PASSWD_PATH, "a");
  if (stream == NULL)
   {
      int errcode = errno;
      DEBUGASSERT(errcode > 0);
      return errcode;
   }

  /* The format of the password file is:
   *
   *   user:x:uid:gid:home
   *
   * Where:
   *   user:  User name
   *   x:     Encrypted password
   *   uid:   User ID (0 for now)
   *   gid:   Group ID (0 for now)
   *   home:  Login directory (/ for now)
   */

  ret = fprintf(stream, "%s:%s:0:0:/\n", username, encrypted);
  if (ret < 0)
    {
      int errcode = errno;
      DEBUGASSERT(errcode > 0);
      ret = -errcode;
      goto errout_with_stream;
    }

  /* Return success */

  ret = OK;

errout_with_stream:
  fclose(stream);
  return ret;
}
