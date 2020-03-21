/****************************************************************************
 * apps/include/fsutils/passwd.h
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

#ifndef __APPS_INCLUDE_FSUTILS_PASSWD_H
#define __APPS_INCLUDE_FSUTILS_PASSWD_H

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

#if defined(CONFIG_FSUTILS_PASSWD_READONLY)
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

#endif /* CONFIG_FSUTILS_PASSWD_READONLY */

/****************************************************************************
 * Name: passwd_verify
 *
 * Description:
 *   Return true if the username exists in the /etc/passwd file and if the
 *   password matches the user password in that failed.
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
