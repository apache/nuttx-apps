/****************************************************************************
 * apps/include/fsutils/passwd.h
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

#ifndef __APPS_INCLUDE_FSUTILS_PASSWD_H
#define __APPS_INCLUDE_FSUTILS_PASSWD_H 1

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* passwd_verify() return value tests */

#define PASSWORD_VERIFY_MATCH(ret)   (ret == 1)
#define PASSWORD_VERIFY_NOMATCH(ret) (ret == 0)
#define PASSWORD_VERIFY_ERROR(ret)   (ret < 0)

/****************************************************************************
 * Public Function Prototypes
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

#if defined(CONFIG_FS_WRITABLE) && defined(CONFIG_FSUTILS_PASSWD_READONLY)
int passwd_adduser(FAR const char *username, FAR const char *password);

/****************************************************************************
 * Name: passwd_deluser
 *
 * Description:
 *   Remove an existing user from the /etc/passwd file.  If the user does
 *   not exist, then this function will fail.
 *
 * Input Parameters:
 *   username - Identifies the user to be deleted
 *
 * Returned Value:
 *   Zero (OK) is returned on success; a negated errno value is returned on
 *   failure.
 *
 ****************************************************************************/

int passwd_deluser(FAR const char *username);

/****************************************************************************
 * Name: passwd_update
 *
 * Description:
 *   Change a user in the /etc/passwd file.  If the user does not exist,
 *   then this function will fail.
 *
 * Input Parameters:
 *   username - Identifies the user whose password will be updated
 *   password - The new password for the existing user
 *
 * Returned Value:
 *   Zero (OK) is returned on success; a negated errno value is returned on
 *   failure.
 *
 ****************************************************************************/

int passwd_update(FAR const char *username, FAR const char *password);

#endif /* CONFIG_FS_WRITABLE && CONFIG_FSUTILS_PASSWD_READONLY */

/****************************************************************************
 * Name: passwd_verify
 *
 * Description:
 *   Return true if the username exists in the /etc/passwd file and if the
 *   password matches the user password in that faile.
 *
 * Input Parameters:
 *   username - Identifies the user whose password will be verified
 *   password - The password to be verified
 *
 * Returned Value:
 *   One (1) is returned on success match, Zero (OK) is returned on an
 *   unsuccessful match; a negated errno value is returned on any other
 *   failure.
 *
 ****************************************************************************/

int passwd_verify(FAR const char *username, FAR const char *password);

#endif /* __APPS_INCLUDE_FSUTILS_PASSWD_H */
