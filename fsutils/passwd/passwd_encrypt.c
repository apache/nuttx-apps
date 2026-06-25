/****************************************************************************
 * apps/fsutils/passwd/passwd_encrypt.c
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
#include <nuttx/debug.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/random.h>
#include <unistd.h>

#include <netutils/base64.h>

#include "passwd.h"
#include "passwd_pbkdf2.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_FSUTILS_PASSWD_PBKDF2_ITERATIONS
#  define CONFIG_FSUTILS_PASSWD_PBKDF2_ITERATIONS 10000
#endif

#define PASSWD_MIN_LENGTH  8

static const char g_password_specials[] =
  "!@#$%^&*()_+-=[]{}|;:,.<>?";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int validate_password_complexity(FAR const char *password)
{
  FAR const char *p;
  size_t passlen;
  int has_upper   = 0;
  int has_lower   = 0;
  int has_digit   = 0;
  int has_special = 0;

  passlen = strlen(password);
  if (passlen < PASSWD_MIN_LENGTH)
    {
      _err("ERROR: password must be at least %d characters\n",
           PASSWD_MIN_LENGTH);
      return -EINVAL;
    }

  if (passlen > MAX_PASSWORD)
    {
      _err("ERROR: password must be at most %d characters\n", MAX_PASSWORD);
      return -EINVAL;
    }

  for (p = password; *p != '\0'; p++)
    {
      if (isupper((unsigned char)*p))
        {
          has_upper = 1;
        }
      else if (islower((unsigned char)*p))
        {
          has_lower = 1;
        }
      else if (isdigit((unsigned char)*p))
        {
          has_digit = 1;
        }
      else if (strchr(g_password_specials, *p) != NULL)
        {
          has_special = 1;
        }
    }

  if (!has_upper)
    {
      _err("ERROR: password must contain at least one uppercase "
           "letter (A-Z)\n");
      return -EINVAL;
    }

  if (!has_lower)
    {
      _err("ERROR: password must contain at least one lowercase "
           "letter (a-z)\n");
      return -EINVAL;
    }

  if (!has_digit)
    {
      _err("ERROR: password must contain at least one digit (0-9)\n");
      return -EINVAL;
    }

  if (!has_special)
    {
      _err("ERROR: password must contain at least one special "
           "character (!@#$%%^&*()_+-=[]{}|;:,.<>?)\n");
      return -EINVAL;
    }

  return OK;
}

/****************************************************************************
 * Name: passwd_fill_random
 *
 * Description:
 *   Fill a buffer with random bytes using getrandom() or /dev/urandom.
 *
 ****************************************************************************/

static int passwd_fill_random(FAR uint8_t *buf, size_t len)
{
  ssize_t nread;
  int fd;

  nread = getrandom(buf, len, 0);
  if (nread == (ssize_t)len)
    {
      return OK;
    }

  fd = open("/dev/urandom", O_RDONLY);
  if (fd < 0)
    {
      return -errno;
    }

  nread = read(fd, buf, len);
  close(fd);

  if (nread != (ssize_t)len)
    {
      return nread < 0 ? -errno : -EIO;
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: passwd_encrypt
 *
 * Description:
 *   Hash a password with PBKDF2-HMAC-SHA256 and encode as modular crypt
 *   format: $pbkdf2-sha256$<iter>$<base64url-salt>$<base64url-hash>
 *
 ****************************************************************************/

int passwd_encrypt(FAR const char *password,
                   char encrypted[MAX_ENCRYPTED + 1])
{
  uint8_t salt[PASSWD_SALT_BYTES];
  uint8_t hash[PASSWD_HASH_BYTES];
  char salt_b64[32];
  char hash_b64[48];
  size_t passlen;
  int ret;

  ret = validate_password_complexity(password);
  if (ret < 0)
    {
      return ret;
    }

  passlen = strlen(password);

  ret = passwd_fill_random(salt, sizeof(salt));
  if (ret < 0)
    {
      return ret;
    }

  ret = passwd_pbkdf2_hmac_sha256((FAR const uint8_t *)password, passlen,
                                  salt, sizeof(salt),
                                  CONFIG_FSUTILS_PASSWD_PBKDF2_ITERATIONS,
                                  hash, sizeof(hash));
  if (ret < 0)
    {
      return ret;
    }

  ret = base64url_encode(salt, sizeof(salt), salt_b64,
                                sizeof(salt_b64));
  if (ret < 0)
    {
      return ret;
    }

  ret = base64url_encode(hash, sizeof(hash), hash_b64,
                                sizeof(hash_b64));
  if (ret < 0)
    {
      return ret;
    }

  ret = snprintf(encrypted, MAX_ENCRYPTED + 1,
                 PASSWD_MCF_PREFIX "%u$%s$%s",
                 CONFIG_FSUTILS_PASSWD_PBKDF2_ITERATIONS,
                 salt_b64, hash_b64);
  if (ret < 0 || (size_t)ret > MAX_ENCRYPTED)
    {
      return -E2BIG;
    }

  return OK;
}
