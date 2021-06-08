/****************************************************************************
 * apps/fsutils/passwd/passwd_verify.c
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

#include <string.h>
#include <semaphore.h>

#include "fsutils/passwd.h"
#include "passwd.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: passwd_verify
 *
 * Description:
 *   Return true if the username exists in the /etc/passwd file and if the
 *   password matches the user password in that failed.
 *
 * Input Parameters:
 *
 * Returned Value:
 *   One (1) is returned on success match, Zero (OK) is returned on an
 *   unsuccessful match; a negated errno value is returned on any other
 *   failure.
 *
 ****************************************************************************/

int passwd_verify(FAR const char *username, FAR const char *password)
{
  struct passwd_s passwd;
  char encrypted[MAX_ENCRYPTED + 1];
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

  /* Encrypt the provided password */

  ret = passwd_encrypt(password, encrypted);
  if (ret < 0)
    {
      goto errout_with_lock;
    }

  /* Compare the encrypted passwords */

  ret = (strcmp(passwd.encrypted, encrypted) == 0) ? 1 : 0;

errout_with_lock:
  passwd_unlock(sem);
  return ret;
}
