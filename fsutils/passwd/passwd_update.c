/****************************************************************************
 * apps/fsutils/passwd/passwd_update.c
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

#include "fsutils/passwd.h"
#include <passwd.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: passwd_update
 *
 * Description:
 *   Change a user in the /etc/passwd file.  If the user does not exist,
 *   then this function will fail.
 *
 * Input Parameters:
 *
 * Returned Value:
 *   Zero (OK) is returned on success; a negated errno value is returned on
 *   failure.
 *
 ****************************************************************************/

int passwd_update(FAR const char *username, FAR const char *password)
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

  /* Verify that the username exists in the /etc/passwd file */

  ret = passwd_find(username, &passwd);
  if (ret < 0)
    {
      /* The username does not exist in the /etc/passwd file */

      goto errout_with_lock;
    }

  /* Remove the line containing this user from the /etc/passwd file */

  ret = passwd_delete(passwd.offset);
  if (ret < 0)
    {
      goto errout_with_lock;
    }

  /* Then append the new password record to the end of the file */

  ret = passwd_append(username, password);

errout_with_lock:
  passwd_unlock(sem);
  return ret;
}
