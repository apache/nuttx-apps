/****************************************************************************
 * apps/fsutils/passwd/passwd_append.c
 *
 *   Copyright (C) 2016, 2019 Gregory Nutt. All rights reserved.
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
