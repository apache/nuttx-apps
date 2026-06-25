/****************************************************************************
 * apps/testing/crypto/passwd/pbkdf2_test.c
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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <fsutils/passwd.h>

#include "passwd.h"
#include "passwd_pbkdf2.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_USERNAME  "testuser"
#define TEST_PASSWORD  "MySecret1!"
#define WRONG_PASSWORD "WrongPass"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct pbkdf2_vector_s
{
  FAR const char *password;
  size_t          passwordlen;
  FAR const char *salt;
  size_t          saltlen;
  uint32_t        iterations;
  size_t          dklen;
  FAR const uint8_t *expected;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* PBKDF2-HMAC-SHA256 test vectors from RFC 6070 (appendix B). */

static const uint8_t g_vector1[] =
{
  0x12, 0x0f, 0xb6, 0xcf, 0xfc, 0xf8, 0xb3, 0x2c,
  0x43, 0xe7, 0x22, 0x52, 0x56, 0xc4, 0xf8, 0x37,
  0xa8, 0x65, 0x48, 0xc9
};

static const uint8_t g_vector2[] =
{
  0xae, 0x4d, 0x0c, 0x95, 0xaf, 0x6b, 0x46, 0xd3,
  0x2d, 0x0a, 0xdf, 0xf9, 0x28, 0xf0, 0x6d, 0xd0,
  0x2a, 0x30, 0x3f, 0x8e
};

static const uint8_t g_vector3[] =
{
  0xc5, 0xe4, 0x78, 0xd5, 0x92, 0x88, 0xc8, 0x41,
  0xaa, 0x53, 0x0d, 0xb6, 0x84, 0x5c, 0x4c, 0x8d,
  0x96, 0x28, 0x93, 0xa0
};

static const uint8_t g_vector4[] =
{
  0xcf, 0x81, 0xc6, 0x6f, 0xe8, 0xcf, 0xc0, 0x4d,
  0x1f, 0x31, 0xec, 0xb6, 0x5d, 0xab, 0x40, 0x89,
  0xf7, 0xf1, 0x79, 0xe8
};

static const uint8_t g_vector5[] =
{
  0x34, 0x8c, 0x89, 0xdb, 0xcb, 0xd3, 0x2b, 0x2f,
  0x32, 0xd8, 0x14, 0xb8, 0x11, 0x6e, 0x84, 0xcf,
  0x2b, 0x17, 0x34, 0x7e, 0xbc, 0x18, 0x00, 0x18,
  0x1c
};

static const uint8_t g_vector6[] =
{
  0x89, 0xb6, 0x9d, 0x05, 0x16, 0xf8, 0x29, 0x89,
  0x3c, 0x69, 0x62, 0x26, 0x65, 0x0a, 0x86, 0x87
};

static const struct pbkdf2_vector_s g_vectors[] =
{
  {
    "password", 8,
    "salt", 4,
    1, 20,
    g_vector1
  },
  {
    "password", 8,
    "salt", 4,
    2, 20,
    g_vector2
  },
  {
    "password", 8,
    "salt", 4,
    4096, 20,
    g_vector3
  },
  {
    "password", 8,
    "salt", 4,
    16777216, 20,
    g_vector4
  },
  {
    "passwordPASSWORDpassword", 24,
    "saltSALTsaltSALTsaltSALTsaltSALTsalt", 36,
    4096, 25,
    g_vector5
  },
  {
    "pass\0word", 9,
    "sa\0lt", 5,
    4096, 16,
    g_vector6
  },
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int test_pbkdf2_vectors(void)
{
  FAR const struct pbkdf2_vector_s *vec;
  uint8_t output[32];
  int failures = 0;
  int i;
  int ret;

  for (i = 0; i < (int)(sizeof(g_vectors) / sizeof(g_vectors[0])); i++)
    {
      vec = &g_vectors[i];

#ifndef CONFIG_TESTING_PBKDF2_SLOW_VECTOR
      if (vec->iterations > 100000)
        {
          printf("pbkdf2_test: skipping slow vector %d (enable "
                 "TESTING_PBKDF2_SLOW_VECTOR)\n", i);
          continue;
        }
#endif

      ret = passwd_pbkdf2_hmac_sha256((FAR const uint8_t *)vec->password,
                                      vec->passwordlen,
                                      (FAR const uint8_t *)vec->salt,
                                      vec->saltlen,
                                      vec->iterations,
                                      output, vec->dklen);
      if (ret != 0)
        {
          printf("pbkdf2_test: vector %d pbkdf2 failed: %d\n", i, ret);
          failures++;
          continue;
        }

      if (memcmp(output, vec->expected, vec->dklen) != 0)
        {
          printf("pbkdf2_test: vector %d output mismatch\n", i);
          failures++;
        }
    }

  if (failures == 0)
    {
      printf("pbkdf2_test: RFC 6070 SHA-256 vectors OK\n");
    }

  return failures;
}

static int test_passwd_roundtrip(void)
{
#if defined(CONFIG_FSUTILS_PASSWD_READONLY)
  printf("pbkdf2_test: skipping passwd round-trip "
         "(FSUTILS_PASSWD_READONLY)\n");
  return 0;
#elif !defined(CONFIG_DEV_URANDOM)
  printf("pbkdf2_test: skipping passwd round-trip "
         "(DEV_URANDOM)\n");
  return 0;
#else
  FILE *stream;
  char encrypted[MAX_ENCRYPTED + 1];
  int ret;

  unlink(CONFIG_FSUTILS_PASSWD_PATH);

  ret = passwd_encrypt(TEST_PASSWORD, encrypted);
  if (ret < 0)
    {
      printf("pbkdf2_test: passwd_encrypt failed: %d\n", ret);
      return 1;
    }

  stream = fopen(CONFIG_FSUTILS_PASSWD_PATH, "w");
  if (stream == NULL)
    {
      printf("pbkdf2_test: cannot write %s: %d\n",
             CONFIG_FSUTILS_PASSWD_PATH, errno);
      return 1;
    }

  if (fprintf(stream, "%s:%s:0:0:/\n", TEST_USERNAME, encrypted) < 0)
    {
      printf("pbkdf2_test: fprintf failed: %d\n", errno);
      fclose(stream);
      return 1;
    }

  fclose(stream);

  ret = passwd_verify(TEST_USERNAME, TEST_PASSWORD);
  if (ret != 0)
    {
      printf("pbkdf2_test: passwd_verify match failed: %d (expected 0)\n",
             ret);
      return 1;
    }

  ret = passwd_verify(TEST_USERNAME, WRONG_PASSWORD);
  if (ret != -1)
    {
      printf("pbkdf2_test: passwd_verify mismatch failed: %d "
             "(expected -1)\n", ret);
      return 1;
    }

  printf("pbkdf2_test: passwd round-trip OK\n");
  return 0;
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int failures = 0;

  (void)argc;
  (void)argv;

  failures += test_pbkdf2_vectors();
  failures += test_passwd_roundtrip();

  if (failures != 0)
    {
      printf("pbkdf2_test: FAILED (%d)\n", failures);
      return 1;
    }

  printf("pbkdf2_test: PASSED\n");
  return 0;
}
