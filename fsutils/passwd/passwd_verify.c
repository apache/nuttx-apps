/****************************************************************************
 * apps/fsutils/passwd/passwd_verify.c
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
 *   password matches the user password in that faile.
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
  (void)passwd_unlock(sem);
  return ret;
}
