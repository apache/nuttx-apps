/****************************************************************************
 * apps/fsutils/passwd/passwd_adduser.c
 *
 *   Copyright (C) 2016 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
  (void)passwd_unlock(sem);
  return ret;
}
