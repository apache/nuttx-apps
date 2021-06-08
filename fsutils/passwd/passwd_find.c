/****************************************************************************
 * apps/fsutils/passwd/passwd_find.c
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

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>

#include "passwd.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: passwd_find
 *
 * Description:
 *   Find a password in the
 *
 * Input Parameters:
 *
 * Returned Value:
 *   Zero (OK) is returned on success; a negated errno value is returned on
 *   failure.
 *
 ****************************************************************************/

int passwd_find(FAR const char *username, FAR struct passwd_s *passwd)
{
  FAR char *iobuffer;
  FAR char *name;
  FAR char *encrypted;
  FAR char *ptr;
  FILE *stream;
  off_t offset;
  int ret;

  /* Allocate an I/O buffer for the transfer */

  iobuffer = (FAR char *)malloc(CONFIG_FSUTILS_PASSWD_IOBUFFER_SIZE);
  if (iobuffer == NULL)
    {
      return -ENOMEM;
    }

  /* Open the password file for reading */

  stream = fopen(CONFIG_FSUTILS_PASSWD_PATH, "r");
  if (stream == NULL)
    {
      int errcode = errno;
      DEBUGASSERT(errcode > 0);
      return -errcode;
    }

  /* Read the password file line by line until the record with the matching
   * username is found, or until the end of the file is reached.
   *
   * The format of the password file is:
   *
   *   user:x:uid:gid:home
   *
   * Where:
   *   user:  User name
   *   x:     Encrypted password
   *   uid:   User ID
   *   gid:   Group ID
   *   home:  Login directory
   */

  offset = 0;
  ret    = -ENOENT;

  while (fgets(iobuffer, CONFIG_FSUTILS_PASSWD_IOBUFFER_SIZE, stream))
    {
      ptr  = iobuffer;
      name = ptr;

      /* Skip to the end of the name and properly terminate it,.  The name
       * must be terminated with the field delimiter ':'.
       */

      for (; *ptr != '\0' && *ptr != ':'; ptr++);
      if (*ptr == '\0')
        {
          /* Bad file format? */

          continue;
        }

      *ptr++ = '\0';

      /* Check for a username match */

      if (strcmp(username, name) == 0)
        {
          /* We have a match.  The encrypted password must immediately
           * follow the ':' delimiter.
           */

          encrypted = ptr;

          /* Skip to the end of the encrypted password and properly
           * terminate it.
           */

          for (; *ptr != '\0' && *ptr != ':'; ptr++);
          if (*ptr == '\0')
            {
              /* Bad file format? */

              ret = -EINVAL;
              break;
            }

          *ptr++ = '\0';

          /* Copy the offset and password into the returned structure */

          if (strlen(encrypted) >= MAX_ENCRYPTED)
            {
              ret = -E2BIG;
              break;
            }

          passwd->offset = offset;
          strncpy(passwd->encrypted, encrypted, MAX_ENCRYPTED);
          passwd->encrypted[MAX_ENCRYPTED] = '\0';

          ret = OK;
          break;
        }

      /* Get the next file offset */

      offset = ftell(stream);
    }

  fclose(stream);
  free(iobuffer);
  return ret;
}
