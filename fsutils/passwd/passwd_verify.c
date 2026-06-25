/****************************************************************************
 * apps/fsutils/passwd/passwd_verify.c
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include <nuttx/config.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netutils/base64.h>
#include <fsutils/passwd.h>

#include "passwd.h"
#include "passwd_pbkdf2.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: passwd_verify_hash
 *
 * Description:
 *   Verify a password against a stored PBKDF2-SHA256 modular crypt hash.
 *   Rejects any hash not in $pbkdf2-sha256$ format.
 *
 * Returned Value:
 *   0 on match, -1 on mismatch or parse error.
 *
 ****************************************************************************/

static int passwd_verify_hash(FAR const char *stored,
                              FAR const char *password)
{
  FAR const char *p;
  FAR const char *salt_b64;
  FAR const char *hash_b64;
  char *endptr;
  uint8_t salt[PASSWD_SALT_BYTES];
  uint8_t expected[PASSWD_HASH_BYTES];
  uint8_t actual[PASSWD_HASH_BYTES];
  size_t saltlen;
  size_t hashlen;
  size_t passlen;
  unsigned long iterations;
  int ret;

  if (strncmp(stored, PASSWD_MCF_PREFIX, strlen(PASSWD_MCF_PREFIX)) != 0)
    {
      return -1;
    }

  p = stored + strlen(PASSWD_MCF_PREFIX);
  iterations = strtoul(p, &endptr, 10);
  if (endptr == p || *endptr != '$' || iterations < 1 ||
      iterations > 200000)
    {
      return -1;
    }

  salt_b64 = endptr + 1;
  hash_b64 = strchr(salt_b64, '$');
  if (hash_b64 == NULL)
    {
      return -1;
    }

  ret = base64url_decode(salt_b64, salt, sizeof(salt), &saltlen);
  if (ret < 0 || saltlen == 0)
    {
      return -1;
    }

  ret = base64url_decode(hash_b64 + 1, expected, sizeof(expected),
                                &hashlen);
  if (ret < 0 || hashlen != PASSWD_HASH_BYTES)
    {
      return -1;
    }

  passlen = strlen(password);
  if (passlen == 0 || passlen > MAX_PASSWORD)
    {
      return -1;
    }

  ret = passwd_pbkdf2_hmac_sha256((FAR const uint8_t *)password, passlen,
                                  salt, saltlen, (uint32_t)iterations,
                                  actual, sizeof(actual));
  if (ret < 0)
    {
      return -1;
    }

  if (timingsafe_bcmp(actual, expected, sizeof(expected)) != 0)
    {
      return -1;
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: passwd_verify
 *
 * Description:
 *   Return true if the username exists in the /etc/passwd file and if the
 *   password matches the user password in that file.
 *
 * Returned Value:
 *   Zero (0) is returned on a successful match, -1 on mismatch or invalid
 *   hash format; a negated errno value is returned on other failures.
 *
 ****************************************************************************/

int passwd_verify(FAR const char *username, FAR const char *password)
{
  struct passwd_s passwd;
  PASSWD_SEM_DECL(sem);
  int ret;

  ret = passwd_lock(&sem);
  if (ret < 0)
    {
      return ret;
    }

  ret = passwd_find(username, &passwd);
  if (ret < 0)
    {
      goto errout_with_lock;
    }

  ret = passwd_verify_hash(passwd.encrypted, password);

errout_with_lock:
  passwd_unlock(sem);
  return ret;
}
