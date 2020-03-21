/****************************************************************************
 * apps/fsutils/passwd/passwd.h
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

#ifndef __APPS_FSUTILS_PASSWD_PASSWD_H
#define __APPS_FSUTILS_PASSWD_PASSWD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <semaphore.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_ENCRYPTED 48         /* Maximum size of a password (encrypted, ASCII) */
#define MAX_USERNAME  48         /* Maximum size of a username */
#define MAX_RECORD    (MAX_USERNAME + MAX_ENCRYPTED + 1)

/* The TEA incryption algorithm generates 8 bytes of encrypted data per
 * 8 bytes of unencrypted data.  The encrypted presentation is base64 which
 * is 8-bits of ASCII for each 6 bits of data.  That is a 3-to-4 expansion
 * ratio.  MAX_ENCRYPTED must be a multiple of 8 bytes.
 */

#define MAX_PASSWORD  (3 * MAX_ENCRYPTED / 4)

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct passwd_s
{
  off_t offset;                      /* File offset (start of record) */
  char encrypted[MAX_ENCRYPTED + 1]; /* Encrtyped password in file */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: passwd_lock and passwd_unlock
 *
 * Description:
 *   Lock the /etc/passwd file.  This is not a real lock at the level of the
 *   file system.  Rather, it only prevents concurrent modification of the
 *   /etc/passwd file by passwd_adduser(), passwd_deluser(), and
 *   passwd_update().  Other accesses to /etc/passwd could still cause
 *   concurrency problem and file corruption.
 *
 * Input Parameters:
 *
 * Returned Value:
 *   Zero (OK) is returned on success; a negated errno value is returned on
 *   failure.
 *
 ****************************************************************************/

#if !defined(CONFIG_FSUTILS_PASSWD_READONLY)
#  define PASSWD_SEM_DECL(s) FAR sem_t *s
int passwd_lock(FAR sem_t **semp);
void passwd_unlock(FAR sem_t *sem);
#else
#  define PASSWD_SEM_DECL(s)
#  define passwd_lock(semp)  (0)
#  define passwd_unlock(sem)
#endif

/****************************************************************************
 * Name: passwd_encrypt
 *
 * Description:
 *   Encrypt a password.  Currently uses the Tiny Encryption Algorithm.
 *
 * Input Parameters:
 *   password -- The password string to be encrypted
 *
 * Returned Value:
 *   Zero (OK) is returned on success; a negated errno value is returned on
 *   failure.
 *
 ****************************************************************************/

int passwd_encrypt(FAR const char *password,
                   char encrypted[MAX_ENCRYPTED + 1]);

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

int passwd_append(FAR const char *username, FAR const char *password);

/****************************************************************************
 * Name: passwd_delete
 *
 * Description:
 *  Delete on record from the password file at offset.
 *
 * Input Parameters:
 *
 * Returned Value:
 *   Zero (OK) is returned on success; a negated errno value is returned on
 *   failure.
 *
 ****************************************************************************/

int passwd_delete(off_t offset);

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

int passwd_find(FAR const char *username, FAR struct passwd_s *passwd);

#endif /* __APPS_FSUTILS_PASSWD_PASSWD_H */
