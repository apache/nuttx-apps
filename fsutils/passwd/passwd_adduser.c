/****************************************************************************
 * apps/fsutils/passwd/passwd_adduser.c
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

#include <semaphore.h>
#include <errno.h>

#include "fsutils/passwd.h"
#include "passwd.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: passwd_adduser
 *
 * Description:
 *   Add a new user to the /etc/passwd file.  If the user already exists,
 *   then this function will fail with -EEXIST.
 *
 * Input Parameters:
 *   username - Identifies the user to be added
 *   password - The password for the new user
 *
 * Returned Value:
 *   Zero (OK) is returned on success; a negated errno value is returned on
 *   failure.
 *
 ****************************************************************************/

int passwd_adduser(FAR const char *username, FAR const char *password)
{
  struct passwd_s passwd;
  PASSWD_SEM_DECL(sem);
  int ret;

  /* Get exclusive access to the /etc/passwd file */

  ret = passwd_lock(&sem);
  if (ret < 0)
    {
      return ret;
    }

  /* Check if the username already exists */

  ret = passwd_find(username, &passwd);
  if (ret >= 0)
    {
      /* The username already exists in the /etc/passwd file */

      ret = -EEXIST;
      goto errout_with_lock;
    }

  /* Append the new user to the end of the file */

  ret = passwd_append(username, password);
  if (ret < 0)
    {
      goto errout_with_lock;
    }

  /* Return success */

  ret = OK;

errout_with_lock:
  passwd_unlock(sem);
  return ret;
}
